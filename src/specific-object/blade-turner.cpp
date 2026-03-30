#include "specific-object/blade-turner.h"
#include "effect/attribute-types.h"
#include "hpmp/hp-mp-processor.h"
#include "spell-kind/spells-launcher.h"
#include "status/bad-status-setter.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "system/creature-entity.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

bool activate_bladeturner(CreatureEntity &creature)
{
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    msg_print(_("あなたはエレメントのブレスを吐いた。", "You breathe the elements."));
    fire_breath(creature, AttributeType::MISSILE, dir, 300, 4);
    msg_print(_("鎧が様々な色に輝いた...", "Your armor glows many colours..."));
    (void)BadStatusSetter(creature).set_fear(0);
    (void)set_hero(creature, randint1(50) + 50, false);
    (void)hp_player(creature, 10);
    (void)set_blessed(creature, randint1(50) + 50, false);
    (void)set_oppose_acid(creature, randint1(50) + 50, false);
    (void)set_oppose_elec(creature, randint1(50) + 50, false);
    (void)set_oppose_fire(creature, randint1(50) + 50, false);
    (void)set_oppose_cold(creature, randint1(50) + 50, false);
    (void)set_oppose_pois(creature, randint1(50) + 50, false);
    return true;
}
