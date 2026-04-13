#pragma once

#include "artifact/fixed-art-types.h"
#include "combat/martial-arts-style.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/trc-types.h"
#include "player-ability/player-ability-types.h"
#include "player-info/class-specific-data.h"
#include "player-info/class-types.h"
#include "player-info/race-types.h"
#include "player/player-personality-types.h"
#include "player/player-sex.h"
#include "player/player-skill.h"
#include "system/angband.h"
#include "system/creature-timed-effect-types.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/monster-profile.h"
#include "system/system-variables.h"
#include "util/dice.h"
#include "util/flag-group.h"
#include "util/point-2d.h"
#include <array>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <tl/optional.hpp>
#include <vector>

constexpr int MONSTER_MAXHP = 10000000; //!< モンスターの最大HP

// Forward declarations
class Direction;
class FloorType;
class ItemEntity;
class MonraceDefinition;
class TimedEffects;
struct player_race_info;
struct player_personality;
struct player_class_info;
enum class ElementRealmType;
enum class FixedArtifactId : short;
enum class ItemKindType : short;
enum class MimicKindType;
enum class MonraceId : int16_t;
enum class MonsterAbilityType;
enum class PlayerSkillKindType;
enum class RealmType;
enum class Virtue : short;

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

/*!
 * @brief 死亡履歴情報構造体
 */
struct DeathRecord {
    int32_t game_turn; /*!< 死亡時のゲームターン */
    int16_t day; /*!< 死亡時の日数 */
    int16_t hour; /*!< 死亡時の時刻 */
    int16_t min; /*!< 死亡時の分 */
    int16_t player_level; /*!< 死亡時のプレイヤーレベル */
    std::string cause; /*!< 死因 */
    MonraceId killer_monrace_id; /*!< 死亡原因のモンスターID */
};

/*!
 * @brief プレイヤーとモンスターの共通基底クラス
 * @details PlayerTypeとMonsterEntityの実装を将来的に一元化するための基底クラス。
 * 座標、HP、速度、エネルギーなど両者に共通する基本属性を保持する。
 */
class CreatureEntity {
public:
    CreatureEntity() = default;
    virtual ~CreatureEntity() = default;

    // コピー・ムーブを許可
    CreatureEntity(const CreatureEntity &) = default;
    CreatureEntity &operator=(const CreatureEntity &) = default;
    CreatureEntity(CreatureEntity &&) = default;
    CreatureEntity &operator=(CreatureEntity &&) = default;

    /*!
     * @brief クリーチャーの座標を取得
     * @return 座標
     */
    virtual Pos2D get_position() const
    {
        return Pos2D(this->y, this->x);
    }

    /*!
     * @brief クリーチャーのX座標を取得
     * @return X座標
     */
    virtual POSITION get_x() const
    {
        return this->x;
    }

    /*!
     * @brief クリーチャーのY座標を取得
     * @return Y座標
     */
    virtual POSITION get_y() const
    {
        return this->y;
    }

    /*!
     * @brief クリーチャーの前回の座標を取得
     * @return 前回の座標
     */
    virtual Pos2D get_old_position() const
    {
        return Pos2D(this->oldpy, this->oldpx);
    }

    /*!
     * @brief 指定座標にクリーチャーがいるかどうかを判定
     * @param pos 判定する座標
     * @return 指定座標にいればtrue
     */
    virtual bool is_located_at(const Pos2D &pos) const
    {
        return (this->y == pos.y) && (this->x == pos.x);
    }

    /*!
     * @brief 現在地の隣 (瞬時値)または現在地を返す
     * @param dir 隣を表す方向番号
     * @details クリーチャーが移動する前後の文脈で使用すると不整合を起こすので注意
     * 方向番号による位置取りは以下の通り. 0と5は現在地.
     * 789 ...
     * 456 .@.
     * 123 ...
     */
    Pos2D get_neighbor(int dir) const;

    /*!
     * @brief 現在地の隣 (瞬時値)または現在地を返す
     * @param dir 隣を表す方向
     * @attention クリーチャーが移動する前後の文脈で使用すると不整合を起こすので注意
     */
    Pos2D get_neighbor(const Direction &dir) const;

    /*!
     * @brief クリーチャーの攻撃目標座標を設定
     * @param pos 目標座標
     */
    void set_target(const Pos2D &pos);

    /*!
     * @brief クリーチャーの攻撃目標座標をリセット
     */
    void reset_target();

    /*!
     * @brief クリーチャーの攻撃目標座標を取得
     * @return 目標座標
     */
    Pos2D get_target_position() const;

