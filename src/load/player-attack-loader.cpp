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
    // ELE_ATTACK は v53 以降 rd_creature_common() の全時限ダンプで読込済み
    if (loading_savefile_version_is_older_than(53)) {
        creature.set_timed_effect(CreatureTimedEffect::ELE_ATTACK, rd_s16b());
    }
    creature.set_special_attack_flags(rd_u32b());
}

void rd_special_action(CreatureEntity &creature)
{
    if (!CreatureClass(creature).monk_stance_is(MonkStanceType::NONE)) {
        creature.set_action(ACTION_MONK_STANCE);
        return;
    }

    if (!CreatureClass(creature).samurai_stance_is(SamuraiStanceType::NONE)) {
        creature.set_action(ACTION_SAMURAI_STANCE);
    }
}

void rd_special_defense(CreatureEntity &creature)
{
    // ELE_IMMUNE は v53 以降 rd_creature_common() の全時限ダンプで読込済み
    if (loading_savefile_version_is_older_than(53)) {
        creature.set_timed_effect(CreatureTimedEffect::ELE_IMMUNE, rd_s16b());
    }
    creature.set_special_defense_flags(rd_u32b());
}

void rd_action(CreatureEntity &creature)
{
    strip_bytes(1);
    creature.set_action(rd_byte());
    set_zangband_action(creature);
}
