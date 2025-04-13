#pragma once

#include "alliance/alliance.h"
#include "locale/localized-string.h"
#include "monster-attack/monster-attack-effect.h"
#include "monster-attack/monster-attack-table.h"
#include "monster-race/monster-aura-types.h"
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-behavior-flags.h"
#include "monster-race/race-brightness-flags.h"
#include "monster-race/race-drop-flags.h"
#include "monster-race/race-feature-flags.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-kind-flags.h"
#include "monster-race/race-misc-flags.h"
#include "monster-race/race-population-flags.h"
#include "monster-race/race-sex-const.h"
#include "monster-race/race-speak-flags.h"
#include "monster-race/race-special-flags.h"
#include "monster-race/race-visual-flags.h"
#include "monster-race/race-wilderness-flags.h"
#include "system/angband.h"
#include "util/dice.h"
#include "util/flag-group.h"
#include "view/display-symbol.h"
#include <string>
#include <string_view>
#include <vector>

/*! モンスターが1ターンに攻撃する最大回数 (射撃を含む) / The maximum number of times a monster can attack in a turn (including SHOOT) */
constexpr int MAX_NUM_BLOWS = 4;

enum class FixedArtifactId : short;
enum class MonraceId : int16_t;

class DropArtifact {
public:
    DropArtifact(FixedArtifactId fa_id, int chance);
    FixedArtifactId fa_id;
    int chance; //!< ドロップ確率 (%)
};

class MonsterBlow {
public:
    RaceBlowMethodType method{};
    RaceBlowEffectType effect{};
    Dice damage_dice;
};

class MonraceDefinition;
class Reinforce {
public:
    Reinforce(MonraceId monrace_id, Dice dice);
    MonraceId get_monrace_id() const;
    bool is_valid() const;
    const MonraceDefinition &get_monrace() const;
    std::string get_dice_as_string() const;
    int roll_dice() const;
    int roll_max_dice() const;

private:
    MonraceId monrace_id;
    Dice dice;
};

/*!
 * @brief モンスター種族の定義構造体
 * @details
 * Monster "race" information, including racial memories
 *
 * Note that "d_attr" and "d_char" are used for MORE than "visual" stuff.
 *
 * Note that "x_attr" and "x_char" are used ONLY for "visual" stuff.
 *
 * Note that "cur_num" (and "max_num") represent the number of monsters
 * of the given race currently on (and allowed on) the current level.
 * This information yields the "dead" flag for Unique monsters.
 *
 * Note that "max_num" is reset when a new player is created.
 * Note that "cur_num" is reset when a new level is created.
 *
 * Note that several of these fields, related to "recall", can be
 * scrapped if space becomes an issue, resulting in less "complete"
 * monster recall (no knowledge of spells, etc).  All of the "recall"
 * fields have a special prefix to aid in searching for them.
 */
enum class DungeonId;
enum class GridFlow : int;
class MonraceDefinition {
public:
    MonraceDefinition();

    MonraceId idx{};
    LocalizedString name{}; //!< モンスターの名称
    std::string text = ""; //!< 思い出テキストのオフセット / Lore text offset
    Dice hit_dice; //!< HPのダイス / Creatures hit dice
    ARMOUR_CLASS ac{}; //!< アーマークラス / Armour Class
    SLEEP_DEGREE sleep{}; //!< 睡眠値 / Inactive counter (base)
    POSITION aaf{}; //!< 感知範囲(1-100スクエア) / Area affect radius (1-100)
    byte speed{}; //!< 加速(110で+0) / Speed (normally 110)
    EXP mexp{}; //!< 殺害時基本経験値 / Exp value for kill
    RARITY freq_spell{}; //!< 魔法＆特殊能力仕様頻度(1/n) /  Spell frequency
    MonsterSex sex{}; //!< 性別 / Sex
    EnumClassFlagGroup<MonsterFeedType> meat_feed_flags;
    EnumClassFlagGroup<MonsterAbilityType> ability_flags; //!< 能力フラグ(魔法/ブレス) / Ability Flags
    EnumClassFlagGroup<MonsterAuraType> aura_flags; //!< オーラフラグ / Aura Flags
    EnumClassFlagGroup<MonsterBehaviorType> behavior_flags; //!< 能力フラグ（習性）
    EnumClassFlagGroup<MonsterVisualType> visual_flags; //!< 能力フラグ（シンボル） / Symbol Flags
    EnumClassFlagGroup<MonsterKindType> kind_flags; //!< 能力フラグ（種族・徳） / Attr Flags
    EnumClassFlagGroup<MonsterResistanceType> resistance_flags; //!< 耐性フラグ / Flags R (resistances info)
    EnumClassFlagGroup<MonsterDropType> drop_flags; //!< 能力フラグ（ドロップ） / Drop Flags
    EnumClassFlagGroup<MonsterWildernessType> wilderness_flags; //!< 荒野フラグ / Wilderness Flags
    EnumClassFlagGroup<MonsterFeatureType> feature_flags; //!< 能力フラグ（地形関連） / Feature Flags
    EnumClassFlagGroup<MonsterPopulationType> population_flags; //!< 能力フラグ（出現数関連） / Population Flags
    EnumClassFlagGroup<MonsterSpeakType> speak_flags; //!< 能力フラグ（セリフ） / Speaking Flags
    EnumClassFlagGroup<MonsterBrightnessType> brightness_flags; //!< 能力フラグ（明暗） / Speaking Lite or Dark
    EnumClassFlagGroup<MonsterSpecialType> special_flags; //!< 能力フラグ(特殊) / Special Flags
    EnumClassFlagGroup<MonsterMiscType> misc_flags; //!< 能力フラグ（その他） / Speaking Other
    MonsterBlow blows[MAX_NUM_BLOWS]{}; //!< 打撃能力定義 / Up to four blows per round
    Dice shoot_damage_dice; //!< 射撃ダメージダイス / shoot damage dice