    /*!
     * @brief クリーチャーの現在HPを取得
     * @return 現在HP
     */
    virtual int get_current_hp() const
    {
        return this->hp;
    }

    /*!
     * @brief クリーチャーの最大HPを取得
     * @return 最大HP
     */
    virtual int get_max_hp() const
    {
        return this->maxhp;
    }

    /*!
     * @brief クリーチャーの速度を取得
     * @return 速度値
     */
    virtual int get_speed() const
    {
        return this->speed;
    }

    /*!<
     * @brief クリーチャーが名前を持っているかどうか
     * @return 名前を持っていればtrue
     */
    bool is_named() const
    {
        return !this->name.empty();
    }

    /*!
     * @brief クリーチャーが男性かどうかを判定
     * @return 男性ならtrue
     */
    bool is_male() const;

    /*!
     * @brief クリーチャーが女性かどうかを判定
     * @return 女性ならtrue（WAIFUIZED フラグも含む）
     */
    bool is_female() const;

    /*!
     * @brief クリーチャーが名前付きペットかどうかを判定
     * @return 名前付きペットならtrue
     */
    bool is_named_pet() const
    {
        return this->is_pet() && this->is_named();
    }

    /*!
     * @brief クリーチャーの座標を設定する
     * @param pos 設定する座標
     */
    void set_position(const Pos2D &pos)
    {
        this->y = pos.y;
        this->x = pos.x;
    }

    /*!
     * @brief クリーチャーの外見種族が実種族と一致しているかどうかを判定
     * @return 外見種族 == 実種族 ならtrue（通常状態）
     * @details モンスターでは変身・誤認がない場合にtrue。プレイヤーでは常にtrue。
     */
    bool is_original_ap() const
    {
        return this->ap_r_idx == this->r_idx;
    }

    /*!
     * @brief クリーチャーの速度を設定
     * @param new_speed 速度値
     */
    virtual void set_speed(int new_speed)
    {
        this->speed = new_speed;
    }

    /*!
     * @brief クリーチャーが有効（生存中）かどうかを判定
     * @return 有効ならtrue
     * @details デフォルト実装は MonraceList::is_valid(r_idx)（モンスター用）。
     *          PlayerType はオーバーライドして常に true を返す。
     */
    virtual bool is_valid() const;

    /*!
     * @brief クリーチャーが死亡しているかどうかを判定
     * @return 死亡していればtrue
     * @details デフォルト実装は hp < 0（モンスター用）。PlayerType はオーバーライドして is_dead_ フラグを返す。
     */
    virtual bool is_dead() const
    {
        return this->hp < 0;
    }

    /*!
     * @brief クリーチャーの実効ACを取得
     * @return 実効AC値（ac + to_a。ただしモンスターで NAKED フラグ付きの場合は 0）
     */
    virtual int get_ac() const;

    /*!
     * @brief セーブフロアに滞在中かどうか
     * @return floor_id が 0 でなければ true
     */
    bool in_saved_floor() const
    {
        return this->floor_id != 0;
    }

    bool is_fully_healthy() const;
    bool is_located_at_running_destination() const;
    bool is_true_winner() const;
    bool try_resist_eldritch_horror() const;
    void ride_monster(MONSTER_IDX m_idx);
    void plus_incident(INCIDENT incidentID, int num);

    /*!
     * @brief モンスターがいない場合にのみ位置を設定する
     * @return 設定できた場合 true
     */
    bool try_set_position(const Pos2D &pos);

    /*!
     * @brief クリーチャーが所属するフロアを取得
     * @return フロアへのポインタ
     */
    virtual FloorType *get_floor() const
    {
        return this->current_floor_ptr;
    }

    /*!
     * @brief クリーチャーの実種族定義を取得する
     * @return 実種族定義への参照（r_idx が MonraceId::PLAYER の場合はプレイヤー種族エントリを返す）
     */
    MonraceDefinition &get_monrace() const;

    /*!
     * @brief クリーチャーの外見種族定義を取得する
     * @return 外見種族定義への参照（通常は get_monrace() と同じ、変身・誤認時は異なる）
     */
    MonraceDefinition &get_appearance_monrace() const;

    /*!
     * @brief クリーチャーの真の種族IDを取得する（カメレオン変身考慮）
     * @return 真の種族ID
     */
    MonraceId get_real_monrace_id() const;

    /*!
     * @brief クリーチャーの真の種族定義を取得する（カメレオン変身考慮）
     * @return 真の種族定義への参照
     */
    MonraceDefinition &get_real_monrace() const;

