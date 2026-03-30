#pragma once

#include "combat/martial-arts-style.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/trc-types.h"
#include "player-ability/player-ability-types.h"
#include "player-info/class-specific-data.h"
#include "player-info/class-types.h"
#include "player-info/race-types.h"
#include "player/player-personality-types.h"
#include "player/player-sex.h"
#include "system/angband.h"
#include "system/creature-entity.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/system-variables.h"
#include "util/dice.h"
#include "util/flag-group.h"
#include "util/point-2d.h"
#include <array>
#include <map>
#include <string>

// Forward declarations
struct player_race_info;
struct player_personality;
struct player_class_info;

enum class DungeonId;
enum class ElementRealmType;
enum class FixedArtifactId : short;
enum class ItemKindType : short;
enum class MimicKindType;
enum class MonraceId : int16_t;
enum class MonsterAbilityType;
enum class PlayerSkillKindType;
enum class RealmType;
enum class Virtue : short;
class Direction;
class FloorType;
class ItemEntity;
class TimedEffects;
class PlayerType : public CreatureEntity {
public:
    PlayerType();
    void plus_incident(INCIDENT incidentID, int num);
    bool is_true_winner() const;

    void ride_monster(MONSTER_IDX m_idx);
    bool is_fully_healthy() const;
    bool is_located_at_running_destination() const;
    bool is_located_at(const Pos2D &pos) const override;
    bool try_set_position(const Pos2D &pos);
    void set_position(const Pos2D &pos);
    bool in_saved_floor() const;
    bool try_resist_eldritch_horror() const;

    // CreatureEntityインターフェースの実装
    int get_current_hp() const override;
    int get_max_hp() const override;

    bool is_valid() const override;
    bool is_dead() const override;

    bool is_player() const override;

    short get_timed_effect(CreatureTimedEffect effect) const override;

    bool is_confused() const override;
    bool is_stunned() const override;
    bool is_fearful() const override;
    bool is_invulnerable() const override;
    bool is_blind() const override;
    bool is_paralyzed() const override;
    bool is_fast() const override;
};

extern PlayerType *p_ptr;
