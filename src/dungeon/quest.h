#pragma once

#include "system/angband.h"
#include "system/dungeon/quest-definition.h"
#include <string>
#include <vector>

enum class QuestId : short;
extern std::vector<std::string> quest_text_lines;
extern QuestId leaving_quest;

constexpr auto QUEST_TEST_LINES_MAX = 10;

enum class QuestStatusType : short;
class CreatureEntity;
class FloorType;
class ItemEntity;
class QuestType;
void determine_random_questor(CreatureEntity &creature, QuestType &quest);
void record_quest_final_status(QuestType *q_ptr, PLAYER_LEVEL lev, QuestStatusType stat);
void complete_quest(CreatureEntity &creature, QuestId quest_num);
void check_find_art_quest_completion(CreatureEntity &creature, ItemEntity *o_ptr);
void quest_discovery(QuestId quest_id);
void leave_quest_check(CreatureEntity &creature);
void leave_tower_check(CreatureEntity &creature);
void exe_enter_quest(CreatureEntity &creature, QuestId quest_id);
void do_cmd_quest(CreatureEntity &creature);
bool inside_quest(QuestId quest_id);