    bool has_living_flag(bool is_appearance = false) const;
    bool has_demon_flag(bool is_appearance = false) const;
    bool has_undead_flag(bool is_appearance = false) const;
    bool is_explodable() const;
    std::string get_died_message() const;
    bool can_ring_boss_call_nazgul() const;
    std::pair<TERM_COLOR, int> get_hp_bar_data() const;

    /*!
     * @brief 次の行動までに必要なエネルギーを取得
     * @return エネルギー値
     */
    virtual ACTION_ENERGY get_energy_need() const
    {
        return this->energy_need;
    }

    /*!
     * @brief 次の行動までに必要なエネルギーを設定
     * @param energy エネルギー値
     */
    virtual void set_energy_need(ACTION_ENERGY energy)
    {
        this->energy_need = energy;
    }

    /*!
     * @brief クリーチャーのレベルを取得
     * @return レベル値。個体レベルが設定されていればそれを返し、未設定なら種族レベルの半分を返す。
     */
    virtual int get_level() const;

    /*!
     * @brief クリーチャーがプレイヤーかどうかを判定
     * @return プレイヤーならtrue、モンスターならfalse
     * @details デフォルトはfalse（モンスター）。PlayerTypeのみtrueを返す。
     */
    virtual bool is_player() const
    {
        return false;
    }

    /*!
     * @brief ダメージを受けた際のフック（dealt_damage等の蓄積処理）
     * @param damage 受けたダメージ量
     * @details dealt_damage を加算し max_maxhp * 100 を上限とする。
     */
    virtual void on_take_hit(int damage)
    {
        this->dealt_damage += damage;
        if (this->dealt_damage > this->max_maxhp * 100) {
            this->dealt_damage = this->max_maxhp * 100;
        }
    }

    /*!
     * @brief 死亡した際のフック（死亡処理・記録等）
     * @param cause 死亡原因の文字列
     */
    virtual void on_death([[maybe_unused]] std::string_view cause) {}

    /*!
     * @brief クリーチャーの時限効果の残りターン数を取得
     * @param effect 取得する時限効果の種別
     * @return 残りターン数（0なら効果なし）
     * @details デフォルト実装は MonsterProfile::mtimed を参照する（モンスター用）。
     *          PlayerType はオーバーライドして TimedEffects を使う。
     */
    virtual short get_timed_effect(CreatureTimedEffect effect) const;

    /*!
     * @brief クリーチャーの時限効果の残りターン数を直接設定する（セーブ/ロード・内部操作用）
     * @param effect 設定する時限効果の種別
     * @param value 設定するターン数
     * @note メッセージや副作用は発生しない。ゲームロジックからの呼び出しには専用セッターを使うこと。
     * @details デフォルト実装は MonsterProfile::mtimed を更新する（モンスター用）。
     *          PlayerType はオーバーライドして TimedEffects を使う。
     */
    virtual void set_timed_effect(CreatureTimedEffect effect, short value);

    short get_remaining_sleep() const
    {
        return this->get_timed_effect(CreatureTimedEffect::SLEEP_OR_PARALYSIS);
    }

    short get_remaining_stun() const
    {
        return this->get_timed_effect(CreatureTimedEffect::STUN);
    }

    short get_remaining_confusion() const
    {
        return this->get_timed_effect(CreatureTimedEffect::CONFUSION);
    }

    short get_remaining_fear() const
    {
        return this->get_timed_effect(CreatureTimedEffect::FEAR);
    }

    short get_remaining_invulnerability() const
    {
        return this->get_timed_effect(CreatureTimedEffect::INVULNERABILITY);
    }

    short get_remaining_acceleration() const
    {
        return this->get_timed_effect(CreatureTimedEffect::ACCELERATION);
    }

    short get_remaining_deceleration() const
    {
        return this->get_timed_effect(CreatureTimedEffect::DECELERATION);
    }

    short get_remaining_hero() const
    {
        return this->get_timed_effect(CreatureTimedEffect::HERO);
    }

    short get_remaining_berserk() const
    {
        return this->get_timed_effect(CreatureTimedEffect::BERSERK);
    }

    short get_remaining_blessed() const
    {
        return this->get_timed_effect(CreatureTimedEffect::BLESSED);
    }

    short get_remaining_shield() const
    {
        return this->get_timed_effect(CreatureTimedEffect::SHIELD);
    }

    short get_remaining_ultimate_resistance() const
    {
        return this->get_timed_effect(CreatureTimedEffect::ULTIMATE_RESISTANCE);
    }

    short get_remaining_wraith_form() const
    {
        return this->get_timed_effect(CreatureTimedEffect::WRAITH_FORM);
    }

