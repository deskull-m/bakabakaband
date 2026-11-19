#pragma once

#include "system/enums/monrace/monrace-id.h"
#include <string_view>

constexpr auto DAMAGE_FORCE = 1;
constexpr auto DAMAGE_GENO = 2;
constexpr auto DAMAGE_LOSELIFE = 3;
constexpr auto DAMAGE_ATTACK = 4;
constexpr auto DAMAGE_NOESCAPE = 5;
constexpr auto DAMAGE_USELIFE = 6;

class CreatureEntity;
int take_hit(CreatureEntity &creature, int damage_type, int damage, std::string_view kb_str, MonraceId killer_monrace_id = static_cast<MonraceId>(0));
int acid_dam(CreatureEntity &creature, int dam, std::string_view kb_str, bool aura);
int elec_dam(CreatureEntity &creature, int dam, std::string_view kb_str, bool aura);
int fire_dam(CreatureEntity &creature, int dam, std::string_view kb_str, bool aura);
int cold_dam(CreatureEntity &creature, int dam, std::string_view kb_str, bool aura);
int dirt_dam(CreatureEntity &creature, int dam, std::string_view kb_str, bool aura);
void touch_zap_player(const CreatureEntity &source, CreatureEntity &creature);
void player_defecate(CreatureEntity &creature);
