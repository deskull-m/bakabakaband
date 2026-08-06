#pragma once

#include "system/angband.h"

enum rate_calc_type_mode {
    CALC_RAND = 0,
    CALC_AVERAGE = 1,
    CALC_MIN = 2,
    CALC_MAX = 3,
};

class CreatureEntity;
PERCENTAGE calc_acid_damage_rate(CreatureEntity &creature);
PERCENTAGE calc_elec_damage_rate(CreatureEntity &creature);
PERCENTAGE calc_fire_damage_rate(CreatureEntity &creature);
PERCENTAGE calc_cold_damage_rate(CreatureEntity &creature);
PERCENTAGE calc_pois_damage_rate(CreatureEntity &creature);
PERCENTAGE calc_nuke_damage_rate(CreatureEntity &creature);
PERCENTAGE calc_lite_damage_rate(CreatureEntity &creature, rate_calc_type_mode mode);
PERCENTAGE calc_dark_damage_rate(CreatureEntity &creature, rate_calc_type_mode mode);
PERCENTAGE calc_shards_damage_rate(CreatureEntity &creature, rate_calc_type_mode mode);
PERCENTAGE calc_sound_damage_rate(CreatureEntity &creature, rate_calc_type_mode mode);
PERCENTAGE calc_conf_damage_rate(CreatureEntity &creature, rate_calc_type_mode mode);
PERCENTAGE calc_chaos_damage_rate(CreatureEntity &creature, rate_calc_type_mode mode);
PERCENTAGE calc_nether_damage_rate(CreatureEntity &creature, rate_calc_type_mode mode);
PERCENTAGE calc_disenchant_damage_rate(CreatureEntity &creature, rate_calc_type_mode mode);
PERCENTAGE calc_nexus_damage_rate(CreatureEntity &creature, rate_calc_type_mode mode);
PERCENTAGE calc_time_damage_rate(CreatureEntity &creature, rate_calc_type_mode mode);
PERCENTAGE calc_water_damage_rate(CreatureEntity &creature, rate_calc_type_mode mode);
PERCENTAGE calc_rocket_damage_rate(CreatureEntity &creature, rate_calc_type_mode mode);
PERCENTAGE calc_deathray_damage_rate(CreatureEntity &creature, rate_calc_type_mode mode);
PERCENTAGE calc_holy_fire_damage_rate(CreatureEntity &creature, rate_calc_type_mode mode);
PERCENTAGE calc_hell_fire_damage_rate(CreatureEntity &creature, rate_calc_type_mode mode);
PERCENTAGE calc_gravity_damage_rate(CreatureEntity &creature, rate_calc_type_mode mode);
PERCENTAGE calc_void_damage_rate(CreatureEntity &creature, rate_calc_type_mode mode);
PERCENTAGE calc_abyss_damage_rate(CreatureEntity &creature, rate_calc_type_mode mode);
PERCENTAGE calc_chaos_damage_rate_rand(CreatureEntity &creature);
PERCENTAGE calc_plasma_damage_rate(CreatureEntity &creature);
PERCENTAGE calc_void_damage_rate_rand(CreatureEntity &creature);
