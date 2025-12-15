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
    void plus_incident_tree(const std::string &incident_id, int num);
    bool is_true_winner() const;

    FloorType *current_floor_ptr{};
    POSITION oldpy{}; /* Previous player location -KMW- */
    POSITION oldpx{}; /* Previous player location -KMW- */

    player_sex psex{}; /* Sex index */
    PlayerRaceType prace{}; /* Race index */
    PlayerClassType pclass{}; /* Class index */
    player_personality_type ppersonality{}; /* Personality index */
    RealmType realm1{}; /* First magic realm */
    RealmType realm2{}; /* Second magic realm */
    ElementRealmType element_realm{}; //!< 元素使い領域

    Dice hit_dice{}; /* Hit dice */
    uint16_t expfact{}; /* Experience factor
                         * Note: was byte, causing overflow for Amberite
                         * characters (such as Amberite Paladins)
                         */

    std::map<INCIDENT, int32_t> incident{}; /*!< これまでに行った出来事カウント（従来型、enumベース） */
    std::map<std::string, int32_t> incident_tree{}; /*!< ツリー構造ID（例: "root/attack/critical"）で記録するインシデントカウント */

    PRICE au{}; /* Current Gold */

    EXP max_max_exp{}; /* Max max experience (only to calculate score) */
    EXP max_exp{}; /* Max experience */
    EXP exp{}; /* Cur experience */
    uint32_t exp_frac{}; /* Cur exp frac (times 2^16) */

    PLAYER_LEVEL lev{}; /* Level */

    int16_t town_num{}; /* Current town number */

    int mhp{}; /* Max hit pts */
    int chp{}; /* Cur hit pts */
    uint32_t chp_frac{}; /* Cur hit frac (times 2^16) */
    PERCENTAGE mutant_regenerate_mod{};

    MANA_POINT msp{}; /* Max mana pts */
    MANA_POINT csp{}; /* Cur mana pts */
    uint32_t csp_frac{}; /* Cur mana frac (times 2^16) */

    int16_t max_plv{}; /* Max Player Level */

    int16_t learned_spells{};
    int16_t add_spells{};

    uint32_t count{};

    TIME_EFFECT invuln{}; /* Timed -- Invulnerable */
    TIME_EFFECT ult_res{}; /* Timed -- Ultimate Resistance */
    TIME_EFFECT hero{}; /* Timed -- Heroism */
    TIME_EFFECT shero{}; /* Timed -- Super Heroism */
    TIME_EFFECT shield{}; /* Timed -- Shield Spell */
    TIME_EFFECT blessed{}; /* Timed -- Blessed */
    TIME_EFFECT tim_invis{}; /* Timed -- See Invisible */
    TIME_EFFECT tim_infra{}; /* Timed -- Infra Vision */
    TIME_EFFECT tsuyoshi{}; /* Timed -- Tsuyoshi Special */
    TIME_EFFECT ele_attack{}; /* Timed -- Elemental Attack */
    TIME_EFFECT ele_immune{}; /* Timed -- Elemental Immune */

    TIME_EFFECT oppose_acid{}; /* Timed -- oppose acid */
    TIME_EFFECT oppose_elec{}; /* Timed -- oppose lightning */
    TIME_EFFECT oppose_fire{}; /* Timed -- oppose heat */
    TIME_EFFECT oppose_cold{}; /* Timed -- oppose cold */
    TIME_EFFECT oppose_pois{}; /* Timed -- oppose poison */

    TIME_EFFECT tim_esp{}; /* Timed ESP */
    TIME_EFFECT wraith_form{}; /* Timed wraithform */

    TIME_EFFECT resist_magic{}; /* Timed Resist Magic (later) */
    TIME_EFFECT tim_regen{};
    TIME_EFFECT tim_pass_wall{};
    TIME_EFFECT tim_stealth{};
    TIME_EFFECT tim_levitation{};
    TIME_EFFECT tim_sh_touki{};
    TIME_EFFECT lightspeed{};
    TIME_EFFECT tsubureru{};
    TIME_EFFECT magicdef{};
    TIME_EFFECT tim_res_nether{}; /* Timed -- Nether resistance */
    TIME_EFFECT tim_res_lite{}; /* Timed -- Lite resistance */
    TIME_EFFECT tim_res_dark{}; /* Timed -- Dark resistance */
    TIME_EFFECT tim_res_fear{}; /* Timed -- Fear resistance */
    TIME_EFFECT tim_res_time{}; /* Timed -- Time resistance */
    MimicKindType mimic_form{};
    TIME_EFFECT tim_mimic{};
    TIME_EFFECT tim_sh_fire{};
    TIME_EFFECT tim_sh_holy{};
    TIME_EFFECT tim_eyeeye{};

    /* for mirror master */
    TIME_EFFECT tim_reflect{}; /* Timed -- Reflect */
    TIME_EFFECT multishadow{}; /* Timed -- Multi-shadow */
    TIME_EFFECT dustrobe{}; /* Timed -- Robe of dust */

    /* for crusade */
    TIME_EFFECT tim_emission{}; /* Timed -- Player Emission */
    TIME_EFFECT tim_exorcism{}; /* Timed -- Exorcism */
    TIME_EFFECT tim_imm_dark{}; /* Timed -- Darkness immunity */

    bool timewalk{};

