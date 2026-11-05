#include "monster-floor/party-generator.h"
#include "dungeon/dungeon-flag-types.h"
#include "game-option/cheat-types.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "system/creature-entity.h"
#include "system/creature-party/creature-party-definition.h"
#include "system/creature-party/creature-party-list.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "util/probability-table.h"
#include "view/display-messages.h"
#include "wizard/wizard-messages.h"
#include <algorithm>

namespace {
/*!
 * @brief パーティ選択の重み付け基準値
 * @details `rarity` の逆数に比例した重みを与える。換算式は monrace の出現テーブル
 *          (`monrace-allocation.cpp` の `100 / rarity`) と揃えてある。
 */
constexpr auto PARTY_RARITY_WEIGHT_BASE = 100;

/*!
 * @brief 「パーティを出さない」仮想候補の重み
 * @details rarity 1 のパーティ 1 件と同じ重み。これを抽選に混ぜることで、`rarity` が
 *          候補同士の相対確率だけでなく**絶対的な出現率**にも効く。これが無いと、
 *          その階層の候補が希少なパーティ 1 件だけの場合に rarity を無視して必ず
 *          出てしまう。パーティ全体の出現頻度を上げ下げしたい場合はこの値を
 *          (小さくするほど出やすい) 調整する。
 */
constexpr auto PARTY_NO_SELECTION_WEIGHT = 100;

//! 「パーティを出さない」仮想候補の ID。パーティ ID は 1 以上 (schema 準拠) なので衝突しない
constexpr auto NO_PARTY_ID = -1;
}

/*!
 * @brief パーティ形式でモンスターを生成する
 * @param creature クリーチャーへの参照
 * @param pos_center パーティの中心座標
 * @return 生成に成功したらtrue
 * @details 階層レベルで絞った候補から `rarity` の逆数に比例した重みで 1 件を選び、
 *          そのメンバーを中心座標の周囲へ散開配置する。候補には「パーティを出さない」
 *          仮想候補が混ざるため、希少なパーティしか居ない階層では何も生成せず
 *          false を返す (呼出側が通常のモンスター生成へフォールバックする)。
 */
bool alloc_creature_party(CreatureEntity &creature, const Pos2D &pos_center)
{
    auto &floor = *creature.get_floor();

    // 現在の階層に適したパーティを取得
    auto &party_list = CreaturePartyList::get_instance();
    const auto available_parties = party_list.get_available_parties(floor.dun_level);
    if (available_parties.empty()) {
        return false;
    }

    // レアリティに基づいてパーティを選択する。rarity が小さいほど重みが大きい。
    ProbabilityTable<int> party_table;
    for (const auto party_id : available_parties) {
        const auto &party = party_list.get_party(party_id);

        // get_available_parties() が既に同じ条件で絞っているので通常は常に真。
        // 両者が将来食い違った場合の保険として残してある。
        if (!party.can_generate(floor.dun_level)) {
            continue;
        }

        // 重み 0 は ProbabilityTable に登録されず候補から消えてしまうため、
        // schema 上限の rarity 255 でも最低 1 の重みを残す。
        party_table.entry_item(party_id, std::max(1, PARTY_RARITY_WEIGHT_BASE / party.rarity));
    }

    if (party_table.empty()) {
        return false;
    }

    // rarity を絶対的な出現率にも効かせるための仮想候補 (説明は定数の宣言を参照)。
    party_table.entry_item(NO_PARTY_ID, PARTY_NO_SELECTION_WEIGHT);

    const auto selected_party_id = party_table.pick_one_at_random();
    if (selected_party_id == NO_PARTY_ID) {
        return false;
    }

    const auto &party = party_list.get_party(selected_party_id);

    // パーティメンバーを生成
    int generated_count = 0;
    const auto &monrace_list = MonraceList::get_instance();

    for (const auto &member : party.members) {
        // 確率判定
        if (randint1(100) > member.probability) {
            continue;
        }

        // ユニークモンスターの扱い
        const auto &monrace = monrace_list.get_monrace(member.monrace_id);
        if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
            if (!party.allow_unique) {
                continue; // ユニーク不可のパーティでは生成しない
            }
        }

        // 生成数をダイスで決定
        const int count = member.count_dice.roll();
        for (int i = 0; i < count; i++) {
            // 散開位置を取得
            const auto pos_scatter = mon_scatter(creature, member.monrace_id, pos_center, 5);
            if (!pos_scatter) {
                continue;
            }

            // モンスターを配置
            const auto m_idx = place_specific_monster(creature, pos_scatter->y, pos_scatter->x,
                member.monrace_id, PM_ALLOW_SLEEP);
            if (m_idx) {
                generated_count++;
            }
        }
    }

    // 1体も生成できなかった場合は失敗
    if (generated_count == 0) {
        return false;
    }

    msg_print_wizard(creature, CHEAT_MONSTER,
        _("パーティ「{}」を生成しました ({}体)", "Generated party \"{}\" ({} creatures)"),
        party.name.string(), generated_count);
    return true;
}
