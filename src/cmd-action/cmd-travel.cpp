#include "cmd-action/cmd-travel.h"
#include "action/travel-execution.h"
#include "core/asking-player.h"
#include "floor/geometry.h"
#include "grid/grid.h"
#include "player/player-move.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "target/grid-selector.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

static tl::optional<Pos2D> decide_travel_goal(CreatureEntity &creature)
{
    const auto &pos_current_goal = Travel::get_instance().get_goal();
    if (pos_current_goal && pos_current_goal != creature.get_position() && input_check(_("トラベルを継続しますか？", "Do you continue to travel? "))) {
        return *pos_current_goal;
    }

    return point_target(creature);
}

/*!
 * @brief トラベル処理のメインルーチン
 */
void do_cmd_travel(CreatureEntity &creature)
{
    const auto pos = decide_travel_goal(creature);
    if (!pos) {
        return;
    }

    if (creature.is_located_at(*pos)) {
        msg_print(_("すでにそこにいます！", "You are already there!!"));
        return;
    }

    if (!Travel::can_travel_to(*creature.current_floor_ptr, *pos)) {
        msg_print(_("そこには行くことができません！", "You cannot travel there!"));
        return;
    }

    Travel::get_instance().set_goal(creature, *pos);
}
