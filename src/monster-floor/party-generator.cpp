#include "monster-floor/party-generator.h"
#include "alliance/alliance.h"
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
#include <string>
#include <vector>

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

/*!
 * @brief パーティのメンバー全員を、パーティが属するアライアンスへ同化させる
 * @param floor フロアへの参照
 * @param party パーティ定義
 * @param member_m_idx_list 配置に成功したメンバーの m_idx
 * @details パーティが無所属 (`AllianceType::NONE`) なら何もしない。所属がある場合は
 *          **種族定義の `alliance` を上書きして**メンバー個体のアライアンスを揃える。
 *          `CreatureEntity::is_hostile_to_melee()` は個体のアライアンスが異なるだけで
 *          敵対と判定するため、揃えておかないと混成パーティが内輪で殺し合う。
 *          なお生成時の友好/敵対判定 (`one_monster_placer.cpp` の `isFriendly()`) は
 *          各メンバーの配置中、つまり**本関数より前**に種族定義のアライアンスで
 *          行われる。そのため「種族が持たないアライアンス」をパーティへ指定すると
 *          態度と所属が食い違いうる。実データでは**メンバー全員が既にそのアライアンスを
 *          持つパーティにのみ指定する**方針で、この食い違いを避けている。
 */
void unify_party_alliance(FloorType &floor, const CreaturePartyDefinition &party, const std::vector<MONSTER_IDX> &member_m_idx_list)
{
    if (!party.belongs_to_alliance()) {
        return;
    }

    for (const auto m_idx : member_m_idx_list) {
        floor.get_monster(m_idx).set_alliance_idx(party.alliance_idx);
    }
}

/*!
 * @brief 生成したメンバーにパーティ所属とリーダーを割り当てる
 * @param floor フロアへの参照
 * @param party パーティ定義
 * @param member_m_idx_list 配置に成功したメンバーの m_idx
 * @return リーダーに選ばれた個体の m_idx
 * @details リーダーは**種族レベルが最も高い個体**とする (同値なら先に配置された方)。
 *          リーダー自身にも自分の m_idx を持たせることで、`party_leader_m_idx` が
 *          そのままグループ識別子になる (m_idx はフロア内で一意なので別途の
 *          インスタンス番号を持たなくても同一パーティの複数実体を区別できる)。
 *          併せてパーティがアライアンスに属している場合は全メンバーをそれへ同化させ、
 *          一団が 1 つの勢力として振舞うようにする (詳細は
 *          `unify_party_alliance()` を参照)。
 */
MONSTER_IDX assign_party_membership(FloorType &floor, const CreaturePartyDefinition &party, const std::vector<MONSTER_IDX> &member_m_idx_list)
{
    const auto leader_it = std::max_element(member_m_idx_list.begin(), member_m_idx_list.end(),
        [&floor](MONSTER_IDX a, MONSTER_IDX b) {
            return floor.get_monster(a).get_monrace().level < floor.get_monster(b).get_monrace().level;
        });
    const auto leader_m_idx = *leader_it;

    for (const auto m_idx : member_m_idx_list) {
        floor.get_monster(m_idx).join_party(static_cast<short>(party.idx), leader_m_idx);
    }

    unify_party_alliance(floor, party, member_m_idx_list);

    return leader_m_idx;
}
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
    std::vector<MONSTER_IDX> member_m_idx_list;
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
                member_m_idx_list.push_back(*m_idx);
            }
        }
    }

    // 1体も生成できなかった場合は失敗
    if (member_m_idx_list.empty()) {
        return false;
    }

    const auto leader_m_idx = assign_party_membership(floor, party, member_m_idx_list);

    const auto alliance_name = party.belongs_to_alliance()
                                   ? alliance_list.at(party.alliance_idx)->name
                                   : std::string(_("無所属", "none"));
    msg_print_wizard(creature, CHEAT_MONSTER,
        _("パーティ「{}」を生成しました ({}体, リーダー: {} @ {},{}, アライアンス: {})",
            "Generated party \"{}\" ({} creatures, leader: {} @ {},{}, alliance: {})"),
        party.name.string(), member_m_idx_list.size(),
        floor.get_monster(leader_m_idx).get_monrace().name.string(),
        floor.get_monster(leader_m_idx).y, floor.get_monster(leader_m_idx).x, alliance_name);
    return true;
}
