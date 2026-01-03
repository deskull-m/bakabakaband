#include "monster-floor/party-generator.h"
#include "dungeon/dungeon-flag-types.h"
#include "game-option/cheat-types.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "system/creature-party/creature-party-definition.h"
#include "system/creature-party/creature-party-list.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "util/probability-table.h"
#include "view/display-messages.h"

/*!
 * @brief パーティ形式でモンスターを生成する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pos_center パーティの中心座標
 * @return 生成に成功したらtrue
 */
bool alloc_creature_party(PlayerType *player_ptr, const Pos2D &pos_center)
{
    auto &floor = *player_ptr->current_floor_ptr;

    // 現在の階層に適したパーティを取得
    auto &party_list = CreaturePartyList::get_instance();
    const auto available_parties = party_list.get_available_parties(floor.dun_level);
    if (available_parties.empty()) {
        return false;
    }

    // レアリティに基づいてパーティを選択
    ProbabilityTable<int> party_table;
    for (const auto party_id : available_parties) {
        const auto &party = party_list.get_party(party_id);
        if (party.can_generate(floor.dun_level)) {
            // レアリティが高いほど出現確率が低い
            party_table.entry_item(party_id, 255 - party.rarity);
        }
    }

    if (party_table.empty()) {
        return false;
    }

    const auto selected_party_id = party_table.pick_one_at_random();
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
            const auto pos_scatter = mon_scatter(player_ptr, member.monrace_id, pos_center, 5);
            if (!pos_scatter) {
                continue;
            }

            // モンスターを配置
            const auto m_idx = place_specific_monster(player_ptr, pos_scatter->y, pos_scatter->x,
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

    return true;
}
