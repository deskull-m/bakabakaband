#include "object-activation/activation-charm.h"
#include "spell-kind/spells-charm.h"
#include "spell-kind/spells-sight.h"
#include "system/creature-entity.h"
#include "target/target-getter.h"

namespace {
template <typename SpellFunc>
bool activate_charm_directional(CreatureEntity &creature, SpellFunc spell, int power)
{
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    (void)spell(creature, dir, static_cast<PLAYER_LEVEL>(power));
    return true;
}
}

bool activate_charm_animal(CreatureEntity &creature)
{
    return activate_charm_directional(creature, charm_animal, creature.get_level());
}

bool activate_charm_undead(CreatureEntity &creature)
{
    return activate_charm_directional(creature, control_one_undead, creature.get_level());
}

bool activate_charm_other(CreatureEntity &creature)
{
    return activate_charm_directional(creature, charm_monster, creature.get_level() * 2);
}

bool activate_charm_animals(CreatureEntity &creature)
{
    (void)charm_animals(creature, creature.get_level() * 2);
    return true;
}

bool activate_charm_others(CreatureEntity &creature)
{
    (void)charm_monsters(creature, creature.get_level() * 2);
    return true;
}
