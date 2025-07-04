#include "load/old/load-v1-7-0.h"
#include "dungeon/quest.h"
#include "game-option/birth-options.h"
#include "load/load-util.h"
#include "load/old/load-v1-5-0.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/floor/floor-info.h"
#include "system/player-type-definition.h"

void set_exp_frac_old(PlayerType *player_ptr)
{
    player_ptr->exp_frac = rd_u16b();
}

void remove_water_cave(PlayerType *player_ptr)
{
    auto &floor = *player_ptr->current_floor_ptr;
    if (floor.quest_number != i2enum<QuestId>(OLD_QUEST_WATER_CAVE)) {
        return;
    }

    floor.set_dungeon_index(lite_town ? DungeonId::ANGBAND : DungeonId::GALGALS);
    floor.dun_level = 1;
    floor.quest_number = QuestId::NONE;
}
