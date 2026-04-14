#include "load/player-attack-loader.h"
#include "load/load-util.h"
#include "load/load-zangband.h"
#include "player-base/player-class.h"
#include "player-info/monk-data-type.h"
#include "player-info/samurai-data-type.h"
#include "player/attack-defense-types.h"
#include "player/special-defense-types.h"
#include "system/creature-entity.h"

void rd_special_attack(CreatureEntity &creature)
{
    creature.set_timed_effect(CreatureTimedEffect::ELE_ATTACK, rd_s16b());
    creature.special_attack = rd_u32b();
}

void rd_special_action(CreatureEntity &creature)
{
    if (!CreatureClass(creature).monk_stance_is(MonkStanceType::NONE)) {
        creature.action = ACTION_MONK_STANCE;
        return;
    }

    if (!CreatureClass(creature).samurai_stance_is(SamuraiStanceType::NONE)) {
        creature.action = ACTION_SAMURAI_STANCE;
    }
}

void rd_special_defense(CreatureEntity &creature)
{
    creature.set_timed_effect(CreatureTimedEffect::ELE_IMMUNE, rd_s16b());
    creature.special_defense = rd_u32b();
}

void rd_action(CreatureEntity &creature)
{
    strip_bytes(1);
    creature.action = rd_byte();
    set_zangband_action(creature);
}
