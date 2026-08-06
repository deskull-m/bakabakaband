#include "load/load-zangband.h"
#include "avatar/avatar.h"
#include "floor/dungeon-feeling.h"
#include "game-option/option-flags.h"
#include "info-reader/fixed-map-parser.h"
#include "load/angband-version-comparer.h"
#include "load/load-util.h"
#include "player/attack-defense-types.h"
#include "player/patron.h"
#include "player/player-personality.h"
#include "player/player-realm.h"
#include "player/player-skill.h"
#include "player/player-spell-status.h"
#include "realm/realm-types.h"
#include "spell/spells-status.h"
#include "system/building-type-definition.h"
#include "system/creature-entity.h"
#include "system/dungeon/dungeon-record.h"
#include "system/dungeon/quest-definition.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/floor/floor-info.h"
#include "system/inner-game-data.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "system/system-variables.h"
#include "world/world.h"

void set_zangband_action(CreatureEntity &creature)
{
    if (rd_byte() != 0) {
        creature.set_action(ACTION_LEARN);
    }
}
