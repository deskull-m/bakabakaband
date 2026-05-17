#include "racial/racial-balrog.h"
#include "effect/attribute-types.h"
#include "player/player-status.h"
#include "spell-kind/spells-launcher.h"
#include "system/creature-entity.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

bool demonic_breath(CreatureEntity &creature)
{
    AttributeType type = (one_in_(2) ? AttributeType::NETHER : AttributeType::FIRE);
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }
    stop_mouth(creature);
    msg_format(_("あなたは%sのブレスを吶いた。", "You breathe %s."), ((type == AttributeType::NETHER) ? _("地獄", "nether") : _("火炎", "fire")));
    fire_breath(creature, type, dir, creature.get_level() * 3, (creature.get_level() / 15) + 1);
    return true;
}
