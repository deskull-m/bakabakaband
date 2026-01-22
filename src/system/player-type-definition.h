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

enum class INCIDENT {
    WALK = 0,
    EAT = 1,
    QUAFF = 2,
    ATTACK_ACT_COUNT = 3,
    ATTACK_EXE_COUNT = 4,
    SHOOT = 5,
    THROW = 6,
    LEAVE_FLOOR = 7,
    TRAPPED = 8,
    READ_SCROLL = 9,
    ZAP_STAFF = 10,
    ZAP_WAND = 11,
    ZAP_ROD = 12,
    STORE_BUY = 13,
    STORE_SELL = 14,
    STAY_INN = 15,
    EAT_FECES = 100,
    EAT_POISON = 101,
};

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

    RealmType realm1{}; /* First magic realm */
    RealmType realm2{}; /* Second magic realm */
    ElementRealmType element_realm{}; //!< 元素使い領域

    Dice hit_dice{}; /* Hit dice */
    uint16_t expfact{}; /* Experience factor
                         * Note: was byte, causing overflow for Amberite
                         * characters (such as Amberite Paladins)
                         */

    std::map<INCIDENT, int32_t> incident{}; /*!< これまでに行った出来事カウント（従来型、enumベース） */

    PERCENTAGE mutant_regenerate_mod{};

    int16_t max_plv{}; /* Max Player Level */

    int16_t learned_spells{};
    int16_t add_spells{};

    uint32_t count{};

    bool timewalk{};

#define COMMAND_ARG_REST_UNTIL_DONE -2 /*!<休憩コマンド引数 … 必要な分だけ回復 */
#define COMMAND_ARG_REST_FULL_HEALING -1 /*!<休憩コマンド引数 … HPとMPが全回復するまで */
    GAME_TURN resting{}; /* Current counter for resting, if any */

    TIME_EFFECT word_recall{}; /* Word of recall counter */
    TIME_EFFECT alter_reality{}; /* Alter reality counter */
    DungeonId recall_dungeon{}; /* Dungeon set to be recalled */

    ENERGY enchant_energy_need{}; /* Energy needed for next upkeep effect	 */

    /*
     * p_ptr->special_attackによるプレイヤーの攻撃状態の定義 / Bit flags for the "p_ptr->special_attack" variable. -LM-
     *
     * Note:  The elemental and poison attacks should be managed using the
     * function "set_ele_attack", in spell2.c.  This provides for timeouts and
     * prevents the player from getting more than one at a time.
     */
    BIT_FLAGS special_attack{};

    BIT_FLAGS spell_learned1{}; /* bit mask of spells learned */
    BIT_FLAGS spell_learned2{}; /* bit mask of spells learned */
    BIT_FLAGS spell_worked1{}; /* bit mask of spells tried and worked */
    BIT_FLAGS spell_worked2{}; /* bit mask of spells tried and worked */
    BIT_FLAGS spell_forgotten1{}; /* bit mask of spells learned but forgotten */
    BIT_FLAGS spell_forgotten2{}; /* bit mask of spells learned but forgotten */
    std::vector<int> spell_order_learned{}; /* order spells learned */

    SUB_EXP spell_exp[64]{}; /* Proficiency of spells */
    std::map<ItemKindType, std::array<SUB_EXP, 64>> weapon_exp{}; /* Proficiency of weapons */
    std::map<ItemKindType, std::array<SUB_EXP, 64>> weapon_exp_max{}; /* Maximum proficiency of weapons */
    std::map<PlayerSkillKindType, SUB_EXP> skill_exp{}; /* Proficiency of misc. skill */
    MartialArtsStyleType martial_arts_style{ MartialArtsStyleType::TRADITIONAL }; /* Martial arts fighting style */

    int player_hp[PY_MAX_LEVEL]{};
    std::string last_message = ""; /* Last message on death or retirement */
    char history[4][60]{}; /* Textual "history" for the Player */

    bool is_dead_{}; /* Player is dead */
    bool now_damaged{};

