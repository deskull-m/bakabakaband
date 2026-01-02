#pragma once

#include "system/angband.h"

class CreatureEntity;
bool gain_mutation(CreatureEntity &creature, MUTATION_IDX choose_mut);
bool lose_mutation(CreatureEntity &creature, MUTATION_IDX choose_mut);
void lose_all_mutations(CreatureEntity &creature);
