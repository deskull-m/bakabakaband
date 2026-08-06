#include "pet/pet-util.h"
#include "core/stuff-handler.h"
#include "grid/grid.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "player-info/class-info.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/redrawing-flags-updater.h"
#include "util/bit-flags-calculator.h"
#include "world/world.h"

int total_friends = 0;

/*!
 * @brief プレイヤーの騎乗/下馬処理判定
 * @param grid プレイヤーの移動先グリッドへの参照
 * @param now_riding trueなら下馬処理、falseならば騎乗処理
 * @return 可能ならばtrueを返す
 */
bool can_player_ride_pet(CreatureEntity &creature, const Grid &grid, bool now_riding)
{
    auto &world = AngbandWorld::get_instance();
    const auto old_character_xtra = world.character_xtra;
    const auto old_riding = creature.get_riding();
    const auto old_riding_two_hands = creature.is_riding_ryoute();
    const auto old_old_riding_two_hands = creature.was_riding_ryoute();
    const auto old_pf_two_hands = creature.has_pet_extra_flag(PF_TWO_HANDS);
    world.character_xtra = true;

    if (now_riding) {
        creature.ride_monster(grid.m_idx);
    } else {
        creature.ride_monster(0);
        creature.remove_pet_extra_flag(PF_TWO_HANDS);
        creature.set_was_riding_ryoute(false);
        creature.set_riding_ryoute(false);
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(creature);

    bool p_can_enter = player_can_enter(creature, grid.feat, CEM_P_CAN_ENTER_PATTERN);
    creature.ride_monster(old_riding);
    if (old_pf_two_hands) {
        creature.add_pet_extra_flag(PF_TWO_HANDS);
    } else {
        creature.remove_pet_extra_flag(PF_TWO_HANDS);
    }

    creature.set_riding_ryoute(old_riding_two_hands);
    creature.set_was_riding_ryoute(old_old_riding_two_hands);
    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(creature);

    world.character_xtra = old_character_xtra;
    return p_can_enter;
}

/*!
 * @brief ペットの維持コスト計算
 * @return 維持コスト(%)
 */
PERCENTAGE calculate_upkeep(CreatureEntity &creature)
{
    bool has_a_unique = false;
    DEPTH total_friend_levels = 0;
    total_friends = 0;
    for (auto m_idx = creature.get_floor()->m_max - 1; m_idx >= 1; m_idx--) {
        const auto &monster = creature.get_floor()->get_monster(static_cast<MONSTER_IDX>(m_idx));
        if (!monster.is_valid()) {
            continue;
        }
        const auto &monrace = monster.get_monrace();

        if (!monster.is_pet()) {
            continue;
        }

        total_friends++;
        if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE)) {
            total_friend_levels += monrace.level;
            continue;
        }

        if (creature.pclass != PlayerClassType::CAVALRY) {
            total_friend_levels += (monrace.level + 5) * 10;
            continue;
        }

        if (monster.is_riding()) {
            total_friend_levels += (monrace.level + 5) * 2;
        } else if (!has_a_unique && monster.get_monrace().misc_flags.has(MonsterMiscType::RIDING)) {
            total_friend_levels += (monrace.level + 5) * 7 / 2;
        } else {
            total_friend_levels += (monrace.level + 5) * 10;
        }

        has_a_unique = true;
    }

    if (total_friends == 0) {
        return 0;
    }

    // pet_upkeep_div は職業を持たないクリーチャー (対応職業のないモンスター等)
    // では 0 になりうるため、ゼロ除算を避けてレベル分の軽減項を省く。
    const int pet_upkeep_div = (*creature.get_class_info()).pet_upkeep_div;
    int upkeep_factor = total_friend_levels;
    if (pet_upkeep_div != 0) {
        upkeep_factor -= creature.get_level() * 80 / pet_upkeep_div;
    }
    if (upkeep_factor < 0) {
        upkeep_factor = 0;
    }

    if (upkeep_factor > 1000) {
        upkeep_factor = 1000;
    }

    return upkeep_factor;
}
