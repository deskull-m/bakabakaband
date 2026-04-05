#pragma once

#include "system/creature-entity.h"
#include "system/monster-profile.h"
#include "util/point-2d.h"
#include <string>
#include <string_view>

/*!
 * @brief Monster information, for a specific monster.
 * @Note
 * fy, fx constrain dungeon size to 256x256
 * The "hold_o_idx" field points to the first object of a stack
 * of objects (if any) being carried by the monster (see above).
 */
constexpr int MONSTER_MAXHP = 10000000; //!< モンスターの最大HP

enum class MonraceId : int16_t;
enum class PlayerRaceType;
enum class PlayerClassType : short;
class FloorType;
class MonsterEntityWriter;
class MonsterEntity : public CreatureEntity {
public:
    friend class MonsterEntityWriter;
    MonsterEntity();
    MonsterEntity(MonsterEntity &&) = default;
    MonsterEntity &operator=(MonsterEntity &&) = default;
    MonsterEntity(const MonsterEntity &) = default;
    MonsterEntity &operator=(const MonsterEntity &) = default;

    static bool check_sub_alignments(const byte sub_align1, const byte sub_align2);

    void wipe();
    MonsterEntity clone() const;
    bool is_hostile_to_melee(const CreatureEntity &other) const override;
    bool is_hostile_align(const byte other_sub_align) const;
    bool is_mimicry() const;
    bool is_decelerated() const override;
    bool is_stunned() const override;
    bool is_confused() const override;
    bool is_fearful() const override;
    bool is_invulnerable() const override;
    short get_timed_effect(CreatureTimedEffect effect) const override;
    void set_timed_effect(CreatureTimedEffect effect, short value) override;
    int get_speed() const override;
    std::string get_pronoun_of_summoned_kin() const;
    tl::optional<bool> order_pet_whistle(const CreatureEntity &other) const;
    tl::optional<bool> order_pet_dismission(const CreatureEntity &other) const;
    Pos2D get_position() const override;
    bool can_ring_boss_call_nazgul() const;
    std::string build_looking_description(bool needs_attitude) const;
    int get_ac() const override;
    void on_take_hit(int damage) override;
    void on_death(std::string_view cause) override;

    void set_individual_speed(bool force_fixed_speed);
    void set_hostile() override;
    void make_lore_treasure(int num_item, int num_gold) const override;
    void reset_chameleon_polymorph() override;
    void set_friendly();
    void initialize_equivalent_player_races();
    void initialize_equivalent_player_classes();

    // CreatureEntityインターフェースの実装
    POSITION get_x() const override;
    POSITION get_y() const override;
    int get_current_hp() const override;
    int get_max_hp() const override;

    bool is_valid() const override;
    bool is_dead() const override;

    int get_level() const override;
    bool is_player() const override;

private:
    tl::optional<bool> order_pet_named(const CreatureEntity &other) const;
    tl::optional<bool> order_pet_hp(const CreatureEntity &other) const;
    std::string build_damage_description() const;
    std::string build_attitude_description() const;
};
