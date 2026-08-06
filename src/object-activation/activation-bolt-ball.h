#pragma once

#include <string_view>

class CreatureEntity;
bool activate_missile_1(CreatureEntity &creature);
bool activate_missile_2(CreatureEntity &creature);
bool activate_missile_3(CreatureEntity &creature);
bool activate_bolt_acid_1(CreatureEntity &creature);
bool activate_bolt_elec_1(CreatureEntity &creature);
bool activate_bolt_fire_1(CreatureEntity &creature);
bool activate_bolt_cold_1(CreatureEntity &creature);
bool activate_bolt_hypodynamia_1(CreatureEntity &creature, std::string_view name);
bool activate_bolt_hypodynamia_2(CreatureEntity &creature);
bool activate_bolt_drain_1(CreatureEntity &creature);
bool activate_bolt_drain_2(CreatureEntity &creature);
bool activate_bolt_mana(CreatureEntity &creature, std::string_view name);
bool activate_ball_pois_1(CreatureEntity &creature);
bool activate_ball_cold_1(CreatureEntity &creature);
bool activate_ball_cold_2(CreatureEntity &creature);
bool activate_ball_cold_3(CreatureEntity &creature);
bool activate_ball_fire_1(CreatureEntity &creature);
bool activate_ball_fire_2(CreatureEntity &creature, std::string_view name);
bool activate_ball_fire_3(CreatureEntity &creature);
bool activate_ball_fire_4(CreatureEntity &creature);
bool activate_ball_elec_2(CreatureEntity &creature);
bool activate_ball_elec_3(CreatureEntity &creature);
bool activate_ball_acid_1(CreatureEntity &creature);
bool activate_ball_nuke_1(CreatureEntity &creature);
bool activate_rocket(CreatureEntity &creature);
bool activate_ball_water(CreatureEntity &creature, std::string_view name);
bool activate_ball_lite(CreatureEntity &creature, std::string_view name);
bool activate_ball_dark(CreatureEntity &creature, std::string_view name);
bool activate_ball_mana(CreatureEntity &creature, std::string_view name);
