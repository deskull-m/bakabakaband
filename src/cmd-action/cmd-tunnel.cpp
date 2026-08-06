#include "cmd-action/cmd-tunnel.h"
#include "action/tunnel-execution.h"
#include "cmd-action/cmd-attack.h"
#include "core/disturbance.h"
#include "floor/geometry.h"
#include "grid/grid.h"
#include "io/input-key-requester.h"
#include "player-base/player-class.h"
#include "player-info/samurai-data-type.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/special-defense-types.h"
#include "status/action-setter.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain/terrain-definition.h"
#include "target/target-getter.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 「掘る」動作コマンドのメインルーチン /
 * Tunnels through "walls" (including rubble and closed doors)
 * @details
 * <pre>
 * Note that you must tunnel in order to hit invisible monsters
 * in walls, though moving into walls still takes a turn anyway.
 *
 * Digging is very difficult without a "digger" weapon, but can be
 * accomplished by strong players using heavy weapons.
 * </pre>
 */
void do_cmd_tunnel(CreatureEntity &creature)
{
    CreatureClass(creature).break_samurai_stance({ SamuraiStanceType::MUSOU });

    set_command_repeat_from_arg();

    const auto dir = get_rep_dir(creature);
    if (!dir) {
        disturb(creature, false, false);
        return;
    }

    auto more = false;
    const auto pos = creature.get_neighbor(dir);
    const auto &grid = creature.get_floor()->get_grid(pos);
    const auto &terrain_mimic = grid.get_terrain(TerrainKind::MIMIC);
    if (terrain_mimic.flags.has(TerrainCharacteristics::DOOR)) {
        msg_print(_("ドアは掘れない。", "You cannot tunnel through doors."));
    } else if (terrain_mimic.flags.has_not(TerrainCharacteristics::TUNNEL)) {
        msg_print(_("そこは掘れない。", "You can't tunnel through that."));
    } else if (grid.has_monster()) {
        attack_monster_in_the_way(creature, pos);
    } else {
        more = exe_tunnel(creature, pos.y, pos.x);
    }

    if (!more) {
        disturb(creature, false, false);
    }
}