    short get_remaining_tim_esp() const
    {
        return this->get_timed_effect(CreatureTimedEffect::TIM_ESP);
    }

    short get_remaining_tim_stealth() const
    {
        return this->get_timed_effect(CreatureTimedEffect::TIM_STEALTH);
    }

    short get_remaining_tim_regen() const
    {
        return this->get_timed_effect(CreatureTimedEffect::TIM_REGEN);
    }

    short get_remaining_tsuyoshi() const
    {
        return this->get_timed_effect(CreatureTimedEffect::TSUYOSHI);
    }

    short get_remaining_tim_invis() const
    {
        return this->get_timed_effect(CreatureTimedEffect::TIM_INVIS);
    }

    short get_remaining_tim_infra() const
    {
        return this->get_timed_effect(CreatureTimedEffect::TIM_INFRA);
    }

    tl::optional<std::string> get_pain_message(std::string_view monster_name, int damage) const;

    /*!
     * @brief カメレオンの変身を元に戻す。
     * @details r_idx と ap_r_idx を実種族IDにリセットする。
     */
    virtual void reset_chameleon_polymorph()
    {
        const auto real_id = this->get_real_monrace_id();
        this->r_idx = real_id;
        this->ap_r_idx = real_id;
    }

    /*!
     * @brief ルアー記録に宝物情報を追加する
     * @param num_item アイテム数
     * @param num_gold 金貨数
     * @details is_original_ap() でない場合は何もしない。
     */
    virtual void make_lore_treasure(int num_item, int num_gold) const;

    virtual std::string build_looking_description(bool needs_attitude) const;

    /*!
     * @brief クリーチャーが睡眠状態かどうかを判定
     * @return 睡眠状態ならtrue
     */
    virtual bool is_asleep() const;

    /*!
     * @brief クリーチャーが朦朧状態かどうかを判定
     * @return 朦朧状態ならtrue
     */
    virtual bool is_stunned() const;

    /*!
     * @brief クリーチャーが混乱状態かどうかを判定
     * @return 混乱状態ならtrue
     */
    virtual bool is_confused() const;

    /*!
     * @brief クリーチャーが恐怖状態かどうかを判定
     * @return 恐怖状態ならtrue
     */
    virtual bool is_fearful() const;

    /*!
     * @brief クリーチャーが無敵状態かどうかを判定
     * @return 無敵状態ならtrue
     */
    virtual bool is_invulnerable() const;

    /*!
     * @brief クリーチャーが盲目かどうかを判定
     * @return 盲目ならtrue（モンスターは常にfalse）
     */
    virtual bool is_blind() const;

    /*!
     * @brief クリーチャーが麻痺しているかどうかを判定
     * @return 麻痺していればtrue（モンスターは常にfalse）
     */
    virtual bool is_paralyzed() const;

    /*!
     * @brief クリーチャーが加速しているかどうかを判定
     * @return 加速中ならtrue
     */
    virtual bool is_fast() const;

    /*!
     * @brief クリーチャーが加速しているかどうかを判定
     * @return 加速中ならtrue
     */
    virtual bool is_accelerated() const;

    /*!
     * @brief クリーチャーが減速しているかどうかを判定
     * @return 減速中ならtrue
     */
    virtual bool is_decelerated() const;

    /*!
     * @brief クリーチャーが祝福状態かどうかを判定
     * @return 祝福されていればtrue
     */
    virtual bool is_blessed() const;

    /*!
     * @brief クリーチャーが士気高揚状態かどうかを判定
     * @return 士気高揚ならtrue
     */
    virtual bool is_hero() const;

    /*!
     * @brief クリーチャーが狂戦士状態かどうかを判定
     * @return 狂戦士ならtrue
     */
    virtual bool is_shero() const;

    virtual bool is_echizen() const;

    /*!
     * @brief クリーチャーがペットかどうかを判定
     * @return ペットならtrue、デフォルトはfalse（プレイヤーはペットではない）
     */
    virtual bool is_pet() const
    {
        return this->has_monster_profile() && this->get_monster_profile().mflag2.has(MonsterConstantFlagType::PET);
    }

    /*!
     * @brief クリーチャーがフレンドリー状態かどうかを判定
     * @return フレンドリーならtrue、デフォルトはfalse（プレイヤーはフレンドリー判定対象外）
     */
    virtual bool is_friendly() const
    {
        return this->has_monster_profile() && this->get_monster_profile().mflag2.has(MonsterConstantFlagType::FRIENDLY);
    }