#define COMMAND_ARG_REST_UNTIL_DONE -2 /*!<休憩コマンド引数 … 必要な分だけ回復 */
#define COMMAND_ARG_REST_FULL_HEALING -1 /*!<休憩コマンド引数 … HPとMPが全回復するまで */
    GAME_TURN resting{}; /* Current counter for resting, if any */

    int16_t patron{};

    EnumClassFlagGroup<PlayerMutationType> muta{}; /*!< 突然変異 / mutations */
    EnumClassFlagGroup<PlayerMutationType> trait{}; /*!< 後天特性 / permanent trait */

    int16_t virtues[8]{};
    Virtue vir_types[8]{};

    TIME_EFFECT word_recall{}; /* Word of recall counter */
    TIME_EFFECT alter_reality{}; /* Alter reality counter */
    DungeonId recall_dungeon{}; /* Dungeon set to be recalled */

    ENERGY energy_need{}; /* Energy needed for next move */
    ENERGY enchant_energy_need{}; /* Energy needed for next upkeep effect	 */

    int16_t food{}; /*!< ゲーム中の滋養度の型定義 / Current nutrition */

    /*
     * p_ptr->special_attackによるプレイヤーの攻撃状態の定義 / Bit flags for the "p_ptr->special_attack" variable. -LM-
     *
     * Note:  The elemental and poison attacks should be managed using the
     * function "set_ele_attack", in spell2.c.  This provides for timeouts and
     * prevents the player from getting more than one at a time.
     */
    BIT_FLAGS special_attack{};

    /* プレイヤーの防御状態の定義 / Bit flags for the "p_ptr->special_defense" variable. -LM- */
    BIT_FLAGS special_defense{};
    byte action{}; /*!< プレイヤーが現在取っている常時行動のID / Currently action */
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

    ClassSpecificData class_specific_data;

    int player_hp[PY_MAX_LEVEL]{};
    std::string died_from{}; /* What killed the player */
    MonraceId killer_monrace_id{}; /* MonraceId of the killer */
    std::string last_message = ""; /* Last message on death or retirement */
    char history[4][60]{}; /* Textual "history" for the Player */

    bool is_dead_{}; /* Player is dead */
    bool now_damaged{};
    bool ambush_flag{};

    MONSTER_IDX riding{}; /* Riding on a monster of this index */

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

    POSITION old_lite{}; /* Old radius of lite (if any) */

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

    POSITION cur_lite{}; /* Radius of lite (if any) */

    int16_t stat_use[A_MAX]{}; /* Current modified stats */
    int16_t stat_top[A_MAX]{}; /* Maximal modified stats */

    bool sutemi{};
    bool counter{};

    int alignment{}; /* Good/evil/neutral */
    POSITION run_py{};
    POSITION run_px{};
    DIRECTION fishing_dir{};

    MONSTER_IDX pet_t_m_idx{};
    MONSTER_IDX riding_t_m_idx{};

    /*** Extracted fields ***/

    int16_t running{}; /* Current counter for running, if any */
    bool suppress_multi_reward{}; /*!< 複数レベルアップ時のパトロンからの報酬多重受け取りを防止 */

    int16_t stat_add[A_MAX]{}; /* Modifiers to stat values */
    int16_t stat_index[A_MAX]{}; /* Indexes into stat tables */

    bool hack_mutation{};
    bool is_fired{};
    bool level_up_message{};

    BIT_FLAGS anti_magic{}; /* Anti-magic */
    BIT_FLAGS anti_tele{}; /* Prevent teleportation */

    EnumClassFlagGroup<CurseTraitType> cursed{}; /* Player is cursed */
    EnumClassFlagGroup<CurseSpecialTraitType> cursed_special{}; /* Player is special type cursed */

    bool can_swim{}; /* No damage falling */
    BIT_FLAGS levitation{}; /* No damage falling */
    BIT_FLAGS lite{}; /* Permanent light */
    BIT_FLAGS free_act{}; /* Never paralyzed */
    BIT_FLAGS see_inv{}; /* Can see invisible */
    BIT_FLAGS regenerate{}; /* Regenerate hit pts */
    BIT_FLAGS hold_exp{}; /* Resist exp draining */

    BIT_FLAGS telepathy{}; /* Telepathy */
    BIT_FLAGS esp_animal{};
    BIT_FLAGS esp_nasty{};
    BIT_FLAGS esp_homo{};
    BIT_FLAGS esp_undead{};
    BIT_FLAGS esp_demon{};
    BIT_FLAGS esp_orc{};
    BIT_FLAGS esp_troll{};
    BIT_FLAGS esp_giant{};
    BIT_FLAGS esp_dragon{};
    BIT_FLAGS esp_human{};
    BIT_FLAGS esp_evil{};
    BIT_FLAGS esp_good{};
    BIT_FLAGS esp_nonliving{};
    BIT_FLAGS esp_unique{};

    BIT_FLAGS slow_digest{}; /* Slower digestion */
    BIT_FLAGS bless_blade{}; //!< 祝福された装備をしている / Blessed by inventory items
    BIT_FLAGS xtra_might{}; /* Extra might bow */
    BIT_FLAGS impact{}; //!< クリティカル率を上げる装備をしている / Critical blows
    BIT_FLAGS earthquake{}; //!< 地震を起こす装備をしている / Earthquake blows
    BIT_FLAGS dec_mana{};
    BIT_FLAGS easy_spell{};
    BIT_FLAGS hard_spell{};
    BIT_FLAGS warning{};
    BIT_FLAGS mighty_throw{};
    BIT_FLAGS see_nocto{}; /* Noctovision */
    bool invoking_midnight_curse{};

    Dice damage_dice_bonus[2]{}; /* Extra damage dice num/sides */

    HIT_PROB dis_to_h[2]{}; /*!< 判明している現在の表記上の近接武器命中修正値 /  Known bonus to hit (wield) */
    HIT_PROB dis_to_h_b{}; /*!< 判明している現在の表記上の射撃武器命中修正値 / Known bonus to hit (bow) */
    int dis_to_d[2]{}; /*!< 判明している現在の表記上の近接武器ダメージ修正値 / Known bonus to dam (wield) */
    ARMOUR_CLASS dis_to_a{}; /*!< 判明している現在の表記上の装備AC修正値 / Known bonus to ac */
    ARMOUR_CLASS dis_ac{}; /*!< 判明している現在の表記上の装備AC基礎値 / Known base ac */

    int16_t to_h[2]{}; /* Bonus to hit (wield) */
    int16_t to_h_b{}; /* Bonus to hit (bow) */
    int16_t to_h_m{}; /* Bonus to hit (misc) */
    int16_t to_d[2]{}; /* Bonus to dam (wield) */
    int16_t to_d_m{}; /* Bonus to dam (misc) */
    ARMOUR_CLASS to_a{}; /* Bonus to ac */

    int16_t to_m_chance{}; /* Minusses to cast chance */

    bool no_flowed{};

    ARMOUR_CLASS ac{}; /*!< 装備無しの基本AC / Base ac */

    int16_t num_blow[2]{}; /* Number of blows */
    int16_t num_fire{}; /* Number of shots */

    byte tval_xtra{}; /* (Unused)Correct xtra tval */
    ItemKindType tval_ammo{}; /* Correct ammo tval */

    int16_t pspeed{}; /*!< 現在の速度 / Current speed */

    ENERGY energy_use{}; /*!< 直近のターンに消費したエネルギー / Energy use this turn */

    POSITION y{}; /*!< ダンジョンの現在Y座標 / Player location in dungeon */
    POSITION x{}; /*!< ダンジョンの現在X座標 / Player location in dungeon */
    GAME_TEXT name[32]{}; /*!< 現在のプレイヤー名 / Current player's character name */
    char base_name[32]{}; /*!< Stripped version of "player_name" */

    void ride_monster(MONSTER_IDX m_idx);
    std::shared_ptr<TimedEffects> effects() const;
    bool is_fully_healthy() const;
    bool is_wielding(FixedArtifactId fa_id) const;
    std::string decrease_ability_random();
    std::string decrease_ability_all();
    Pos2D get_position() const override;
    Pos2D get_old_position() const;
    Pos2D get_neighbor(int dir) const;
    Pos2D get_neighbor(const Direction &dir) const;
    bool is_located_at_running_destination() const;
    bool is_located_at(const Pos2D &pos) const;
    bool try_set_position(const Pos2D &pos);
    void set_position(const Pos2D &pos);
    bool in_saved_floor() const;
    int calc_life_rating() const;
    bool try_resist_eldritch_horror() const;

    // CreatureEntityインターフェースの実装
    POSITION get_x() const override;
    POSITION get_y() const override;
    int get_current_hp() const override;
    int get_max_hp() const override;
    int get_speed() const override;
    bool is_valid() const override;
    bool is_dead() const override;
    FloorType *get_floor() const override;
    ACTION_ENERGY get_energy_need() const override;
    void set_energy_need(ACTION_ENERGY energy) override;
    int get_level() const override;
    bool is_player() const override;

private:
    std::shared_ptr<TimedEffects> timed_effects;
};

extern PlayerType *p_ptr;