    std::vector<std::tuple<int, int, MonraceId>> spawn_monsters; //!< 落とし子生成率
    std::vector<std::tuple<int, int, FEAT_IDX>> change_feats; //!< 地形変化率
    std::vector<std::tuple<int, int, short>> spawn_items; //!< アイテム自然生成率
    std::vector<std::tuple<int, int, short, int, int, int>> drop_kinds; //!< アイテム特定ドロップ指定
    std::vector<std::tuple<int, int, short, int, int, int>> drop_tvals; //!< アイテム種別ドロップ指定
    std::vector<std::tuple<int, int, MonraceId, int, int>> dead_spawns; //!< 死亡時モンスター生成

    //! 特定アーティファクトドロップリスト <アーティファクトID,ドロップ率>
    std::vector<std::tuple<FixedArtifactId, PERCENTAGE>> drop_artifacts;
    int suicide_dice_num{}; //!< 自滅ターンダイス数
    int suicide_dice_side{}; //!< 自滅ターン面数
    PERCENTAGE arena_ratio{}; //!< モンスター闘技場の掛け金倍率修正値(%基準 / 0=100%) / The adjustment ratio for gambling monster
    MonraceId next_r_idx{}; //!< 進化先モンスター種族ID
    EXP next_exp{}; //!< 進化に必要な経験値
    DEPTH level{}; //!< レベル / Level of creature
    RARITY rarity{}; //!< レアリティ / Rarity of creature
    DisplaySymbol symbol_definition{}; //!< 定義上のシンボル (色/文字).
    DisplaySymbol symbol_config{}; //!< 設定したシンボル (色/文字).
    MONSTER_NUMBER max_num{}; //!< 階に最大存在できる数 / Maximum population allowed per level
    MONSTER_NUMBER mob_num{}; //!< 動員可能数
    MONSTER_NUMBER cur_num{}; //!< 階に現在いる数 / Monster population on current level
    MONSTER_NUMBER father_r_idx{}; //!< 父親モンスター種族ID
    MONSTER_NUMBER mother_r_idx{}; //!< 母親モンスター種族ID
    int32_t collapse_over = 0; //!< 生成条件：時空崩壊度加減
    int32_t plus_collapse{}; //!< 死亡時の時空崩壊度進行値
    FLOOR_IDX floor_id{}; //!< 存在している保存階ID /  Location of unique monster
    MONSTER_NUMBER r_sights{}; //!< 見えている数 / Count sightings of this monster
    MONSTER_NUMBER r_deaths{}; //!< このモンスターに殺された人数 / Count deaths from this monster
    MONSTER_NUMBER r_pkills{}; //!< このゲームで倒すのを見た数 / Count visible monsters killed in this life
    MONSTER_NUMBER r_akills{}; //!< このゲームで倒した数 / Count all monsters killed in this life
    MONSTER_NUMBER r_tkills{}; //!< 全ゲームで倒した数 / Count monsters killed in all lives
    byte r_wake{}; //!< @に気づいて起きた数 / Number of times woken up (?)
    byte r_ignore{}; //!< @に気づいていない数 / Number of times ignored (?)
    bool r_can_evolve{}; //!< 進化するか否か / Flag being able to evolve
    ITEM_NUMBER r_drop_gold{}; //!< これまでに撃破時に落とした財宝の数 / Max number of gold dropped at once
    ITEM_NUMBER r_drop_item{}; //!< これまでに撃破時に落としたアイテムの数 / Max number of item dropped at once
    byte r_cast_spell{}; //!< 使った魔法/ブレスの種類数 /  Max unique number of spells seen
    byte r_blows[MAX_NUM_BLOWS]{}; //!< 受けた打撃 /  Number of times each blow type was seen
    EnumClassFlagGroup<MonsterAbilityType> r_ability_flags; //!< 見た能力フラグ(魔法/ブレス) / Observed racial ability flags
    EnumClassFlagGroup<MonsterAuraType> r_aura_flags; //!< 見た能力フラグ(オーラ) / Observed aura flags
    EnumClassFlagGroup<MonsterBehaviorType> r_behavior_flags; //!< 見た能力フラグ（習性） / Observed racial attr flags
    EnumClassFlagGroup<MonsterKindType> r_kind_flags; //!< 見た能力フラグ（種族・徳） / Observed racial attr flags
    EnumClassFlagGroup<MonsterResistanceType> r_resistance_flags; //!< 見た耐性フラグ / Observed racial resistances flags
    EnumClassFlagGroup<MonsterDropType> r_drop_flags; //!< 見た能力フラグ（ドロップ） / Observed drop flags
    EnumClassFlagGroup<MonsterFeatureType> r_feature_flags; //!< 見た能力フラグ(地形関連) / Observed feature flags
    EnumClassFlagGroup<MonsterSpecialType> r_special_flags; //!< 見た能力フラグ(特殊) / Observed special flags
    EnumClassFlagGroup<MonsterMiscType> r_misc_flags; //!< 見た能力フラグ(その他) / Observed feature flags
    PLAYER_LEVEL defeat_level{}; //!< 倒したレベル(ユニーク用) / player level at which defeated this race
    REAL_TIME defeat_time{}; //!< 倒した時間(ユニーク用) / time at which defeated this race
    PERCENTAGE cur_hp_per{}; //!< 生成時現在HP率(%)
    AllianceType alliance_idx = AllianceType::NONE;

