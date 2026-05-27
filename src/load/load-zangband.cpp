#include "load/load-zangband.h"
#include "load/load-util.h"
#include "player/attack-defense-types.h"
#include "system/creature-entity.h"

void set_zangband_action(CreatureEntity &creature)
{
    if (rd_byte() != 0) {
        creature.set_action(ACTION_LEARN);
    }
}
