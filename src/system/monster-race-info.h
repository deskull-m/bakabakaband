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
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

/*! モンスターが1ターンに攻撃する最大回数 (射撃を含む) / The maximum number of times a monster can attack in a turn (including SHOOT) */
constexpr int MAX_NUM_BLOWS = 4;

enum class FixedArtifactId : short;
enum class MonsterRaceId : int16_t;

class MonsterBlow {
public:
    RaceBlowMethodType method{};
    RaceBlowEffectType effect{};
    Dice damage_dice;
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
class Reinforce;
class MonsterRaceInfo {
public:
    MonsterRaceInfo();

    MonsterRaceId idx{};
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

    std::vector<std::tuple<int, int, MonsterRaceId>> spawn_monsters; //!< 落とし子生成率
    std::vector<std::tuple<int, int, FEAT_IDX>> change_feats; //!< 地形変化率
    std::vector<std::tuple<int, int, short>> spawn_items; //!< アイテム自然生成率
    std::vector<std::tuple<int, int, short, int, int, int>> drop_kinds; //!< アイテム特定ドロップ指定
    std::vector<std::tuple<int, int, short, int, int, int>> drop_tvals; //!< アイテム種別ドロップ指定
    std::vector<std::tuple<int, int, MonsterRaceId, int, int>> dead_spawns; //!< 死亡時モンスター生成

    //! 特定アーティファクトドロップリスト <アーティファクトID,ドロップ率>
    std::vector<std::tuple<FixedArtifactId, PERCENTAGE>> drop_artifacts;
    int suicide_dice_num{}; //!< 自滅ターンダイス数
    int suicide_dice_side{}; //!< 自滅ターン面数
    PERCENTAGE arena_ratio{}; //!< モンスター闘技場の掛け金倍率修正値(%基準 / 0=100%) / The adjustment ratio for gambling monster
    MonsterRaceId next_r_idx{}; //!< 進化先モンスター種族ID
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
    std::optional<bool> order_level(const MonsterRaceInfo &other) const;
    bool order_level_strictly(const MonsterRaceInfo &other) const;
    std::optional<bool> order_pet(const MonsterRaceInfo &other) const;
    void kill_unique();
    std::string get_pronoun_of_summoned_kin() const;
    const MonsterRaceInfo &get_next() const;
    bool is_bounty(bool unachieved_only) const;
    int calc_power() const;
    int calc_figurine_value() const;
    int calc_capture_value() const;
    std::string build_eldritch_horror_message(std::string_view description) const;
    bool has_reinforce() const;
    const std::vector<Reinforce> &get_reinforces() const;

    std::optional<std::string> probe_lore();
    void make_lore_treasure(int num_item, int num_drop);
    void emplace_reinforce(MonsterRaceId monrace_id, const Dice &dice);

private:
    std::vector<Reinforce> reinforces; //!< 指定護衛リスト

    const std::string &decide_horror_message() const;
};

class Reinforce {
public:
    Reinforce(MonsterRaceId monrace_id, Dice dice);
    MonsterRaceId get_monrace_id() const;
    bool is_valid() const;
    const MonsterRaceInfo &get_monrace() const;
    std::string get_dice_as_string() const;
    int roll_dice() const;
    int roll_max_dice() const;

private:
    MonsterRaceId monrace_id;
    Dice dice;
};

extern std::map<MonsterRaceId, MonsterRaceInfo> monraces_info;

class MonraceList {
public:
    MonraceList(MonraceList &&) = delete;
    MonraceList(const MonraceList &) = delete;
    MonraceList &operator=(const MonraceList &) = delete;
    MonraceList &operator=(MonraceList &&) = delete;

    static bool is_valid(MonsterRaceId monrace_id);
    static const std::map<MonsterRaceId, std::set<MonsterRaceId>> &get_unified_uniques();
    static MonraceList &get_instance();
    static MonsterRaceId empty_id();
    std::map<MonsterRaceId, MonsterRaceInfo>::iterator begin();
    std::map<MonsterRaceId, MonsterRaceInfo>::const_iterator begin() const;
    std::map<MonsterRaceId, MonsterRaceInfo>::iterator end();
    std::map<MonsterRaceId, MonsterRaceInfo>::const_iterator end() const;
    std::map<MonsterRaceId, MonsterRaceInfo>::reverse_iterator rbegin();
    std::map<MonsterRaceId, MonsterRaceInfo>::const_reverse_iterator rbegin() const;
    std::map<MonsterRaceId, MonsterRaceInfo>::reverse_iterator rend();
    std::map<MonsterRaceId, MonsterRaceInfo>::const_reverse_iterator rend() const;
    size_t size() const;
    MonsterRaceInfo &get_monrace(MonsterRaceId monrace_id);
    const MonsterRaceInfo &get_monrace(MonsterRaceId monrace_id) const;
    const std::vector<MonsterRaceId> &get_valid_monrace_ids() const;
    bool can_unify_separate(const MonsterRaceId r_idx) const;
    void kill_unified_unique(const MonsterRaceId r_idx);
    bool is_selectable(const MonsterRaceId r_idx) const;
    void defeat_separated_uniques();
    bool is_unified(const MonsterRaceId r_idx) const;
    bool exists_separates(const MonsterRaceId r_idx) const;
    bool is_separated(const MonsterRaceId r_idx) const;
    bool can_select_separate(const MonsterRaceId morace_id, const int hp, const int maxhp) const;
    bool order(MonsterRaceId id1, MonsterRaceId id2, bool is_detailed = false) const;
    bool order_level_unique(MonsterRaceId id1, MonsterRaceId id2) const;
    MonsterRaceId pick_id_at_random() const;
    const MonsterRaceInfo &pick_monrace_at_random() const;

    void reset_all_visuals();
    std::optional<std::string> probe_lore(MonsterRaceId monrace_id);

private:
    MonraceList() = default;

    static MonraceList instance;

    const static std::map<MonsterRaceId, std::set<MonsterRaceId>> unified_uniques;
};