    /*!
     * @brief クリーチャーが敵対状態かどうかを判定
     * @return 敵対ならtrue（ペットでもフレンドリーでもない場合）
     * @note プレイヤーに対してはfalseを返す
     */
    virtual bool is_hostile() const
    {
        return this->has_monster_profile() && !this->is_friendly() && !this->is_pet();
    }

    /*!
     * @brief クリーチャーを敵対状態に設定する
     * @note モンスターの場合はペット・フレンドリーフラグをリセットし同盟も更新する。プレイヤーには無効。
     */
    virtual void set_hostile();

    /*!
     * @brief クリーチャーをフレンドリー状態に設定する
     * @details モンスターの場合は FRIENDLY フラグをセットする。
     */
    virtual void set_friendly()
    {
        if (this->has_monster_profile()) {
            this->get_monster_profile().mflag2.set(MonsterConstantFlagType::FRIENDLY);
        }
    }

    virtual void set_individual_speed(bool force_fixed_speed);

    /*!
     * @brief モンスターのフラグに基づいて対応するプレイヤー種族IDを初期化する
     * @details モンスター以外では何もしない。
     */
    virtual void initialize_equivalent_player_races();

    /*!
     * @brief モンスターのフラグに基づいて対応するプレイヤー職業IDを初期化する
     * @details モンスター以外では何もしない。
     */
    virtual void initialize_equivalent_player_classes();

    byte get_temporary_speed() const;

    /*!
     * @brief クリーチャーの状態をデフォルト（空）にリセットする
     * @note モンスターでは *this = {} を行う。プレイヤーではデフォルト実装は何もしない。
     */
    virtual void wipe()
    {
    }

    /*!
     * @brief 二つのサブアライメントが敵対しているかどうかを判定
     * @param sub_align1 アライメント1
     * @param sub_align2 アライメント2
     * @return 敵対しているならtrue
     */
    static bool check_sub_alignments(const byte sub_align1, const byte sub_align2);

    /*!
     * @brief 近接攻撃において敵対しているかどうかを判定
     * @param other 対象クリーチャー
     * @return 敵対しているならtrue、デフォルトはfalse
     */
    virtual bool is_hostile_to_melee(const CreatureEntity &other) const;
    bool is_hostile_align(byte other_sub_align) const;
    bool is_mimicry() const;
    tl::optional<bool> order_pet_whistle(const CreatureEntity &other) const;
    tl::optional<bool> order_pet_dismission(const CreatureEntity &other) const;

    /*!
     * @brief クリーチャーが騎乗されているかどうかを判定
     * @return 騎乗されているならtrue、デフォルトはfalse
     */
    virtual bool is_riding() const
    {
        return this->has_monster_profile() && this->get_monster_profile().mflag2.has(MonsterConstantFlagType::RIDING);
    }

    /*!
     * @brief クリーチャーに召喚主がいるかどうかを判定
     * @return 召喚主がいるならtrue、デフォルトはfalse
     */
    virtual bool has_parent() const
    {
        return this->has_monster_profile() && this->get_monster_profile().parent_m_idx > 0;
    }

    bool is_tough() const
    {
        return this->ppersonality == PERSONALITY_TOUGH;
    }

    bool is_chargeman() const
    {
        return this->ppersonality == PERSONALITY_CHARGEMAN;
    }

    bool is_sushi_eater() const
    {
        return this->ppersonality == PERSONALITY_SUSHI_EATER;
    }

    virtual bool is_time_limit_esp() const;
    virtual bool is_time_limit_stealth() const;

    /*!
     * @brief 体力ランク (0-100) を計算する
     * @return 体力ランク
     */
    int calc_life_rating() const;
    std::string decrease_ability_random();
    std::string decrease_ability_all();

    /*!
     * @brief 指定した固定アーティファクトを装備しているかどうか調べる
     * @param fa_id 固定アーティファクトのID
     * @return 装備していればtrue、そうでなければfalse
     */
    bool is_wielding(FixedArtifactId fa_id) const;

    /*!
     * @brief モンスター固有データを取得する（非const版）
     * @return モンスター固有データへの参照
     * @pre has_monster_profile() == true
     */
    MonsterProfile &get_monster_profile()
    {
        return this->monster_profile.value();
    }

    /*!
     * @brief モンスター固有データを取得する（const版）
     * @return モンスター固有データへのconst参照
     * @pre has_monster_profile() == true
     */
    const MonsterProfile &get_monster_profile() const
    {
        return this->monster_profile.value();
    }

    /*!
     * @brief モンスター固有データを持っているかどうかを返す
     * @return モンスター固有データがあればtrue
     */
    bool has_monster_profile() const
    {
        return this->monster_profile.has_value();
    }

