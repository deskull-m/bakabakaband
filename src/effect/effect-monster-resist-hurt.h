#pragma once

#include "system/angband.h"

class EffectMonster;
class CreatureEntity;
ProcessResult effect_monster_nothing(EffectMonster *em_ptr);
ProcessResult effect_monster_acid(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_elec(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_fire(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_cold(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_pois(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_nuke(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_hell_fire(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_holy_fire(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_plasma(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_nether(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_water(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_chaos(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_shards(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_rocket(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_sound(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_confusion(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_disenchant(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_nexus(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_force(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_inertial(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_time(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_gravity(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_disintegration(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_icee_bolt(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_dirt(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_spider_string(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_stungun(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_void(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_abyss(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_meteor(CreatureEntity &creature, EffectMonster *em_ptr);
