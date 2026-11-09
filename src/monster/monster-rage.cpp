/*!
 * @brief 植木鉢くんが破壊されたときに周囲のモンスターを激怒させる処理 (Issue #180)
 * @details `MonsterMiscType::UEKIBACHIKUN` を持つ種族が死亡すると、周囲の敵対
 *          モンスターへ `MonsterConstantFlagType::ENRAGED_BY_BROKEN_POT` を付け、
 *          速度・近接命中・近接ダメージ・AC・セーヴに常時ボーナスを乗せる。
 *          既存の「周囲のモンスターを怒らせる」(`aggravate_monsters()` の ANGER、
 *          一時加速のみ) とは別枠の、より大きく永続的な激怒。
 */

#include "monster/monster-rage.h"
#include "grid/grid.h"
#include "monster-race/race-misc-flags.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-status-setter.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "view/display-messages.h"

void enrage_monsters_by_broken_pot(CreatureEntity &creature, MONSTER_IDX pot_m_idx)
{
    auto &floor = *creature.get_floor();
    auto &pot = floor.get_monster(pot_m_idx);
    auto &pot_monrace = pot.get_monrace();
    if (pot_monrace.misc_flags.has_not(MonsterMiscType::UEKIBACHIKUN)) {
        return;
    }

    const auto pot_pos = pot.get_position();
    auto enraged_count = 0;
    auto is_any_visible = false;
    for (auto m_idx = 1; m_idx < floor.m_max; m_idx++) {
        if (m_idx == pot_m_idx) {
            continue;
        }

        auto &monster = floor.get_monster(m_idx);
        if (!monster.is_valid() || monster.is_pet() || monster.is_enraged_by_broken_pot()) {
            continue;
        }

        if (Grid::calc_distance(pot_pos, monster.get_position()) > RAGE_BROKEN_POT_RADIUS) {
            continue;
        }

        monster.set_constant_flag(MonsterConstantFlagType::ENRAGED_BY_BROKEN_POT);

        // 激怒した相手はもう手懐けられない (既存の ANGER と同じ扱い)。
        monster.set_constant_flag(MonsterConstantFlagType::NOPET);

        // 眠っていれば叩き起こし、怯えていれば立ち直らせる。
        (void)set_monster_csleep(floor, m_idx, 0);
        if (monster.is_fearful()) {
            (void)set_monster_monfear(floor, m_idx, 0);
        }

        enraged_count++;
        is_any_visible |= monster.is_visible_on_map();
    }

    if (enraged_count == 0) {
        return;
    }

    // 目の前で起きたなら思い出に記録する (見えていなければ何が起きたか分からない)。
    if (is_any_visible || pot.is_visible_on_map()) {
        pot_monrace.r_misc_flags.set(MonsterMiscType::UEKIBACHIKUN);
    }

    if (is_any_visible) {
        msg_print(_("周囲のモンスターが植木鉢くんの無残な姿に激怒した！", "The monsters nearby are enraged by the pitiful remains of the plant pot!"));
        return;
    }

    msg_print(_("どこかで何かが激怒したような気配を感じた！", "You sense something nearby seething with rage!"));
}