    bool is_valid() const;
    bool is_male() const;
    bool is_female() const;
    bool has_living_flag() const;
    bool is_explodable() const;
    bool symbol_char_is_any_of(std::string_view symbol_characters) const;
    std::string get_died_message() const;
    std::optional<bool> order_level(const MonraceDefinition &other) const;
    bool order_level_strictly(const MonraceDefinition &other) const;
    std::optional<bool> order_pet(const MonraceDefinition &other) const;
    std::string get_pronoun_of_summoned_kin() const;
    const MonraceDefinition &get_next() const;
    bool is_bounty(bool unachieved_only) const;
    int calc_power() const;
    int calc_figurine_value() const;
    int calc_capture_value() const;
    std::string build_eldritch_horror_message(std::string_view description) const;
    bool has_reinforce() const;
    const std::vector<Reinforce> &get_reinforces() const;
    bool can_generate() const;
    GridFlow get_grid_flow_type() const;
    bool is_suitable_for_floor() const;
    bool is_suitable_for_random_quest() const;
    bool is_suitable_for_shallow_water() const;
    bool is_suitable_for_deep_water() const;
    bool is_suitable_for_lava() const;
    bool is_suitable_for_trapped_pit() const;
    bool is_suitable_for_special_room() const;
    bool is_suitable_for_glass() const;
    bool is_suitable_for_town() const;
    bool is_suitable_for_ocean() const;
    bool is_suitable_for_shore() const;
    bool is_suitable_for_waste() const;
    bool is_suitable_for_grass() const;
    bool is_suitable_for_wood() const;
    bool is_suitable_for_tanuki() const;

    void init_sex(uint32_t value);

    std::optional<std::string> probe_lore();
    void make_lore_treasure(int num_item, int num_drop);
    void emplace_reinforce(MonraceId monrace_id, const Dice &dice);

    //!< @todo ここから先はミュータブルなフィールドなので分離すべき.
    bool has_entity() const;
    bool should_display(bool is_alive) const;
    bool is_details_known() const;
    bool is_blow_damage_known(int num_blow) const;
    void kill_unique();
    bool is_dead_unique() const;

    void reset_current_numbers();
    void increment_current_numbers();
    void decrement_current_numbers();
    void reset_max_number();

    void increment_akills();
    void increment_pkills();
    void increment_tkills();

private:
    std::vector<Reinforce> reinforces; //!< 指定護衛リスト

    const std::string &decide_horror_message() const;
};
