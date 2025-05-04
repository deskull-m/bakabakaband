/*!
 * @brief monster-processのための構造体群
 * @date 2020/03/07
 * @author Hourier
 */

#pragma once

#include "monster-floor/monster-movement-direction-list.h"
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-behavior-flags.h"
#include "monster-race/race-drop-flags.h"
#include "monster-race/race-feature-flags.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-kind-flags.h"
#include "monster-race/race-special-flags.h"
#include "system/angband.h"
#include "util/flag-group.h"
#include "util/point-2d.h"
#include <optional>

struct turn_flags {
    bool see_m;
    bool aware;
    bool is_riding_mon;
    bool do_turn;
    bool do_move;
    bool do_view;
    bool do_take;
    bool must_alter_to_move;

    bool did_open_door;
    bool did_bash_door;
    bool did_take_item;
    bool did_kill_item;
    bool did_move_body;
    bool did_pass_wall;
    bool did_kill_wall;
};

// @details ダミーIDが渡されるとオブジェクトが生焼けになるので、ヘッダ側で全て初期化しておく.
enum class MonraceId : short;
class MonraceDefinition;
class OldRaceFlags {
public:
    OldRaceFlags(MonraceId monrace_id);

    EnumClassFlagGroup<MonsterAbilityType> old_r_ability_flags{};
    EnumClassFlagGroup<MonsterBehaviorType> old_r_behavior_flags{};
    EnumClassFlagGroup<MonsterKindType> old_r_kind_flags{};
    EnumClassFlagGroup<MonsterResistanceType> old_r_resistance_flags{};
    EnumClassFlagGroup<MonsterDropType> old_r_drop_flags{};
    EnumClassFlagGroup<MonsterFeatureType> old_r_feature_flags{};
    EnumClassFlagGroup<MonsterSpecialType> old_r_special_flags{};

    byte old_r_blows0 = 0;
    byte old_r_blows1 = 0;
    byte old_r_blows2 = 0;
    byte old_r_blows3 = 0;

    byte old_r_cast_spell = 0;

    void update_lore_window_flag(const MonraceDefinition &monrace) const;
};

struct coordinate_candidate {
    Pos2D pos = { 0, 0 };
    int gdis = 0;
};

turn_flags *init_turn_flags(bool is_riding, turn_flags *turn_flags_ptr);

class Direction;
MonsterMovementDirectionList get_enemy_approch_direction(MONSTER_IDX m_idx, const Pos2DVec &vec);
std::optional<MonsterMovementDirectionList> get_moves_val(MONSTER_IDX m_idx, const Pos2DVec &vec);
