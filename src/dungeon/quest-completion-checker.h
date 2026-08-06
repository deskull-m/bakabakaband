#pragma once

#include "dungeon/quest.h"
#include "system/angband.h"
#include "system/creature-entity.h"
#include "util/point-2d.h"
#include <tuple>

class ItemEntity;
class QuestType;
class QuestCompletionChecker {
public:
    QuestCompletionChecker(CreatureEntity &creature, const CreatureEntity &monster);
    virtual ~QuestCompletionChecker() = default;

    void complete();

private:
    CreatureEntity *creature_ptr;
    const CreatureEntity *m_ptr;
    QuestId quest_idx;
    QuestType *q_ptr = nullptr;

    void set_quest_idx();
    std::tuple<bool, bool> switch_completion();
    void complete_kill_number();
    void complete_kill_all();
    std::tuple<bool, bool> complete_random();
    void complete_kill_any_level();
    void complete_tower();
    int count_all_hostile_monsters();
    Pos2D make_stairs(const bool create_stairs);
    void make_reward(const Pos2D pos);
    bool check_quality(ItemEntity &item);
};
