#include "mind/mind-berserker.h"
#include "action/movement-execution.h"
#include "cmd-action/cmd-attack.h"
#include "floor/geometry.h"
#include "game-option/input-options.h"
#include "grid/grid.h"
#include "mind/mind-numbers.h"
#include "player-attack/player-attack.h"
#include "player/player-move.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/spells-detection.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

/*!
 * @brief 怒りの発動 /
 * do_cmd_cast calls this function if the player's class is 'berserker'.
 * @param creature クリーチャーへの参照
 * @param spell 発動する特殊技能のID
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
bool cast_berserk_spell(CreatureEntity &creature, MindBerserkerType spell)
{
    switch (spell) {
    case MindBerserkerType::DETECT_MANACE:
        detect_monsters_mind(creature, DETECT_RAD_DEFAULT);
        return true;
    case MindBerserkerType::CHARGE: {
        if (creature.riding) {
            msg_print(_("乗馬中には無理だ。", "You cannot do it when riding."));
            return false;
        }

        const auto dir = get_direction(creature);
        if (!dir.has_direction()) {
            return false;
        }

        const auto pos = creature.get_neighbor(dir);
        const auto &floor = *creature.current_floor_ptr;
        const auto &grid = floor.get_grid(pos);
        if (!grid.has_monster()) {
            msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
            return false;
        }

        do_cmd_attack(creature, pos.y, pos.x, HISSATSU_NONE);
        if (!player_can_enter(creature, grid.feat, 0) || floor.has_trap_at(pos)) {
            return true;
        }

        const auto pos_new = pos + dir.vec();
        const auto &grid_new = floor.get_grid(pos_new);
        if (player_can_enter(creature, grid_new.feat, 0) && !floor.has_trap_at(pos_new) && !grid_new.has_monster()) {
            msg_erase();
            (void)move_player_effect(creature, pos_new.y, pos_new.x, MPE_FORGET_FLOW | MPE_HANDLE_STUFF | MPE_DONT_PICKUP);
        }

        return true;
    }
    case MindBerserkerType::SMASH_TRAP: {
        const auto dir = get_direction(creature);
        if (!dir) {
            return false;
        }

        exe_movement(creature, dir, easy_disarm, true);
        return true;
    }
    case MindBerserkerType::QUAKE:
        earthquake(creature, creature.get_position(), 8 + randint0(5));
        return true;
    case MindBerserkerType::MASSACRE:
        massacre(creature);
        return true;
    default:
        msg_print(_("なに？", "Zap?"));
        return true;
    }
}
