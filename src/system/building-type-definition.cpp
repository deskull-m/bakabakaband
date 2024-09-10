#include "system/building-type-definition.h"

std::array<building_type, MAX_BUILDINGS> buildings;
MonsterRaceId battle_mon_list[4];
uint32_t mon_odds[4];
int battle_odds;
int wager_melee;
int bet_number;

MeleeArena MeleeArena::instance{};

MeleeArena &MeleeArena::get_instance()
{
    return instance;
}

MeleeGladiator &MeleeArena::get_gladiator(int n)
{
    return this->gladiators.at(n);
}

const MeleeGladiator &MeleeArena::get_gladiator(int n) const
{
    return this->gladiators.at(n);
}

void MeleeArena::set_gladiator(int n, const MeleeGladiator &gladiator)
{
    this->gladiators[n] = gladiator;
}
