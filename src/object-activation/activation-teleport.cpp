#include "object-activation/activation-teleport.h"
#include "cmd-io/cmd-save.h"
#include "core/asking-player.h"
#include "effect/attribute-types.h"
#include "game-option/special-options.h"
#include "spell-kind/spells-grid.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "system/creature-entity.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

bool activate_teleport_away(CreatureEntity &creature)
{
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    (void)fire_beam(creature, AttributeType::AWAY_ALL, dir, creature.get_level());
    return true;
}

bool activate_escape(CreatureEntity &creature)
{
    switch (randint1(13)) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        teleport_player(creature, 10, TELEPORT_SPONTANEOUS);
        return true;
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
        teleport_player(creature, 222, TELEPORT_SPONTANEOUS);
        return true;
    case 11:
    case 12:
        (void)stair_creation(creature);
        return true;
    default:
        if (!input_check(_("この階を去りますか？", "Leave this level? "))) {
            return true;
        }

        if (autosave_l) {
            do_cmd_save_game(creature, true);
        }

        creature.leaving = true;
        return true;
    }
}

bool activate_teleport_level(CreatureEntity &creature)
{
    if (!input_check(_("本当に他の階にテレポートしますか？", "Are you sure? (Teleport Level)"))) {
        return false;
    }

    teleport_level(creature, 0);
    return true;
}

bool activate_dimension_door(CreatureEntity &creature)
{
    msg_print(_("次元の扉が開いた。目的地を選んで下さい。", "You open a dimensional gate. Choose a destination."));
    return dimension_door(creature);
}

bool activate_teleport(CreatureEntity &creature)
{
    msg_print(_("周りの空間が歪んでいる...", "It twists space around you..."));
    teleport_player(creature, 100, TELEPORT_SPONTANEOUS);
    return true;
}

bool activate_phase_door(CreatureEntity &creature)
{
    teleport_player(creature, 10, TELEPORT_SPONTANEOUS);
    return true;
}