    // モンスター種族ID（プレイヤーの場合は0）
    MonraceId r_idx{}; /*!< モンスターの実種族ID (これが0の時は死亡扱いになる) / Monster race index 0 = dead. */
    MonraceId ap_r_idx{}; /*!< モンスターの外見種族ID（あやしい影、たぬき、ジュラル星人誤認などにより変化する）Monster race appearance index */

    // 与ダメージ蓄積（プレイヤー・モンスター共通）
    int32_t dealt_damage{}; /*!< これまでに蓄積して与えてきたダメージ / Sum of damages dealt by player or to monster */

    // 種族・職業・性格情報（参照風アクセス用）
    const player_race_info *race{}; /*!< 現在の種族情報 / Current race info */
    const player_personality *personality{}; /*!< 現在の性格情報 / Current personality info (accessed like reference) */
    const player_class_info *pclass_ref{}; /*!< 現在の職業情報 / Current class info (accessed like reference) */

    // 行動技能値 / Action skills
    ACTION_SKILL_POWER see_infra{}; /*!< 赤外線視能力の強さ / Infravision range */
    ACTION_SKILL_POWER skill_dis{}; /*!< 行動技能値:解除能力 / Skill: Disarming */
    ACTION_SKILL_POWER skill_dev{}; /*!< 行動技能値:魔道具使用 / Skill: Magic Devices */
    ACTION_SKILL_POWER skill_sav{}; /*!< 行動技能値:魔法防御 / Skill: Saving throw */
    ACTION_SKILL_POWER skill_stl{}; /*!< 行動技能値:隠密 / Skill: Stealth factor */
    ACTION_SKILL_POWER skill_srh{}; /*!< 行動技能値:知覚 / Skill: Searching ability */
    ACTION_SKILL_POWER skill_fos{}; /*!< 行動技能値:探索 / Skill: Searching frequency */
    ACTION_SKILL_POWER skill_thn{}; /*!< 行動技能値:打撃命中能力 / Skill: To hit (normal) */
    ACTION_SKILL_POWER skill_thb{}; /*!< 行動技能値:射撃命中能力 / Skill: To hit (shooting) */
    ACTION_SKILL_POWER skill_tht{}; /*!< 行動技能値:投射命中能力 / Skill: To hit (throwing) */
    ACTION_SKILL_POWER skill_dig{}; /*!< 行動技能値:掘削 / Skill: Digging */

    POSITION run_py{}; /*!< 走行中の目標Y座標 / Target Y position while running */
    POSITION run_px{}; /*!< 走行中の目標X座標 / Target X position while running */

    Pos2D target{}; /*!< 攻撃目標座標 (0,0 は未設定) / Attack target position ({0,0} means none) */

    tl::optional<MonsterProfile> monster_profile{}; /*!< モンスター固有データ (モンスターの場合のみ有効) */

    bool ambush_flag{}; /*!< 待ち伏せフラグ / Ambush flag */

    // 基本情報
    int16_t age{}; /*!< 年齢 / Age */
    int16_t ht{}; /*!< 身長 / Height */
    int16_t wt{}; /*!< 体重 / Weight */
    int16_t prestige{}; /*!< 名声 / Prestige */
    int32_t death_count{}; /*!< 死亡カウント / Death count */

    // ステータス関連
    short stat_max[A_MAX]{}; /*!< 現在の最大能力値 / Current "maximal" stat values */
    short stat_max_max[A_MAX]{}; /*!< 最大の最大能力値 / Maximal "maximal" stat values */
    short stat_cur[A_MAX]{}; /*!< 現在の基本能力値 / Current "natural" stat values */
    int16_t stat_use[A_MAX]{}; /*!< 現在の修正済み能力値 / Current modified stats */
    int16_t stat_top[A_MAX]{}; /*!< 最大の修正済み能力値 / Maximal modified stats */
    int16_t stat_add[A_MAX]{}; /* Modifiers to stat values */
    int16_t stat_index[A_MAX]{}; /* Indexes into stat tables */

    // 徳関連
    std::map<Virtue, int16_t> virtues; /*!< 徳の値 / Virtue values */

    // 経験値関連
    EXP max_max_exp{}; /*!< 最大の最大経験値 / Max max experience (only to calculate score) */
    EXP max_exp{}; /*!< 最大経験値 / Max experience */
    EXP exp{}; /*!< 現在の経験値 / Current experience */
    uint32_t exp_frac{}; /*!< 経験値の小数部 / Current exp frac (times 2^16) */

    // 騎乗関連
    MONSTER_IDX riding{}; /*!< 騎乗中のモンスターID / Riding on a monster of this index */

