#pragma once

#include "dungeon/quest.h"
#include "system/angband.h"
#include <stdint.h>
#include <vector>

class CreatureEntity;
enum class QuestId : short;
void do_cmd_checkquest(CreatureEntity &creature);
void do_cmd_knowledge_quests_completed(CreatureEntity &creature, FILE *fff, const std::vector<QuestId> &quest_ids);
void do_cmd_knowledge_quests_failed(CreatureEntity &creature, FILE *fff, const std::vector<QuestId> &quest_ids);
void do_cmd_knowledge_quests(CreatureEntity &creature);
void do_cmd_knowledge_death_history(CreatureEntity &creature);