#define KNOW_STAT 0x01
#define KNOW_HPRATE 0x02
    BIT_FLAGS8 knowledge{}; /* Knowledge about yourself */
    BIT_FLAGS visit{}; /* Visited towns */

    BIT_FLAGS old_race1{}; /* Record of race changes */
    BIT_FLAGS old_race2{}; /* Record of race changes */
    int16_t old_realm{}; /* Record of realm changes */

    int16_t pet_follow_distance{}; /* Length of the imaginary "leash" for pets */
    BIT_FLAGS16 pet_extra_flags{}; /* Various flags for controling pets */

    bool dtrap{}; /* Whether you are on trap-safe grids */
    FLOOR_IDX floor_id{}; /* Current floor location */

    bool autopick_autoregister{}; /* auto register is in-use or not */

    /*** Temporary fields ***/

    bool select_ring_slot{};

    bool playing{}; /* True if player is playing */
    bool leaving{}; /* True if player is leaving */
    bool vanish_stairs_flag{}; /* True if stairs should vanish after floor change */

    bool monk_notify_aux{};

    bool teleport_town{};

    IDX health_who{}; /* Health bar trackee */

    short tracking_bi_id{}; /* Object kind trackee */

    int16_t new_spells{}; /* Number of spells available */
    int16_t old_spells{};

    int16_t old_food_aux{}; /* Old value of food */

    bool old_cumber_armor{};
    bool old_cumber_glove{};
    bool old_heavy_wield[2]{};
    bool old_heavy_shoot{};
    bool old_icky_wield[2]{};
    bool old_riding_wield[2]{};
    bool old_riding_ryoute{};
    bool old_monlite{};
    int extra_blows[2]{};

    bool cumber_armor{}; /* Mana draining armor */
    bool cumber_glove{}; /* Mana draining gloves */
    bool heavy_wield[2]{}; /* Heavy weapon */
    bool is_icky_wield[2]{}; /* クラスにふさわしくない装備をしている / Icky weapon */
    bool is_icky_riding_wield[2]{}; /* 乗馬中に乗馬にふさわしくない装備をしている / Riding weapon */
    bool riding_ryoute{}; /* Riding weapon */
    bool monlite{};
    BIT_FLAGS yoiyami{};
    BIT_FLAGS easy_2weapon{};
    BIT_FLAGS down_saving{};

    bool sutemi{};
    bool counter{};

    DIRECTION fishing_dir{};

    MONSTER_IDX pet_t_m_idx{};
    MONSTER_IDX riding_t_m_idx{};

    /*** Extracted fields ***/

    bool suppress_multi_reward{}; /*!< 複数レベルアップ時のパトロンからの報酬多重受け取りを防止 */

    int16_t stat_add[A_MAX]{}; /* Modifiers to stat values */
    int16_t stat_index[A_MAX]{}; /* Indexes into stat tables */

    Dice damage_dice_bonus[2]{}; /* Extra damage dice num/sides */

    bool no_flowed{};

    byte tval_xtra{}; /* (Unused)Correct xtra tval */
    ItemKindType tval_ammo{}; /* Correct ammo tval */

    ENERGY energy_use{}; /*!< 直近のターンに消費したエネルギー / Energy use this turn */

    std::string base_name{}; /*!< Stripped version of "player_name" */

    void ride_monster(MONSTER_IDX m_idx);
    bool is_fully_healthy() const;
    std::string decrease_ability_random();
    std::string decrease_ability_all();
    bool is_located_at_running_destination() const;
    bool is_located_at(const Pos2D &pos) const override;
    bool try_set_position(const Pos2D &pos);
    void set_position(const Pos2D &pos);
    bool in_saved_floor() const;
    int calc_life_rating() const;
    bool try_resist_eldritch_horror() const;

    // CreatureEntityインターフェースの実装
    int get_current_hp() const override;
    int get_max_hp() const override;

    bool is_valid() const override;
    bool is_dead() const override;

    bool is_player() const override;
};

extern PlayerType *p_ptr;