    // インベントリ関連
    std::vector<std::shared_ptr<ItemEntity>> inventory{}; /*!< 所持品リスト / The creature's inventory */
    int16_t inven_cnt{}; /*!< 所持品数 / Number of items in inventory */
    int16_t equip_cnt{}; /*!< 装備品数 / Number of items in equipment */

    // 座標関連
    POSITION oldpy{}; /*!< 前回のY座標 / Previous location (Y) */
    POSITION oldpx{}; /*!< 前回のX座標 / Previous location (X) */
    POSITION y{}; /*!< 現在のY座標 / Current location (Y) */
    POSITION x{}; /*!< 現在のX座標 / Current location (X) */

    // MP関連
    MANA_POINT msp{}; /*!< 最大MP / Max mana pts */
    MANA_POINT csp{}; /*!< 現在MP / Current mana pts */
    uint32_t csp_frac{}; /*!< MP小数部 / Current mana frac (times 2^16) */

    // HP関連
    int hp{}; /*!< 現在のHP / Current Hit points */
    int maxhp{}; /*!< 現在の最大HP / Max Hit points */
    int max_maxhp{}; /*!< 生成時の初期最大HP / Max Max Hit points */
    uint32_t hp_frac{}; /*!< HP小数部 / Current hit frac (times 2^16) */

    // 基本パラメータ（主にプレイヤー用、モンスターでは未使用）
    player_sex psex{}; /*!< 性別 / Sex index */
    PlayerRaceType prace{}; /*!< 種族 / Race index */
    PlayerClassType pclass{}; /*!< クラス / Class index */
    player_personality_type ppersonality{}; /*!< 性格 / Personality index */
    int16_t town_num{}; /*!< 現在いる街番号 / Current town number */

    // クラス固有データ / Class-specific data
    ClassSpecificData class_specific_data;

    // 変異関連
    EnumClassFlagGroup<PlayerMutationType> muta{}; /*!< 突然変異 / mutations */
    EnumClassFlagGroup<PlayerMutationType> trait{}; /*!< 後天特性 / permanent trait */

    // パトロン関連（主にカオス戦士用、モンスターでは未使用）
    int16_t patron{}; /*!< カオスパトロンのID / Chaos patron ID */

    // エネルギー関連
    ACTION_ENERGY energy_need{}; /*!< 次の行動までに必要なエネルギー / Energy needed for next move */

    // 速度関連
    int speed{}; /*!< クリーチャーの速度 / Creature speed */

    // レベル関連
    int16_t level{}; /*!< クリーチャーのレベル / Creature level */

    // アライメント関連
    int alignment{}; /*!< 善悪の属性 / Good/evil/neutral */

    // 行動状態関連
    byte action{}; /*!< クリーチャーが現在取っている常時行動のID / Currently action */
    int16_t running{}; /*!< 現在の走行カウンタ / Current counter for running, if any */

    // 所持金関連
    PRICE au{}; /*!< 所持金 / Current Gold */

    // AC関連
    ARMOUR_CLASS ac{}; /*!< アーマークラス（プレイヤーは装備無しの基本AC、モンスターは総合AC） / Armor class (base AC for player, total AC for monster) */
    ARMOUR_CLASS to_a{}; /*!< ACへのボーナス（主にプレイヤー用、装備などによるボーナス） / Bonus to AC (mainly for player, bonus from equipment) */

    // 名前関連
    std::string name{}; /*!< クリーチャーの名前（プレイヤー名またはペット名） / Creature's name (player name or pet nickname) */

    // テレパシー・感知能力関連 / Telepathy and ESP abilities
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

    // 地形移動能力 / Terrain movement abilities
    bool can_swim{}; /* No damage in water */

    // 時限効果 / Timed effects
    TIME_EFFECT invuln{}; /* Timed -- Invulnerable */
    TIME_EFFECT ult_res{}; /* Timed -- Ultimate Resistance */
    TIME_EFFECT hero{}; /* Timed -- Heroism */
    TIME_EFFECT berserk{}; /* Timed -- Super Heroism */
    TIME_EFFECT shield{}; /* Timed -- Shield Spell */
    TIME_EFFECT blessed{}; /* Timed -- Blessed */
    TIME_EFFECT tim_invis{}; /* Timed -- See Invisible */
    TIME_EFFECT tim_infra{}; /* Timed -- Infra Vision */
    TIME_EFFECT tsuyoshi{}; /* Timed -- Tsuyoshi Special */
    TIME_EFFECT ele_attack{}; /* Timed -- Elemental Attack */
    TIME_EFFECT ele_immune{}; /* Timed -- Elemental Immune */

