#include "load/old/load-v1-7-0.h"
#include "dungeon/quest.h"
#include "game-option/birth-options.h"
#include "load/load-util.h"
#include "load/old/load-v1-5-0.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/floor/floor-info.h"

void set_exp_frac_old(CreatureEntity &creature)
{
    creature.exp_frac = rd_u16b();
}

void remove_water_cave(CreatureEntity &creature)
{
    auto &floor = *creature.get_floor();
    if (floor.quest_number != i2enum<QuestId>(OLD_QUEST_WATER_CAVE)) {
        return;
    }

    floor.set_dungeon_index(lite_town ? DungeonId::ANGBAND : DungeonId::GALGALS);
    floor.dun_level = 1;
    floor.quest_number = QuestId::NONE;
}
