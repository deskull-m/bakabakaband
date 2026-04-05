#pragma once

#include "system/angband.h"
#include <string>

/*
 * Bit flags for the *_can_enter() and monster_can_cross_terrain()
 */
#define CEM_RIDING 0x0001
#define CEM_P_CAN_ENTER_PATTERN 0x0002
enum class MonraceId : int16_t;
enum class AllianceType : int;
class MonraceDefinition;
class CreatureEntity;
bool monster_can_cross_terrain(CreatureEntity *creature_ptr, FEAT_IDX feat, const MonraceDefinition &monrace, BIT_FLAGS16 mode);
bool monster_can_enter(CreatureEntity *creature_ptr, POSITION y, POSITION x, const MonraceDefinition &monrace, BIT_FLAGS16 mode);
bool monster_has_hostile_to_player(CreatureEntity &creature, int pa_good, int pa_evil, const MonraceDefinition &monrace);
bool monster_has_hostile_to_other_monster(const CreatureEntity &creature_other, const MonraceDefinition &monrace);
bool monster_has_hostile_to_other_monster(const CreatureEntity &creature_other, const MonraceDefinition &monrace, AllianceType alliance_id);
bool is_original_ap_and_seen(CreatureEntity &subject, const CreatureEntity &creature);
std::string monster_name(CreatureEntity &creature, MONSTER_IDX m_idx);