    /* クリーチャーの防御状態の定義 / Bit flags for the "special_defense" variable. -LM- */
    BIT_FLAGS special_defense{};

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

    // 装備・能力関連フラグ / Equipment and ability flags
    bool hack_mutation{};
    bool is_fired{};
    bool level_up_message{};

    BIT_FLAGS anti_magic{}; /* Anti-magic */
    BIT_FLAGS anti_tele{}; /* Prevent teleportation */

    EnumClassFlagGroup<CurseTraitType> cursed{}; /* Player is cursed */
    EnumClassFlagGroup<CurseSpecialTraitType> cursed_special{}; /* Player is special type cursed */

    BIT_FLAGS levitation{}; /* No damage falling */
    BIT_FLAGS lite{}; /* Permanent light */
    BIT_FLAGS free_act{}; /* Never paralyzed */
    BIT_FLAGS see_inv{}; /* Can see invisible */
    BIT_FLAGS regenerate{}; /* Regenerate hit pts */
    BIT_FLAGS hold_exp{}; /* Resist exp draining */

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

    // インシデント記録（ツリー構造）
    std::map<std::string, int32_t> incident_tree{}; /*!< ツリー構造ID（例: "root/attack/critical"）で記録するインシデントカウント */

    // 死亡情報
    std::string died_from{}; /*!< 何によって殺されたか / What killed the creature */
    MonraceId killer_monrace_id{}; /*!< 死因となったモンスターのID / MonraceId of the killer */

    // 死亡履歴
    std::vector<DeathRecord> death_history{}; /*!< 死亡履歴リスト */

    // フロア情報
    FloorType *current_floor_ptr{}; /*!< 現在所属しているフロアへのポインタ / Current floor pointer */

    /*!<
     * @brief 時限効果管理オブジェクトを取得
     * @return 時限効果管理オブジェクトへの共有ポインタ
     */
    std::shared_ptr<TimedEffects> effects() const;

    /*!
     * @brief ツリー構造インシデント数加算
     * @param incident_id 階層構造のインシデントID（例: "root/attack/critical"）
     * @param num 加算量
     */
    void plus_incident_tree(const std::string &incident_id, int num);

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

    int16_t to_m_chance{}; /* Minusses to cast chance */

    int16_t num_blow[2]{}; /* Number of blows */
    int16_t num_fire{}; /* Number of shots */

    int16_t food{}; /*!< ゲーム中の滋養度の型定義 / Current nutrition */

    POSITION cur_lite{}; /* Radius of lite (if any) */
    POSITION old_lite{}; /* Radius of lite (if any) */

    BIT_FLAGS special_attack{};

    SUB_EXP spell_exp[64]{}; /* Proficiency of spells */
    std::map<ItemKindType, std::array<SUB_EXP, 64>> weapon_exp{}; /* Proficiency of weapons */
    std::map<ItemKindType, std::array<SUB_EXP, 64>> weapon_exp_max{}; /* Maximum proficiency of weapons */
    std::map<PlayerSkillKindType, SUB_EXP> skill_exp{}; /* Proficiency of misc. skill */
    MartialArtsStyleType martial_arts_style{ MartialArtsStyleType::TRADITIONAL }; /* Martial arts fighting style */

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

    BIT_FLAGS spell_learned1{}; /* bit mask of spells learned */
    BIT_FLAGS spell_learned2{}; /* bit mask of spells learned */
    BIT_FLAGS spell_worked1{}; /* bit mask of spells tried and worked */
    BIT_FLAGS spell_worked2{}; /* bit mask of spells tried and worked */
    BIT_FLAGS spell_forgotten1{}; /* bit mask of spells learned but forgotten */
    BIT_FLAGS spell_forgotten2{}; /* bit mask of spells learned but forgotten */
    std::vector<int> spell_order_learned{}; /* order spells learned */

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

    Dice damage_dice_bonus[2]{}; /* Extra damage dice num/sides */

    bool no_flowed{};

    byte tval_xtra{}; /* (Unused)Correct xtra tval */
    ItemKindType tval_ammo{}; /* Correct ammo tval */

    ENERGY energy_use{}; /*!< 直近のターンに消費したエネルギー / Energy use this turn */

    std::string base_name{}; /*!< Stripped version of "player_name" */

protected:
    std::shared_ptr<TimedEffects> timed_effects; /*!< 時限効果管理オブジェクト */

private:
    std::string build_damage_description() const;
    std::string build_attitude_description() const;
    tl::optional<bool> order_pet_named(const CreatureEntity &other) const;
    tl::optional<bool> order_pet_hp(const CreatureEntity &other) const;
};
