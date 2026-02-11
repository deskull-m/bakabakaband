#pragma once

#include <string_view>

class CreatureEntity;
bool activate_resistance_elements(CreatureEntity &creature);
bool activate_resistance_acid(CreatureEntity &creature, std::string_view name);
bool activate_resistance_elec(CreatureEntity &creature, std::string_view name);
bool activate_resistance_fire(CreatureEntity &creature, std::string_view name);
bool activate_resistance_cold(CreatureEntity &creature, std::string_view name);
bool activate_resistance_pois(CreatureEntity &creature, std::string_view name);
bool activate_acid_ball_and_resistance(CreatureEntity &creature, std::string_view name);
bool activate_elec_ball_and_resistance(CreatureEntity &creature, std::string_view name);
bool activate_fire_ball_and_resistance(CreatureEntity &creature, std::string_view name);
bool activate_cold_ball_and_resistance(CreatureEntity &creature, std::string_view name);
bool activate_pois_ball_and_resistance(CreatureEntity &creature, std::string_view name);
bool activate_ultimate_resistance(CreatureEntity &creature);
