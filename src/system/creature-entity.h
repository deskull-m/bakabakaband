#pragma once

#include "artifact/fixed-art-types.h"
#include "combat/martial-arts-style.h"
#include "inventory/inventory-slot-types.h"
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
#include "system/item-entity.h"
#include "system/material-type-definition.h"
#include "system/monster-profile.h"
#include "system/system-variables.h"
#include "util/dice.h"
#include "util/flag-group.h"
#include "util/point-2d.h"
#include <array>
#include <functional>
#include <initializer_list>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <tl/optional.hpp>
#include <utility>
#include <vector>

constexpr int MONSTER_MAXHP = 10000000; //!< モンスターの最大HP

// Forward declarations
class Direction;
class FloorType;
class MonraceDefinition;
struct player_race_info;
struct player_personality;
struct player_class_info;
enum class ElementRealmType;
enum class FixedArtifactId : short;
enum class ItemKindType : short;
enum class MimicKindType;
enum class CurseTraitType;
enum class CurseSpecialTraitType;
enum class ExtendedSlotType : uint8_t;
enum class MonraceId : int16_t;
enum class MonsterAbilityType;
enum class PlayerSkillKindType;
enum class RaceBlowMethodType;
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
 * @details PlayerTypeとモンスターの実装を一元化した基底クラス。
 * 座標、HP、速度、エネルギーなど両者に共通する基本属性を保持する。
 */
class CreatureEntity {
public:
    CreatureEntity();
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
     * @brief クリーチャーの本来の(一時減少前の)最大HPを取得する
     * @return 本来の最大HP (max_maxhp)
     * @details プレイヤー・モンスター共通で「一時的な最大HP減少が一切無い場合の
     *          最大HP」を表す。経験値計算・dealt_damage 上限・捕獲判定など、
     *          一時減少に左右されるべきでない処理はこちらを参照する。
     */
    virtual int get_max_maxhp() const
    {
        return this->max_maxhp;
    }

    /*!
     * @brief 一時的な最大HP減少量を取得する (プレイヤー・モンスター共通の拡張点)
     * @return 現在の最大HP減少量 (>= 0)
     * @details 既定では減少なし (0)。将来「最大HP一時減少」のステータス異常を
     *          追加する際は、本メソッド (あるいは派生クラスでのオーバーライド) が
     *          減少量を返すようにすれば、set_max_hp() / refresh_max_hp() 経由で
     *          maxhp (現在の最大HP) に自動的に反映される。
     */
    virtual int get_maxhp_reduction() const
    {
        return 0;
    }

    /*!
     * @brief 本来の最大HP (max_maxhp) を確定し、現在の最大HP (maxhp) を再計算する
     * @param full_max_hp 一時減少を含まない本来の最大HP
     * @details max_maxhp と maxhp の関係を定義する唯一の窓口。
     *          max_maxhp に full_max_hp を設定し、refresh_max_hp() で
     *          一時減少を差し引いた値を maxhp に反映する。
     */
    void set_max_hp(int full_max_hp);

    /*!
     * @brief 本来の最大HP (max_maxhp) から現在の最大HP (maxhp) を再計算する
     * @details maxhp = max(1, max_maxhp - get_maxhp_reduction()) を反映する。
     *          一時減少量が変化した際に呼ぶ。現在HP (hp) が新しい maxhp を
     *          超える場合は maxhp まで切り詰める。
     */
    void refresh_max_hp();

    /*!
     * @brief 能力値の内部値 (stat_use 等) を adj_* テーブルの索引に変換する
     * @param stat_value 能力値の内部値 (30 = 表示 3.0 〜 STAT_MAX_VALUE)
     * @return 0 〜 STAT_TABLE_SIZE-1 にクランプした索引
     * @details プレイヤー・モンスター共通。表示 3.0 未満や上限超過でも
     *          配列範囲外参照を起こさないようガードを内蔵する。
     */
    static int stat_value_to_table_index(int stat_value);

    /*!
     * @brief 耐久 (CON) に基づく最大HP補正値を計算する (プレイヤー・モンスター共通)
     * @return レベルと CON に応じた加算HP
     * @details adj_con_mhp テーブルを CON の内部値から引き、(値 - 128) * レベル / 4 を返す。
     *          索引算出には stat_value_to_table_index() のガードを用いる。
     */
    int calc_max_hp_con_bonus() const;

    /*!
     * @brief 最大HPの下限値を返す (プレイヤー・モンスター共通)
     * @return レベル + 1
     * @details 各種補正の結果が極端に小さくなっても、最低でもこの値を最大HPとして保証する。
     */
    int calc_min_max_hp() const
    {
        return this->get_level() + 1;
    }

    /*!
     * @brief 一時的な状態による最大HP補正値を計算する (プレイヤー・モンスター共通)
     * @return 加算HP (英雄化 +10 / 狂戦士化 +30 / つよしスペシャル +50 /
     *         呪術 HEX_XTRA_MIGHT +15 / HEX_BUILDING +60 の合計)
     * @details 呪術 (HEX) はプレイヤー専用のため、モンスターでは spell_hex_data が
     *          無く常に 0 となる。英雄化等の時限効果は timed_effects_map で共通管理。
     */
    int calc_max_hp_status_bonus();

    /*!
     * @brief レベル別HPテーブル hp_table[] を hit_dice から振り直す (プレイヤー・モンスター共通)
     * @details Lv1 は hit_dice の最大値 + 3 回ロール、以降は各レベルで hit_dice を
     *          1 回ずつ累積する。最終レベルのHPが期待値の 75%〜125% に収まるまで
     *          振り直す。呼出側で事前に hit_dice を設定しておくこと。
     *          UI 更新 (再描画・体力ランク表示等) は行わない純粋なロール処理。
     */
    void roll_hp_table();

    /*!
     * @brief 敵モンスターの最大HPをレベル別HPテーブル経由で算出する (スケール保存)
     * @param force_max 各レベルのロールを最大値で固定するか (FORCE_MAXHP 用)
     * @return 実効レベルにおける基礎最大HP (hp_table[level-1] 相当)
     * @details モンスター種族の全HPダイス (hit_dice) の期待値を実効レベル
     *          (monrace.level/2) に均等配分する per-level ダイス (1d s) を導き、
     *          1 レベルずつ累積して hp_table[] を埋める。s を較正することで累積HPの
     *          期待値が従来の単発ロール (hit_dice.roll()) と一致し、分散のみ低下する
     *          (より安定したHP)。プレイヤー用 roll_hp_table() の前方加重 (Lv1 maxroll
     *          + 3 ロール) は敵HPのスケールを膨らませるため用いない。呼出側で事前に
     *          hit_dice にモンスター種族のダイスを設定しておくこと。
     */
    int roll_monster_hp_table(bool force_max);

    /*!
     * @brief 敵モンスターのレベルアップに伴い hp_table[] を上の添字へ伸ばし最大HPを成長させる
     * @param new_level 成長後の実効レベル ([1, PY_MAX_LEVEL] にクランプ)
     * @details 生成時と同じ per-level ダイスで hp_table を旧レベルから new_level まで累積し、
     *          基礎HPの増分と CON 補正の増分を現在の最大HPに加算する (加算的成長)。
     *          set_level() で実効レベルも更新する。new_level が現レベル以下なら何もしない。
     *          モンスター専用 (プレイヤーは通常のレベルアップ経路で成長する)。
     */
    void grow_hp_table_to_level(int new_level);

    /*!
     * @brief 敵モンスターのレベルアップに伴い能力値を成長させる (提案C2)
     * @param levels_gained 今回獲得したレベル数 (正のときのみ成長)
     * @details 6 能力値それぞれに `levels_gained * 成長量` を加算し STAT_MAX_VALUE で
     *          クランプする。stat_max_max / stat_cur / stat_use も同期する。
     *          成長量は保守的な既定値でバランス調整用の定数として実装に持つ。
     *          呼出は grows_stats フラグの立つモンスターのレベルアップ時に限る。
     */
    void grow_stats_by_levels(int levels_gained);

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
     * @brief クリーチャーが所属するフロアを設定
     * @param floor フロアへのポインタ
     */
    void set_floor(FloorType *floor)
    {
        this->current_floor_ptr = floor;
    }

    /*!
     * @brief クリーチャーの実種族定義を取得する
     * @return 実種族定義への参照（r_idx が MonraceId::PLAYER の場合はプレイヤー種族エントリを返す）
     */
    MonraceDefinition &get_monrace() const;

    /*!
     * @brief クリーチャーの実種族定義を shared_ptr で取得する
     * @return 実種族定義への shared_ptr
     */
    std::shared_ptr<MonraceDefinition> get_monrace_shared();
    std::shared_ptr<const MonraceDefinition> get_monrace_shared() const;

    /*!
     * @brief クリーチャーの外見種族定義を取得する
     * @return 外見種族定義への参照（通常は get_monrace() と同じ、変身・誤認時は異なる）
     */
    MonraceDefinition &get_apparent_monrace() const;
    std::shared_ptr<MonraceDefinition> get_apparent_monrace_shared();
    std::shared_ptr<const MonraceDefinition> get_apparent_monrace_shared() const;

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

    /*!
     * @brief クリーチャーの性別表記情報を取得する
     * @return 性別表記情報への参照
     * @details プレイヤーは psex フィールドをそのまま用いる。
     * モンスターの場合は種族定義の MonsterSex から player_sex_type へマップする。
     */
    const player_sex_type &get_sex_info() const;

    /*!
     * @brief 性別 enum を取得する
     * @return psex (player_sex)
     * @details モンスターは生成途中で SEX_NONE → kind_flags MALE/FEMALE から
     * 確定値に置換される。プレイヤーは birth で確定済み。
     */
    virtual player_sex get_psex() const
    {
        return this->psex;
    }

    /*!
     * @brief 性格 enum を取得する
     * @return ppersonality (player_personality_type)
     * @details モンスターは init_monster_profile() で PERSONALITY_NONE に
     * 初期化されており、種族側に性格設定が無い限り NONE のまま。
     */
    virtual player_personality_type get_ppersonality() const
    {
        return this->ppersonality;
    }

    /*!
     * @brief 種族 enum を取得する (提案 1/2)
     * @details モンスターは init_monster_profile() で PlayerRaceType::NONE に
     * 初期化されており、種族効果は発動しない (NONE ガード前提)。
     */
    virtual PlayerRaceType get_prace() const
    {
        return this->prace;
    }

    /*!
     * @brief 職業 enum を取得する (提案 1/2)
     * @details モンスターは init_monster_profile() で PlayerClassType::NONE に
     * 初期化されており、職業効果は発動しない (NONE ガード前提)。
     */
    virtual PlayerClassType get_pclass() const
    {
        return this->pclass;
    }

    /*!
     * @brief 第 1 魔法領域 enum を取得する (提案 1/2)
     * @details モンスターは init_monster_profile() で RealmType::NONE に
     * 初期化されており、魔法領域効果は発動しない (NONE ガード前提)。
     */
    virtual RealmType get_realm1() const
    {
        return this->realm1;
    }

    /*!
     * @brief 第 2 魔法領域 enum を取得する (提案 1/2)
     */
    virtual RealmType get_realm2() const
    {
        return this->realm2;
    }

    /*!
     * @brief 元素使い領域 enum を取得する (提案 1/2)
     */
    virtual ElementRealmType get_element_realm() const
    {
        return this->element_realm;
    }

    /*!
     * @brief カオスパトロン ID を取得する (提案 1/2)
     * @details プレイヤーのカオス戦士のみ意味を持つ。モンスターは 0。
     */
    virtual int16_t get_patron() const
    {
        return this->patron;
    }

    // [提案 1/2] プレイヤー専用フィールド setter virtual 群。
    // 主に birth / wizard / shape-changer 経路から呼ばれる。
    virtual void set_psex(player_sex value)
    {
        this->psex = value;
    }
    virtual void set_ppersonality(player_personality_type value)
    {
        this->ppersonality = value;
    }
    virtual void set_prace(PlayerRaceType value)
    {
        this->prace = value;
    }
    virtual void set_pclass(PlayerClassType value)
    {
        this->pclass = value;
    }
    virtual void set_realm1(RealmType value)
    {
        this->realm1 = value;
    }
    virtual void set_realm2(RealmType value)
    {
        this->realm2 = value;
    }
    virtual void set_element_realm(ElementRealmType value)
    {
        this->element_realm = value;
    }
    virtual void set_patron(int16_t value)
    {
        this->patron = value;
    }

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

    /*! @brief 次の行動までに必要なエネルギーを加算する (A-2) */
    virtual void add_energy_need(ACTION_ENERGY delta)
    {
        this->energy_need += delta;
    }

    /*! @brief 次の行動までに必要なエネルギーを減算する (A-2) */
    virtual void sub_energy_need(ACTION_ENERGY delta)
    {
        this->energy_need -= delta;
    }

    /*!
     * @brief 速度に応じてターン経過分のエネルギーを消費する (提案 B2)
     * @param speed 適用する速度値 (プレイヤーは自速度、モンスターは騎乗時搭乗者速度/通常は一時速度)
     * @details `sub_energy_need(speed_to_energy(speed))` を 1 箇所に集約した
     * プレイヤー・モンスター共通のターンエネルギー消費プリミティブ。速度の決定
     * (騎乗・一時速度等) は呼出側に残す。定義は creature-entity.cpp (speed-table.h
     * の include を本ヘッダに持ち込まないため)。
     */
    void consume_energy_by_speed(int speed);

    /*!
     * @brief クリーチャーのレベルを取得
     * @return レベル値。個体レベルが設定されていればそれを返し、未設定なら種族レベルの半分を返す。
     */
    virtual PLAYER_LEVEL get_level() const;

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
     * @brief 防御（無敵・幽体化・分身等）による被ダメージ軽減を計算する
     * @param damage 元のダメージ量（参照渡し。関数内で軽減後の値に書き換えられる）
     * @param damage_type ダメージ種別（DAMAGE_ATTACK 等）
     * @return true ならダメージが完全吸収された（呼び出し元は以降の処理をスキップすべき）
     * @details
     * プレイヤー側では無敵・幻影・幽体化・無想の構え等による軽減ロジックを実行し、
     * メッセージ表示も行う（そのため非 const）。
     * モンスターはデフォルト実装（軽減なし）で十分。
     */
    virtual bool calc_damage_reduction(int &damage, [[maybe_unused]] int damage_type)
    {
        (void)damage;
        return false;
    }

    /*!
     * @brief ダメージをHPに適用する共通処理
     * @param damage 適用するダメージ量
     * @details hp を減算し -9999 でクランプ、on_take_hit() を呼ぶ。
     * プレイヤー・モンスター両者の基本的なダメージ算術を統一する。
     */
    void apply_raw_damage(int damage)
    {
        this->hp -= damage;
        if (this->hp < -9999) {
            this->hp = -9999;
        }
        if (damage > 0) {
            this->on_take_hit(damage);
        }
    }

    /*!
     * @brief HP を回復する共通プリミティブ (提案 B5)
     * @param amount 回復量
     * @details `apply_raw_damage()` と対称な回復側プリミティブ。現在 HP に
     * amount を加え、現在の最大 HP (maxhp) でクランプする。プレイヤー・
     * モンスターで散在していた `hp += X; if (hp > maxhp) hp = maxhp;` /
     * `hp = std::min(hp + X, maxhp)` を一本化する。
     */
    void heal_hp(int amount)
    {
        this->hp += amount;
        if (this->hp > this->maxhp) {
            this->hp = this->maxhp;
        }
    }

    /*!
     * @brief クリーチャーの時限効果の残りターン数を取得
     * @param effect 取得する時限効果の種別
     * @return 残りターン数（0なら効果なし）
     * @details デフォルト実装は CreatureEntity::timed_effects_map を参照する。
     *          PlayerType は STUN / CONFUSION / FEAR / ACCELERATION / DECELERATION /
     *          PARALYSIS / BLINDNESS に限り TimedEffects オブジェクト経由で管理し、
     *          それ以外はデフォルト実装（= map）へフォールバックする。
     */
    virtual short get_timed_effect(CreatureTimedEffect effect) const;

    /*!
     * @brief クリーチャーの時限効果の残りターン数を直接設定する（セーブ/ロード・内部操作用）
     * @param effect 設定する時限効果の種別
     * @param value 設定するターン数
     * @note メッセージや副作用は発生しない。ゲームロジックからの呼び出しには専用セッターを使うこと。
     * @details デフォルト実装は CreatureEntity::timed_effects_map を更新する。
     *          PlayerType は get_timed_effect と同様に特定の効果だけ TimedEffects 経由。
     */
    virtual void set_timed_effect(CreatureTimedEffect effect, short value);

    /*!
     * @brief モンスター自身のフロア上インデックス (m_idx) を、配置先グリッドから導出して返す
     * @return 配置済みモンスターなら m_idx、プレイヤー・未配置・フロア未設定なら 0
     * @details モンスターは自身の m_idx を保持しないが、floor.m_list 上のインデックスは
     *          配置先グリッドの m_idx と一致するため、現在位置のグリッドから導出できる。
     *          時限効果の mproc キャッシュ保守 (set_timed_effect) 等で自己参照に用いる。
     */
    MONSTER_IDX get_self_m_idx() const;

    /*!
     * @brief 自然回復をスキップすべき状態か（侍 KOUKIJIN 構え・早駆け中等）を判定
     * @return スキップすべきなら true。デフォルトは false（モンスターは常に回復対象）
     * @details PlayerType でオーバーライドし、KOUKIJIN 構え・HAYAGAKE 行動を判定する
     */
    virtual bool should_skip_natural_regen() const
    {
        return false;
    }

    /*!
     * @brief 自然回復ベース量を取得（満腹度等の影響を反映）
     * @return ベース回復量。デフォルトは PY_REGEN_NORMAL
     * @details PlayerType でオーバーライドし、PY_FOOD_WEAK / FAINT / STARVE による減衰を適用する
     */
    virtual int get_base_natural_regen_amount() const;

    /*!
     * @brief 構え・呪いによる回復量補正を適用
     * @param amount 補正前の回復量
     * @return 補正後の回復量。デフォルトは引数をそのまま返す
     * @details PlayerType でオーバーライドし、僧/侍構え・SLOW_REGEN 呪いによる減衰を適用する
     */
    virtual int apply_state_regen_modifier(int amount) const
    {
        return amount;
    }

    /*!
     * @brief クリーチャー固有要因による最終回復量補正を適用
     * @param amount 補正前の回復量
     * @return 補正後の回復量。デフォルトは引数をそのまま返す
     * @details PlayerType でオーバーライドし、ミュータント体質 (mutant_regenerate_mod) による補正を適用する
     */
    virtual int apply_creature_specific_regen_modifier(int amount) const
    {
        return amount;
    }

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

    short get_remaining_oppose_acid() const
    {
        return this->get_timed_effect(CreatureTimedEffect::OPPOSE_ACID);
    }

    short get_remaining_oppose_elec() const
    {
        return this->get_timed_effect(CreatureTimedEffect::OPPOSE_ELEC);
    }

    short get_remaining_oppose_fire() const
    {
        return this->get_timed_effect(CreatureTimedEffect::OPPOSE_FIRE);
    }

    short get_remaining_oppose_cold() const
    {
        return this->get_timed_effect(CreatureTimedEffect::OPPOSE_COLD);
    }

    short get_remaining_oppose_pois() const
    {
        return this->get_timed_effect(CreatureTimedEffect::OPPOSE_POIS);
    }

    short get_remaining_cut() const
    {
        return this->get_timed_effect(CreatureTimedEffect::CUT);
    }

    short get_remaining_poison() const
    {
        return this->get_timed_effect(CreatureTimedEffect::POISON);
    }

    short get_remaining_blindness() const
    {
        return this->get_timed_effect(CreatureTimedEffect::BLINDNESS);
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
     * @brief クリーチャーが幻覚状態かどうかを判定
     * @return 幻覚状態ならtrue（モンスターは常にfalse）
     */
    virtual bool is_hallucinated() const;

    /*!
     * @brief クリーチャーが麻痺しているかどうかを判定
     * @return 麻痺していればtrue（モンスターは常にfalse）
     */
    virtual bool is_paralyzed() const;

    /*!
     * @brief クリーチャーが切り傷状態かどうかを判定
     * @return 切り傷を受けていればtrue
     */
    virtual bool is_cut() const;

    /*!
     * @brief クリーチャーが毒状態かどうかを判定
     * @return 毒に侵されていればtrue
     */
    virtual bool is_poisoned() const;

    virtual bool is_protected_from_evil() const;

    virtual int get_stun_magic_chance_penalty() const;
    virtual int get_stun_item_chance_penalty() const;
    virtual short get_stun_damage_penalty() const;
    virtual std::pair<TERM_COLOR, std::string> get_stun_expr() const;
    virtual std::pair<TERM_COLOR, std::string> get_cut_expr() const;
    virtual int get_cut_damage_per_turn() const;

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
     * @brief クリーチャーがマップ上で視認可能（プレイヤーから見える）かどうかを判定
     * @return 視認可能ならtrue
     * @details モンスターは MonsterProfile::ml の値、プレイヤーや MonsterProfile を
     *          持たないクリーチャーは false（プレイヤー自身を「視認対象」として
     *          扱わないことで既存の `is_seen()` 等の意味論を維持）
     */
    virtual bool is_visible_on_map() const
    {
        return this->has_monster_profile() && this->get_monster_profile().ml;
    }

    // [提案 14] AI ターゲット選定の共通化
    using CreaturePredicate = std::function<bool(const CreatureEntity &)>;

    /*!
     * @brief このクリーチャーから見て最寄りで条件を満たすクリーチャーを探す
     * @param predicate 候補クリーチャーの判定関数
     * @param require_projectable true なら projectable な相手のみを候補に含める
     * @return 最寄りクリーチャーの m_idx (0 = 該当なし)
     */
    MONSTER_IDX find_nearest_creature(const CreaturePredicate &predicate, bool require_projectable = false) const;

    /*!
     * @brief このクリーチャーから見て条件を満たすクリーチャーが存在するかチェック
     * @param predicate 候補クリーチャーの判定関数
     * @return 1 体でも条件を満たすクリーチャーがあれば true
     */
    bool has_visible_creature(const CreaturePredicate &predicate) const;

    /*!
     * @brief このクリーチャーから見て条件を満たすクリーチャーの m_idx 一覧を返す
     * @param predicate 候補クリーチャーの判定関数
     * @return 該当する m_idx のベクタ (空ならなし)
     */
    std::vector<MONSTER_IDX> collect_creatures(const CreaturePredicate &predicate) const;

    /*!
     * @brief このクリーチャーの所持品にアイテムを格納する
     * @param item 格納するアイテム (内部でクローンされる)
     * @return 格納されたインベントリスロットID、失敗時 -1
     * @details inventory[INVEN_PACK] の空きスロットに対しスタック吸収または新規格納を行う。
     *          プレイヤー・モンスター共通 API として利用する。
     *          実装は free function `store_item_to_inventory()` (inventory/inventory-object.cpp)
     *          に委譲する。
     */
    int16_t store_item(const ItemEntity &item);

    /*!
     * @brief このクリーチャーの所持品にアイテムを格納可能か判定する (提案 17)
     * @param item 判定対象のアイテム
     * @return 格納可能なら true
     * @details 既存の `check_store_item_to_inventory()` への薄いラッパ。
     */
    bool can_store_item(const ItemEntity &item) const;

    /*!
     * @brief 装備可能なら装備スロットへ、不可ならパックへアイテムを格納する
     * @param item 格納するアイテム (内部でクローンされる)
     * @return 格納されたインベントリスロットID、失敗時 -1
     * @details `wield_slot()` で得られた装備スロットが空かつアイテムが単体 (number==1) の
     *          場合は装備スロットへ直接装着し equip_cnt をインクリメント。
     *          そうでない場合は store_item() でパックに格納。
     *          モンスター pickup・窃取等で「拾う or 奪う」と「装備する」を一括で行うため。
     */
    int16_t acquire_item(const ItemEntity &item);

    /*!
     * @brief このクリーチャーの所持品全てを近隣にドロップする
     * @param dropper ドロップ処理の主体 (フロア・UI 文脈の参照クリーチャー)
     * @details inventory[INVEN_TOTAL] を走査し各有効アイテムを drop_near() でドロップ。
     *          drop 後 inventory[] を wipe し inven_cnt / equip_cnt を 0 にリセット。
     *          モンスター死亡時のアイテムドロップに使用される。
     */
    void drop_all_inventory(CreatureEntity &dropper);

    /*!
     * @brief クリーチャーのマップ上視認状態を設定する
     * @param value 視認状態
     * @details モンスターのみ意味を持つ。プレイヤーには無効。
     */
    virtual void set_visible_on_map(bool value)
    {
        if (this->has_monster_profile()) {
            this->get_monster_profile().ml = value;
        }
    }

    /*!
     * @brief クリーチャーが所属するアライアンスを取得する
     * @return アライアンス種別。デフォルトは AllianceType::NONE（無所属）
     */
    virtual AllianceType get_alliance_idx() const
    {
        return this->has_monster_profile() ? this->get_monster_profile().alliance_idx : AllianceType::NONE;
    }

    /*!
     * @brief クリーチャーのサブアライメント（中立モンスターが召喚主の影響で一時的に持つ陣営）を取得する
     * @return サブアライメントフラグ。デフォルトは SUB_ALIGN_NEUTRAL
     */
    virtual BIT_FLAGS8 get_sub_align() const
    {
        return this->has_monster_profile() ? this->get_monster_profile().sub_align : static_cast<BIT_FLAGS8>(SUB_ALIGN_NEUTRAL);
    }

    /*!
     * @brief このクリーチャーを召喚した親モンスターのインデックスを取得する
     * @return 親モンスター m_idx。召喚されていない（または不明）なら 0
     */
    virtual MONSTER_IDX get_parent_m_idx() const
    {
        return this->has_monster_profile() ? this->get_monster_profile().parent_m_idx : 0;
    }

    /*!
     * @brief アライアンス所属を設定する (提案 9b)
     * @details モンスター以外（プレイヤー）に対する呼出は無視される
     */
    virtual void set_alliance_idx(AllianceType alliance)
    {
        if (this->has_monster_profile()) {
            this->get_monster_profile().alliance_idx = alliance;
        }
    }

    /*!
     * @brief サブアライメントを設定する (提案 9b)
     */
    virtual void set_sub_align(BIT_FLAGS8 sub_align)
    {
        if (this->has_monster_profile()) {
            this->get_monster_profile().sub_align = sub_align;
        }
    }

    /*!
     * @brief サブアライメントに特定ビットを追加する (提案 9b)
     */
    virtual void add_sub_align(BIT_FLAGS8 mask)
    {
        if (this->has_monster_profile()) {
            this->get_monster_profile().sub_align |= mask;
        }
    }

    /*!
     * @brief 親モンスター m_idx を設定する (提案 9b)
     */
    virtual void set_parent_m_idx(MONSTER_IDX m_idx)
    {
        if (this->has_monster_profile()) {
            this->get_monster_profile().parent_m_idx = m_idx;
        }
    }

    /*!
     * @brief smart_learn フラグに 1 ビット追加する (提案 9b)
     */
    virtual void add_smart_flag(MonsterSmartLearnType flag)
    {
        if (this->has_monster_profile()) {
            this->get_monster_profile().smart.set(flag);
        }
    }

    /*!
     * @brief smart_learn フラグを全クリアする (提案 9b)
     */
    virtual void clear_smart_flags()
    {
        if (this->has_monster_profile()) {
            this->get_monster_profile().smart.clear();
        }
    }

    /*!
     * @brief 変身先モンスター種族 ID を取得する (提案 19)
     */
    virtual MonraceId get_transform_r_idx() const
    {
        return this->has_monster_profile() ? this->get_monster_profile().transform_r_idx : MonraceId::PLAYER;
    }

    /*!
     * @brief 変身先モンスター種族 ID を設定する (提案 19)
     */
    virtual void set_transform_r_idx(MonraceId new_r_idx)
    {
        if (this->has_monster_profile()) {
            this->get_monster_profile().transform_r_idx = new_r_idx;
        }
    }

    /*!
     * @brief 変身する HP 閾値 (最大 HP の %) を取得する (提案 19)
     */
    virtual PERCENTAGE get_transform_hp_threshold() const
    {
        return this->has_monster_profile() ? this->get_monster_profile().transform_hp_threshold : 0;
    }

    /*!
     * @brief 変身する HP 閾値を設定する (提案 19)
     */
    virtual void set_transform_hp_threshold(PERCENTAGE threshold)
    {
        if (this->has_monster_profile()) {
            this->get_monster_profile().transform_hp_threshold = threshold;
        }
    }

    /*!
     * @brief 変身済みかどうか (提案 19)
     */
    virtual bool has_transformed() const
    {
        return this->has_monster_profile() && this->get_monster_profile().has_transformed;
    }

    /*!
     * @brief 変身済みフラグを設定する (提案 19)
     */
    virtual void set_has_transformed(bool transformed)
    {
        if (this->has_monster_profile()) {
            this->get_monster_profile().has_transformed = transformed;
        }
    }

    /*!
     * @brief 自壊までの残りターン数を取得する (提案 19)
     */
    virtual int get_death_count() const
    {
        return this->has_monster_profile() ? this->get_monster_profile().death_count : 0;
    }

    /*!
     * @brief 自壊までの残りターン数を設定する (提案 19)
     */
    virtual void set_death_count(int new_count)
    {
        if (this->has_monster_profile()) {
            this->get_monster_profile().death_count = new_count;
        }
    }

    /*!
     * @brief 自壊までの残りターン数を 1 減らす (提案 19)
     * @return 減算後の値
     */
    virtual int decrement_death_count()
    {
        if (this->has_monster_profile()) {
            return --this->get_monster_profile().death_count;
        }
        return 0;
    }

    /*!
     * @brief MonsterConstantFlagType (mflag2) のチェック共通ヘルパ
     * @details 提案 15: mflag2.has(...) パターンを virtual で集約
     */
    bool has_constant_flag(MonsterConstantFlagType flag) const
    {
        return this->has_monster_profile() && this->get_monster_profile().mflag2.has(flag);
    }

    /*!
     * @brief MonsterConstantFlagType を立てる (提案 18)
     */
    virtual void set_constant_flag(MonsterConstantFlagType flag)
    {
        if (this->has_monster_profile()) {
            this->get_monster_profile().mflag2.set(flag);
        }
    }

    /*!
     * @brief MonsterConstantFlagType を複数まとめて立てる (提案 18)
     */
    virtual void set_constant_flags(std::initializer_list<MonsterConstantFlagType> flags)
    {
        if (this->has_monster_profile()) {
            this->get_monster_profile().mflag2.set(flags);
        }
    }

    /*!
     * @brief MonsterConstantFlagType をクリアする (提案 18)
     */
    virtual void reset_constant_flag(MonsterConstantFlagType flag)
    {
        if (this->has_monster_profile()) {
            this->get_monster_profile().mflag2.reset(flag);
        }
    }

    /*!
     * @brief MonsterConstantFlagType を複数まとめてクリアする (提案 18)
     */
    virtual void reset_constant_flags(std::initializer_list<MonsterConstantFlagType> flags)
    {
        if (this->has_monster_profile()) {
            this->get_monster_profile().mflag2.reset(flags);
        }
    }

    /*!
     * @brief MonsterConstantFlagType を真偽値で設定する (提案 20)
     * @details savefile 復元等で `mflag2[flag] = bool` 風の代入を行う
     *          パターン用。true なら set、false なら reset。
     */
    virtual void assign_constant_flag(MonsterConstantFlagType flag, bool value)
    {
        if (!this->has_monster_profile()) {
            return;
        }
        if (value) {
            this->get_monster_profile().mflag2.set(flag);
        } else {
            this->get_monster_profile().mflag2.reset(flag);
        }
    }

    /*!
     * @brief MonsterConstantFlagType (mflag2) をすべてクリアする (提案 20)
     */
    virtual void clear_constant_flags()
    {
        if (this->has_monster_profile()) {
            this->get_monster_profile().mflag2.clear();
        }
    }

    /*!
     * @brief mflag2 全体を読み取る (提案 20)
     * @details モンスターボール捕獲・解放等で mflag2 ビット集合を
     *          まるごとコピーする用途。プレイヤーは空集合を返す。
     */
    virtual const EnumClassFlagGroup<MonsterConstantFlagType> &get_all_constant_flags() const
    {
        static const EnumClassFlagGroup<MonsterConstantFlagType> empty{};
        return this->has_monster_profile() ? this->get_monster_profile().mflag2 : empty;
    }

    /*!
     * @brief mflag2 全体を上書きする (提案 20)
     */
    virtual void set_all_constant_flags(const EnumClassFlagGroup<MonsterConstantFlagType> &flags)
    {
        if (this->has_monster_profile()) {
            this->get_monster_profile().mflag2 = flags;
        }
    }

    /*!
     * @brief MonsterTemporaryFlagType (mflag) をすべてクリアする (提案 20)
     */
    virtual void clear_temporary_flags()
    {
        if (this->has_monster_profile()) {
            this->get_monster_profile().mflag.clear();
        }
    }

    /*!
     * @brief MonsterTemporaryFlagType (mflag) のチェック共通ヘルパ (提案 16)
     */
    bool has_temporary_flag(MonsterTemporaryFlagType flag) const
    {
        return this->has_monster_profile() && this->get_monster_profile().mflag.has(flag);
    }

    /*!
     * @brief MonsterTemporaryFlagType を立てる (提案 16)
     */
    virtual void set_temporary_flag(MonsterTemporaryFlagType flag)
    {
        if (this->has_monster_profile()) {
            this->get_monster_profile().mflag.set(flag);
        }
    }

    /*!
     * @brief MonsterTemporaryFlagType をクリアする (提案 16)
     */
    virtual void reset_temporary_flag(MonsterTemporaryFlagType flag)
    {
        if (this->has_monster_profile()) {
            this->get_monster_profile().mflag.reset(flag);
        }
    }

    /*! @brief プレイヤーの視界内 (VIEW) にいるか (提案 16) */
    virtual bool is_in_view() const
    {
        return this->has_temporary_flag(MonsterTemporaryFlagType::VIEW);
    }

    /*! @brief project_all_los の対象 (LOS) としてマークされているか */
    virtual bool is_marked_for_los() const
    {
        return this->has_temporary_flag(MonsterTemporaryFlagType::LOS);
    }

    /*! @brief ESP で感知されているか */
    virtual bool is_sensed_by_esp() const
    {
        return this->has_temporary_flag(MonsterTemporaryFlagType::ESP);
    }

    /*! @brief ターン開始時にフロアにいたか (PRESENT_AT_TURN_START) */
    virtual bool was_present_at_turn_start() const
    {
        return this->has_temporary_flag(MonsterTemporaryFlagType::PRESENT_AT_TURN_START);
    }

    /*! @brief 反魔法状態 (PREVENT_MAGIC) か */
    virtual bool has_prevent_magic() const
    {
        return this->has_temporary_flag(MonsterTemporaryFlagType::PREVENT_MAGIC);
    }

    /*! @brief 正気喪失効果 (SANITY_BLAST) を持つか */
    virtual bool has_sanity_blast() const
    {
        return this->has_temporary_flag(MonsterTemporaryFlagType::SANITY_BLAST);
    }

    /*!
     * @brief モンスター実種族 ID を取得する (提案 28)
     * @details プレイヤーは MonraceId::PLAYER を保持。
     */
    virtual MonraceId get_r_idx() const
    {
        return this->r_idx;
    }

    /*!
     * @brief モンスター外見種族 ID を取得する (提案 28)
     */
    virtual MonraceId get_ap_r_idx() const
    {
        return this->ap_r_idx;
    }

    /*!
     * @brief 騎乗中のモンスター m_idx を取得する (提案 28)
     */
    virtual MONSTER_IDX get_riding() const
    {
        return this->riding;
    }

    /*!
     * @brief モンスター実種族 ID を設定する (提案 22)
     * @details ap_r_idx は変更しない。両方更新したい場合は polymorph_to() を使う。
     */
    virtual void set_r_idx(MonraceId new_r_idx)
    {
        this->r_idx = new_r_idx;
    }

    /*!
     * @brief モンスター外見種族 ID を設定する (提案 22)
     */
    virtual void set_ap_r_idx(MonraceId new_ap_r_idx)
    {
        this->ap_r_idx = new_ap_r_idx;
    }

    /*!
     * @brief 実種族と外見種族をまとめて設定する (提案 22)
     * @details polymorph / 進化 / 変身などで両者を同期させたいときに使用。
     */
    virtual void polymorph_to(MonraceId new_r_idx)
    {
        this->r_idx = new_r_idx;
        this->ap_r_idx = new_r_idx;
    }

    void increment_seen_count() const;

    /*!
     * @brief 騎乗中のモンスター m_idx を直接設定する (提案 22)
     * @details ride_monster() と異なり mflag2/RIDING の更新を行わない低レベル
     *          setter。compaction で m_idx 圧縮時に既に立っている RIDING を
     *          別 idx へ付け替える等の用途。通常の騎乗開始/終了処理は
     *          ride_monster() を使用すること。
     */
    virtual void set_riding(MONSTER_IDX m_idx)
    {
        this->riding = m_idx;
    }

    // [提案 40] ペット関連フィールドの virtual API。
    // pet_extra_flags は BIT_FLAGS16 のビットマスク (`PF_OPEN_DOORS` 等) で、
    // 単一フラグ操作は add/remove/has、一括代入は set/get_X_flags 経由。
    virtual bool has_pet_extra_flag(BIT_FLAGS16 flag) const
    {
        return (this->pet_extra_flags & flag) != 0;
    }
    virtual void add_pet_extra_flag(BIT_FLAGS16 flag)
    {
        this->pet_extra_flags |= flag;
    }
    virtual void remove_pet_extra_flag(BIT_FLAGS16 flag)
    {
        this->pet_extra_flags &= static_cast<BIT_FLAGS16>(~flag);
    }
    virtual BIT_FLAGS16 get_pet_extra_flags() const
    {
        return this->pet_extra_flags;
    }
    virtual void set_pet_extra_flags(BIT_FLAGS16 value)
    {
        this->pet_extra_flags = value;
    }
    virtual int16_t get_pet_follow_distance() const
    {
        return this->pet_follow_distance;
    }
    virtual void set_pet_follow_distance(int16_t value)
    {
        this->pet_follow_distance = value;
    }
    virtual MONSTER_IDX get_pet_t_m_idx() const
    {
        return this->pet_t_m_idx;
    }
    virtual void set_pet_t_m_idx(MONSTER_IDX value)
    {
        this->pet_t_m_idx = value;
    }
    virtual MONSTER_IDX get_riding_t_m_idx() const
    {
        return this->riding_t_m_idx;
    }
    virtual void set_riding_t_m_idx(MONSTER_IDX value)
    {
        this->riding_t_m_idx = value;
    }

    // [提案 45] ペット追従先 (pet_t_m_idx) / 騎乗ターゲット (riding_t_m_idx) の
    // ターゲットモンスター idx 保守を集約する共通操作。モンスター index が
    // 除去・付替え・全消去される際に両ターゲットを一貫更新する。従来は
    // monster-remover / monster-compaction / dungeon-processor に同一ロジックが
    // 分散していた不変条件を 1 箇所に集約したもの。
    void reset_pet_riding_targets()
    {
        this->set_pet_t_m_idx(0);
        this->set_riding_t_m_idx(0);
    }
    void clear_pet_riding_targets_pointing_to(MONSTER_IDX m_idx)
    {
        if (this->get_pet_t_m_idx() == m_idx) {
            this->set_pet_t_m_idx(0);
        }
        if (this->get_riding_t_m_idx() == m_idx) {
            this->set_riding_t_m_idx(0);
        }
    }
    void remap_pet_riding_targets(MONSTER_IDX from, MONSTER_IDX to)
    {
        if (this->get_pet_t_m_idx() == from) {
            this->set_pet_t_m_idx(to);
        }
        if (this->get_riding_t_m_idx() == from) {
            this->set_riding_t_m_idx(to);
        }
    }

    // [提案 42] 差分検出キャッシュ (old_*) の virtual API。
    // update_creature() 系で 1 ターン前の値スナップショットを保持し、
    // 状態変化検出 / メッセージ出力に使用される。
    virtual POSITION get_old_lite() const
    {
        return this->old_lite;
    }
    virtual void set_old_lite(POSITION value)
    {
        this->old_lite = value;
    }
    virtual BIT_FLAGS get_old_race_flags1() const
    {
        return this->old_race1;
    }
    virtual void set_old_race_flags1(BIT_FLAGS value)
    {
        this->old_race1 = value;
    }
    virtual BIT_FLAGS get_old_race_flags2() const
    {
        return this->old_race2;
    }
    virtual void set_old_race_flags2(BIT_FLAGS value)
    {
        this->old_race2 = value;
    }
    virtual int16_t get_old_realm() const
    {
        return this->old_realm;
    }
    virtual void set_old_realm(int16_t value)
    {
        this->old_realm = value;
    }
    virtual int16_t get_old_spells() const
    {
        return this->old_spells;
    }
    virtual void set_old_spells(int16_t value)
    {
        this->old_spells = value;
    }
    virtual bool was_cumber_armor() const
    {
        return this->old_cumber_armor;
    }
    virtual void set_was_cumber_armor(bool value)
    {
        this->old_cumber_armor = value;
    }
    virtual bool was_cumber_glove() const
    {
        return this->old_cumber_glove;
    }
    virtual void set_was_cumber_glove(bool value)
    {
        this->old_cumber_glove = value;
    }
    virtual bool was_heavy_wield(int hand) const
    {
        return this->old_heavy_wield[hand];
    }
    virtual void set_was_heavy_wield(int hand, bool value)
    {
        this->old_heavy_wield[hand] = value;
    }
    virtual bool was_heavy_shoot() const
    {
        return this->old_heavy_shoot;
    }
    virtual void set_was_heavy_shoot(bool value)
    {
        this->old_heavy_shoot = value;
    }
    virtual bool was_icky_wield(int hand) const
    {
        return this->old_icky_wield[hand];
    }
    virtual void set_was_icky_wield(int hand, bool value)
    {
        this->old_icky_wield[hand] = value;
    }
    virtual bool was_icky_riding_wield(int hand) const
    {
        return this->old_riding_wield[hand];
    }
    virtual void set_was_icky_riding_wield(int hand, bool value)
    {
        this->old_riding_wield[hand] = value;
    }
    virtual bool was_riding_ryoute() const
    {
        return this->old_riding_ryoute;
    }
    virtual void set_was_riding_ryoute(bool value)
    {
        this->old_riding_ryoute = value;
    }
    virtual bool was_monlite() const
    {
        return this->old_monlite;
    }
    virtual void set_was_monlite(bool value)
    {
        this->old_monlite = value;
    }

    // [提案 43] 行動・状態フラグ群の virtual API。
    // action / running / resting / is_fired / level_up_message /
    // timewalk / now_damaged / playing / leaving / monk_notify_aux /
    // teleport_town / yoiyami / sutemi / fishing_dir を private 化し
    // get/set virtual 経由でアクセス。
    virtual byte get_action() const
    {
        return this->action;
    }
    virtual void set_action(byte value)
    {
        this->action = value;
    }
    virtual int16_t get_running() const
    {
        return this->running;
    }
    virtual void set_running(int16_t value)
    {
        this->running = value;
    }
    virtual GAME_TURN get_resting() const
    {
        return this->resting;
    }
    virtual void set_resting(GAME_TURN value)
    {
        this->resting = value;
    }
    virtual bool is_fired() const
    {
        return this->fired;
    }
    virtual void set_is_fired(bool value)
    {
        this->fired = value;
    }
    virtual bool has_level_up_message() const
    {
        return this->level_up_message;
    }
    virtual void set_level_up_message(bool value)
    {
        this->level_up_message = value;
    }
    virtual bool is_timewalking() const
    {
        return this->timewalk;
    }
    virtual void set_timewalking(bool value)
    {
        this->timewalk = value;
    }
    virtual bool is_now_damaged() const
    {
        return this->now_damaged;
    }
    virtual void set_now_damaged(bool value)
    {
        this->now_damaged = value;
    }
    virtual bool is_playing() const
    {
        return this->playing;
    }
    virtual void set_playing(bool value)
    {
        this->playing = value;
    }
    virtual bool is_leaving() const
    {
        return this->leaving;
    }
    virtual void set_leaving(bool value)
    {
        this->leaving = value;
    }
    virtual bool get_monk_notify_aux() const
    {
        return this->monk_notify_aux;
    }
    virtual void set_monk_notify_aux(bool value)
    {
        this->monk_notify_aux = value;
    }
    virtual bool is_teleport_town() const
    {
        return this->teleport_town;
    }
    virtual void set_teleport_town(bool value)
    {
        this->teleport_town = value;
    }
    virtual BIT_FLAGS get_yoiyami() const
    {
        return this->yoiyami;
    }
    virtual void set_yoiyami(BIT_FLAGS value)
    {
        this->yoiyami = value;
    }
    virtual bool is_sutemi() const
    {
        return this->sutemi;
    }
    virtual void set_sutemi(bool value)
    {
        this->sutemi = value;
    }
    virtual DIRECTION get_fishing_dir() const
    {
        return this->fishing_dir;
    }
    virtual void set_fishing_dir(DIRECTION value)
    {
        this->fishing_dir = value;
    }

    // [提案 47] その他の小規模フィールドの virtual API。
    virtual int32_t get_dealt_damage() const
    {
        return this->dealt_damage;
    }
    virtual void set_dealt_damage(int32_t value)
    {
        this->dealt_damage = value;
    }
    virtual void add_dealt_damage(int32_t delta)
    {
        this->dealt_damage += delta;
    }
    virtual POSITION get_run_py() const
    {
        return this->run_py;
    }
    virtual void set_run_py(POSITION value)
    {
        this->run_py = value;
    }
    virtual POSITION get_run_px() const
    {
        return this->run_px;
    }
    virtual void set_run_px(POSITION value)
    {
        this->run_px = value;
    }
    virtual bool is_vanish_stairs_flag() const
    {
        return this->vanish_stairs_flag;
    }
    virtual void set_vanish_stairs_flag(bool value)
    {
        this->vanish_stairs_flag = value;
    }
    virtual bool is_suppress_multi_reward() const
    {
        return this->suppress_multi_reward;
    }
    virtual void set_suppress_multi_reward(bool value)
    {
        this->suppress_multi_reward = value;
    }
    virtual short get_tracking_bi_id() const
    {
        return this->tracking_bi_id;
    }
    virtual void set_tracking_bi_id(short value)
    {
        this->tracking_bi_id = value;
    }

    // [提案 48] さらなる小規模フィールドの virtual API。
    virtual ItemKindType get_tval_ammo() const
    {
        return this->tval_ammo;
    }
    virtual void set_tval_ammo(ItemKindType value)
    {
        this->tval_ammo = value;
    }
    virtual bool is_dtrap() const
    {
        return this->dtrap;
    }
    virtual void set_dtrap(bool value)
    {
        this->dtrap = value;
    }
    virtual bool is_autopick_autoregister() const
    {
        return this->autopick_autoregister;
    }
    virtual void set_autopick_autoregister(bool value)
    {
        this->autopick_autoregister = value;
    }
    virtual DungeonId get_recall_dungeon() const
    {
        return this->recall_dungeon;
    }
    virtual void set_recall_dungeon(DungeonId value)
    {
        this->recall_dungeon = value;
    }
    virtual ENERGY get_enchant_energy_need() const
    {
        return this->enchant_energy_need;
    }
    virtual void set_enchant_energy_need(ENERGY value)
    {
        this->enchant_energy_need = value;
    }
    virtual void add_enchant_energy_need(ENERGY delta)
    {
        this->enchant_energy_need += delta;
    }
    virtual void sub_enchant_energy_need(ENERGY delta)
    {
        this->enchant_energy_need -= delta;
    }
    virtual ENERGY get_energy_use() const
    {
        return this->energy_use;
    }
    virtual void set_energy_use(ENERGY value)
    {
        this->energy_use = value;
    }
    virtual void add_energy_use(ENERGY delta)
    {
        this->energy_use += delta;
    }
    virtual void sub_energy_use(ENERGY delta)
    {
        this->energy_use -= delta;
    }
    virtual void mul_energy_use(ENERGY factor)
    {
        this->energy_use *= factor;
    }
    virtual void div_energy_use(ENERGY divisor)
    {
        this->energy_use /= divisor;
    }

    /*!
     * @brief パック内の所持品数を inventory[] から計算する (提案 25)
     * @details inventory[0..INVEN_PACK) の有効アイテム数を返す。
     *          以前は `inven_cnt` フィールドにキャッシュしていたが、
     *          inventory[] が単一の真実源となるよう自動計算化。
     */
    short get_inven_cnt() const;

    /*!
     * @brief 装備品数を inventory[] から計算する (提案 25)
     * @details inventory[INVEN_MAIN_HAND..INVEN_TOTAL) の有効アイテム数を返す。
     */
    short get_equip_cnt() const;

    /*!
     * @brief 近接打撃で使用する武器スロットを決定する (提案 B4)
     * @param blow_index 打撃インデックス (二刀流時の交互使用判定に使用)
     * @param method 打撃メソッド
     * @return 使用する武器スロット (INVEN_MAIN_HAND / INVEN_SUB_HAND)、武器なしは -1
     * @details HIT/PUNCH/SLASH/STING の物理打撃で MAIN/SUB の双方が有効な近接武器なら
     *          blow index で交互、片方のみならそちら、武器なしなら -1。モンスター対
     *          プレイヤー／モンスター対モンスターの両攻撃経路で共用する。
     */
    short select_melee_weapon_slot(int blow_index, RaceBlowMethodType method) const;

    /*!
     * @brief 指定スロットに装備可能か判定する (提案: モンスター体構造)
     * @param slot inventory_slot_type (INVEN_MAIN_HAND..INVEN_TOTAL-1)
     * @return 体構造的に装備可能なら true
     * @details プレイヤーは HUMANOID 固定で常に true。モンスターは
     *          MonraceDefinition::body_structure を参照し、
     *          BodySlotPolicy で許可されたスロットのみ true を返す。
     *          slot が範囲外なら false。
     *          docs/monster-body-structure-equipment-slots.md 参照。
     */
    virtual bool can_equip_to(int slot) const;

    /*!
     * @brief 拡張装備スロット数を取得する (Phase 2)
     * @return body_structure に依存する拡張スロット数。プレイヤーは 0。
     */
    virtual size_t get_extended_slot_count() const;

    /*!
     * @brief 指定インデックスの拡張スロットの種別を取得する (Phase 2)
     * @param idx 拡張スロットインデックス (0..get_extended_slot_count()-1)
     * @return 拡張スロット種別。範囲外なら ExtendedSlotType::MAX
     */
    virtual ExtendedSlotType get_extended_slot_type(size_t idx) const;

    /*!
     * @brief 拡張インベントリを初期化する (Phase 2)
     * @details body_structure から得られる拡張スロット数分の
     *          空の ItemEntity を確保する。生成時に呼ぶ。
     */
    void init_extended_inventory();

    /*! @brief 年齢を設定する (提案 24) */
    virtual void set_age(int16_t value)
    {
        this->age = value;
    }

    /*! @brief 年齢を加算する (提案 24) */
    virtual void add_age(int16_t delta)
    {
        this->age += delta;
    }

    /*! @brief 身長を設定する (提案 24) */
    virtual void set_ht(int16_t value)
    {
        this->ht = value;
    }

    /*! @brief 体重を設定する (提案 24) */
    virtual void set_wt(int16_t value)
    {
        this->wt = value;
    }

    /*! @brief 名声を設定する (提案 24) */
    virtual void set_prestige(int16_t value)
    {
        this->prestige = value;
    }

    /*! @brief 名声を加算する (提案 24) */
    virtual void add_prestige(int16_t delta)
    {
        this->prestige += delta;
    }

    /*! @brief 名声を半減する等の比率変更 (提案 24) */
    virtual void divide_prestige(int divisor)
    {
        if (divisor != 0) {
            this->prestige = static_cast<int16_t>(this->prestige / divisor);
        }
    }

    /*! @brief 待ち伏せ状態を設定する (提案 26) */
    virtual void set_ambush_flag(bool value)
    {
        this->ambush_flag = value;
    }

    /*! @brief 滋養度を設定する (提案 26) */
    virtual void set_food(int16_t value)
    {
        this->food = value;
    }

    /*! @brief 現在いる街番号を設定する (提案 26) */
    virtual void set_town_num(int16_t value)
    {
        this->town_num = value;
    }

    /*! @brief レベルを設定する (提案 26) */
    virtual void set_level(int16_t value)
    {
        this->level = value;
    }

    /*! @brief 経験レベル最大値を設定する (提案 27) */
    virtual void set_max_plv(int16_t value)
    {
        this->max_plv = value;
    }

    /*! @brief ミュータント体質による自然回復補正(%)を設定する (A-1) */
    virtual void set_mutant_regenerate_mod(PERCENTAGE value)
    {
        this->mutant_regenerate_mod = value;
    }

    /*! @brief 習得済み呪文数を設定する (A-1) */
    virtual void set_learned_spells(int16_t value)
    {
        this->learned_spells = value;
    }

    /*! @brief 追加習得可能呪文数を設定する (A-1) */
    virtual void set_add_spells(int16_t value)
    {
        this->add_spells = value;
    }

    /*! @brief 二刀流ペナルティ軽減フラグを設定する (A-1) */
    virtual void set_easy_2weapon(BIT_FLAGS value)
    {
        this->easy_2weapon = value;
    }

    /*! @brief 劣化セーヴィングスローフラグを設定する (A-1) */
    virtual void set_down_saving(BIT_FLAGS value)
    {
        this->down_saving = value;
    }

    /*! @brief 最大 MP (max_mp) を設定する (提案 27) */
    virtual void set_max_mp(int value)
    {
        this->max_mp = value;
    }

    /*! @brief 現在の経験値を設定する (提案 27) */
    virtual void set_exp(EXP value)
    {
        this->exp = value;
    }

    /*! @brief 最大経験値を設定する (提案 27) */
    virtual void set_max_exp(EXP value)
    {
        this->max_exp = value;
    }

    /*! @brief 最大の最大経験値を設定する (提案 27) */
    virtual void set_max_max_exp(EXP value)
    {
        this->max_max_exp = value;
    }

    /*! @brief 所持金を設定する (提案 27b) */
    virtual void set_au(int value)
    {
        this->au = value;
    }

    /*! @brief 所持金を加算する (提案 27b) */
    virtual void add_au(int delta)
    {
        this->au += delta;
    }

    /*! @brief 所持金を減算する (提案 27b) */
    virtual void sub_au(int delta)
    {
        this->au -= delta;
    }

    /*! @brief 所持金を比率変更する (提案 27b) */
    virtual void divide_au(int divisor)
    {
        if (divisor != 0) {
            this->au /= divisor;
        }
    }

    /*! @brief 現在の MP を設定する (提案 27b) */
    virtual void set_current_mp(int value)
    {
        this->current_mp = value;
    }

    /*! @brief 現在の MP を加算する (提案 27b) */
    virtual void add_current_mp(int delta)
    {
        this->current_mp += delta;
    }

    /*! @brief 現在の MP を減算する (提案 27b) */
    virtual void sub_current_mp(int delta)
    {
        this->current_mp -= delta;
    }

    /*! @brief 64bit ペア演算で現在 MP を加算する (提案 32b) */
    virtual void add_current_mp_with_frac(int delta, uint32_t delta_frac);

    /*! @brief 64bit ペア演算で現在 MP を減算する (提案 32b) */
    virtual void sub_current_mp_with_frac(int delta, uint32_t delta_frac);

    /*! @brief 64bit ペア演算で経験値を加算する (提案 32b) */
    virtual void add_exp_with_frac(EXP delta, uint32_t delta_frac);

    /*! @brief 所持金を取得する (提案 31) */
    virtual int get_au() const
    {
        return this->au;
    }

    /*! @brief 現在の MP を取得する (提案 31) */
    virtual int get_current_mp() const
    {
        return this->current_mp;
    }

    /*! @brief 滋養度を取得する (提案 31) */
    virtual int16_t get_food() const
    {
        return this->food;
    }

    /*! @brief 現在いる街番号を取得する (提案 31) */
    virtual int16_t get_town_num() const
    {
        return this->town_num;
    }

    /*! @brief 年齢を取得する (提案 31) */
    virtual int16_t get_age() const
    {
        return this->age;
    }

    /*! @brief 身長を取得する (提案 31) */
    virtual int16_t get_ht() const
    {
        return this->ht;
    }

    /*! @brief 体重を取得する (提案 31) */
    virtual int16_t get_wt() const
    {
        return this->wt;
    }

    /*! @brief 名声を取得する (提案 31) */
    virtual int16_t get_prestige() const
    {
        return this->prestige;
    }

    /*! @brief 経験レベル最大値を取得する (提案 31) */
    virtual int16_t get_max_plv() const
    {
        return this->max_plv;
    }

    /*! @brief ミュータント体質による自然回復補正(%)を取得する (A-1) */
    virtual PERCENTAGE get_mutant_regenerate_mod() const
    {
        return this->mutant_regenerate_mod;
    }

    /*! @brief 習得済み呪文数を取得する (A-1) */
    virtual int16_t get_learned_spells() const
    {
        return this->learned_spells;
    }

    /*! @brief 追加習得可能呪文数を取得する (A-1) */
    virtual int16_t get_add_spells() const
    {
        return this->add_spells;
    }

    /*! @brief 二刀流ペナルティ軽減フラグを取得する (A-1) */
    virtual BIT_FLAGS get_easy_2weapon() const
    {
        return this->easy_2weapon;
    }

    /*! @brief 劣化セーヴィングスローフラグを取得する (A-1) */
    virtual BIT_FLAGS get_down_saving() const
    {
        return this->down_saving;
    }

    /*! @brief 自己分析で得た知識フラグを保持しているか (A-2) */
    virtual bool has_knowledge(BIT_FLAGS8 flag) const
    {
        return (this->knowledge & flag) != 0;
    }

    /*! @brief 自己分析で得た知識フラグを追加する (A-2) */
    virtual void add_knowledge(BIT_FLAGS8 flag)
    {
        this->knowledge |= flag;
    }

    /*! @brief 自己分析で得た知識フラグを除去する (A-2) */
    virtual void remove_knowledge(BIT_FLAGS8 flag)
    {
        this->knowledge &= ~flag;
    }

    /*! @brief 自己分析で得た知識フラグ全体を取得する (A-2, savefile 用) */
    virtual BIT_FLAGS8 get_knowledge() const
    {
        return this->knowledge;
    }

    /*! @brief 自己分析で得た知識フラグ全体を設定する (A-2, savefile 用) */
    virtual void set_knowledge(BIT_FLAGS8 value)
    {
        this->knowledge = value;
    }

    /*! @brief 最大 MP (max_mp) を取得する (提案 31) */
    virtual int get_max_mp() const
    {
        return this->max_mp;
    }

    /*! @brief 現在の経験値を取得する (提案 31) */
    virtual EXP get_exp() const
    {
        return this->exp;
    }

    /*! @brief 最大経験値を取得する (提案 31) */
    virtual EXP get_max_exp() const
    {
        return this->max_exp;
    }

    /*! @brief 最大の最大経験値を取得する (提案 31) */
    virtual EXP get_max_max_exp() const
    {
        return this->max_max_exp;
    }

    /*! @brief 経験値を加算する (提案 31) */
    virtual void add_exp(EXP delta)
    {
        this->exp += delta;
    }

    /*! @brief 経験値を減算する (提案 31) */
    virtual void sub_exp(EXP delta)
    {
        this->exp -= delta;
    }

    /*! @brief 最大経験値を加算する (提案 31) */
    virtual void add_max_exp(EXP delta)
    {
        this->max_exp += delta;
    }

    /*! @brief 最大経験値を減算する (提案 31) */
    virtual void sub_max_exp(EXP delta)
    {
        this->max_exp -= delta;
    }

    /*! @brief 待ち伏せ状態を取得する (提案 31) */
    virtual bool get_ambush_flag() const
    {
        return this->ambush_flag;
    }

    /*! @brief 命中ボーナス (近接利き手分以外) を設定する (提案 30) */
    virtual void set_to_h_b(int16_t value)
    {
        this->to_h_b = value;
    }

    /*! @brief 命中ボーナス (その他装備分) を設定する (提案 30) */
    virtual void set_to_h_m(int16_t value)
    {
        this->to_h_m = value;
    }

    /*! @brief ダメージボーナス (その他装備分) を設定する (提案 30) */
    virtual void set_to_d_m(int16_t value)
    {
        this->to_d_m = value;
    }

    /*! @brief AC ボーナスを設定する (提案 30) */
    virtual void set_to_a(int16_t value)
    {
        this->to_a = value;
    }

    /*! @brief 命中ボーナス to_h[hand] を設定する (提案 31b) */
    virtual void set_to_h(int hand, int16_t value)
    {
        this->to_h[hand] = value;
    }

    /*! @brief ダメージボーナス to_d[hand] を設定する (提案 31b) */
    virtual void set_to_d(int hand, int16_t value)
    {
        this->to_d[hand] = value;
    }

    /*! @brief 命中ボーナス (近接利き手分以外) を取得する (提案 31b) */
    virtual int16_t get_to_h_b() const
    {
        return this->to_h_b;
    }

    /*! @brief 命中ボーナス (その他装備分) を取得する (提案 31b) */
    virtual int16_t get_to_h_m() const
    {
        return this->to_h_m;
    }

    /*! @brief ダメージボーナス (その他装備分) を取得する (提案 31b) */
    virtual int16_t get_to_d_m() const
    {
        return this->to_d_m;
    }

    /*! @brief AC ボーナスを取得する (提案 31b) */
    virtual int16_t get_to_a() const
    {
        return this->to_a;
    }

    /*! @brief 命中ボーナス to_h[hand] を取得する (提案 31b) */
    virtual int16_t get_to_h(int hand) const
    {
        return this->to_h[hand];
    }

    /*! @brief ダメージボーナス to_d[hand] を取得する (提案 31b) */
    virtual int16_t get_to_d(int hand) const
    {
        return this->to_d[hand];
    }

    // [提案 34] 表示用既知値 dis_to_h / dis_to_d / dis_to_h_b / dis_to_a / dis_ac
    // の getter / setter virtual。書込は player-status.cpp の update_creature()
    // から、読取は表示系 (display-player-middle / main-window-left-frame /
    // status-first-page / io-dump 等) から行われる。
    virtual HIT_PROB get_dis_to_h(int hand) const
    {
        return this->dis_to_h[hand];
    }
    virtual void set_dis_to_h(int hand, HIT_PROB value)
    {
        this->dis_to_h[hand] = value;
    }
    virtual HIT_PROB get_dis_to_h_b() const
    {
        return this->dis_to_h_b;
    }
    virtual void set_dis_to_h_b(HIT_PROB value)
    {
        this->dis_to_h_b = value;
    }
    virtual int get_dis_to_d(int hand) const
    {
        return this->dis_to_d[hand];
    }
    virtual void set_dis_to_d(int hand, int value)
    {
        this->dis_to_d[hand] = value;
    }
    virtual ARMOUR_CLASS get_dis_to_a() const
    {
        return this->dis_to_a;
    }
    virtual void set_dis_to_a(ARMOUR_CLASS value)
    {
        this->dis_to_a = value;
    }
    virtual ARMOUR_CLASS get_dis_ac() const
    {
        return this->dis_ac;
    }
    virtual void set_dis_ac(ARMOUR_CLASS value)
    {
        this->dis_ac = value;
    }

    // [提案 39] 装備派生キャッシュフィールド (player-status.cpp の update_creature() から
    // 装備状態に応じて再計算される) の getter / setter virtual。書込は主に update_creature()
    // から、読取は戦闘/表示/AI 等から行われる。
    virtual void set_ac(ARMOUR_CLASS value)
    {
        this->ac = value;
    }
    virtual int16_t get_num_blow(int hand) const
    {
        return this->num_blow[hand];
    }
    virtual void set_num_blow(int hand, int16_t value)
    {
        this->num_blow[hand] = value;
    }
    /*! @brief 装備由来の追加攻撃回数を取得する (A-3) */
    virtual int get_extra_blows(int hand) const
    {
        return this->extra_blows[hand];
    }
    /*! @brief 装備由来の追加攻撃回数を設定する (A-3) */
    virtual void set_extra_blows(int hand, int value)
    {
        this->extra_blows[hand] = value;
    }
    /*! @brief 装備由来の追加攻撃回数を加算する (A-3) */
    virtual void add_extra_blows(int hand, int delta)
    {
        this->extra_blows[hand] += delta;
    }
    /*! @brief セーブ用カウンタを取得する (A-3) */
    virtual uint32_t get_count() const
    {
        return this->count;
    }
    /*! @brief セーブ用カウンタを設定する (A-3) */
    virtual void set_count(uint32_t value)
    {
        this->count = value;
    }
    /*! @brief レベル別累積HPテーブルの値を取得する (A-3) */
    virtual int get_hp_table(int level_index) const
    {
        return this->hp_table[level_index];
    }
    /*! @brief レベル別累積HPテーブルの値を設定する (A-3) */
    virtual void set_hp_table(int level_index, int value)
    {
        this->hp_table[level_index] = value;
    }
    virtual int16_t get_num_fire() const
    {
        return this->num_fire;
    }
    virtual void set_num_fire(int16_t value)
    {
        this->num_fire = value;
    }
    virtual int16_t get_to_m_chance() const
    {
        return this->to_m_chance;
    }
    virtual void set_to_m_chance(int16_t value)
    {
        this->to_m_chance = value;
    }
    virtual POSITION get_cur_lite() const
    {
        return this->cur_lite;
    }
    virtual void set_cur_lite(POSITION value)
    {
        this->cur_lite = value;
    }
    virtual bool is_cumber_armor() const
    {
        return this->cumber_armor;
    }
    virtual void set_cumber_armor(bool value)
    {
        this->cumber_armor = value;
    }
    virtual bool is_cumber_glove() const
    {
        return this->cumber_glove;
    }
    virtual void set_cumber_glove(bool value)
    {
        this->cumber_glove = value;
    }
    virtual bool is_heavy_wield(int hand) const
    {
        return this->heavy_wield[hand];
    }
    virtual void set_heavy_wield(int hand, bool value)
    {
        this->heavy_wield[hand] = value;
    }
    virtual bool is_icky_wield(int hand) const
    {
        return this->icky_wield[hand];
    }
    virtual void set_icky_wield(int hand, bool value)
    {
        this->icky_wield[hand] = value;
    }
    virtual bool is_icky_riding_wield(int hand) const
    {
        return this->icky_riding_wield[hand];
    }
    virtual void set_icky_riding_wield(int hand, bool value)
    {
        this->icky_riding_wield[hand] = value;
    }
    virtual bool is_riding_ryoute() const
    {
        return this->riding_ryoute;
    }
    virtual void set_riding_ryoute(bool value)
    {
        this->riding_ryoute = value;
    }
    virtual bool is_monlite() const
    {
        return this->monlite;
    }
    virtual void set_monlite(bool value)
    {
        this->monlite = value;
    }

    /*! @brief 能力値最大値 stat_max[idx] を取得する (提案 31b) */
    virtual short get_stat_max(int idx) const
    {
        return this->stat_max[idx];
    }

    /*! @brief 能力値現在値 stat_cur[idx] を取得する (提案 31b) */
    virtual short get_stat_cur(int idx) const
    {
        return this->stat_cur[idx];
    }

    /*! @brief 能力値最大の最大値 stat_max_max[idx] を取得する (提案 31b) */
    virtual short get_stat_max_max(int idx) const
    {
        return this->stat_max_max[idx];
    }

    /*! @brief 能力値修正済み値 stat_use[idx] を取得する (提案 31b) */
    virtual int16_t get_stat_use(int idx) const
    {
        return this->stat_use[idx];
    }

    /*! @brief 能力値最大修正済み値 stat_top[idx] を取得する (提案 31b) */
    virtual int16_t get_stat_top(int idx) const
    {
        return this->stat_top[idx];
    }

    /*! @brief 能力値修正値 stat_add[idx] を取得する (提案 31b) */
    virtual int16_t get_stat_add(int idx) const
    {
        return this->stat_add[idx];
    }

    /*! @brief 能力値インデックス stat_index[idx] を取得する (提案 31b) */
    virtual int16_t get_stat_index(int idx) const
    {
        return this->stat_index[idx];
    }

    /*! @brief 能力値最大値 stat_max[idx] を設定する (提案 30) */
    virtual void set_stat_max(int idx, short value)
    {
        this->stat_max[idx] = value;
    }

    /*! @brief 能力値現在値 stat_cur[idx] を設定する (提案 30) */
    virtual void set_stat_cur(int idx, short value)
    {
        this->stat_cur[idx] = value;
    }

    /*! @brief 能力値現在値 stat_cur[idx] を加算する (提案 30) */
    virtual void add_stat_cur(int idx, short delta)
    {
        this->stat_cur[idx] += delta;
    }

    /*! @brief 能力値最大の最大値 stat_max_max[idx] を設定する (提案 30) */
    virtual void set_stat_max_max(int idx, short value)
    {
        this->stat_max_max[idx] = value;
    }

    /*! @brief 能力値修正済み値 stat_use[idx] を設定する (提案 30) */
    virtual void set_stat_use(int idx, int16_t value)
    {
        this->stat_use[idx] = value;
    }

    /*! @brief 能力値最大修正済み値 stat_top[idx] を設定する (提案 30) */
    virtual void set_stat_top(int idx, int16_t value)
    {
        this->stat_top[idx] = value;
    }

    /*! @brief 能力値修正値 stat_add[idx] を設定する (提案 30) */
    virtual void set_stat_add(int idx, int16_t value)
    {
        this->stat_add[idx] = value;
    }

    /*! @brief 能力値インデックス stat_index[idx] を設定する (提案 30) */
    virtual void set_stat_index(int idx, int16_t value)
    {
        this->stat_index[idx] = value;
    }

    /*! @brief 影 (KAGE) かどうか */
    virtual bool is_kage() const
    {
        return this->has_constant_flag(MonsterConstantFlagType::KAGE);
    }

    /*! @brief 狂乱状態 (FRENZY) かどうか */
    virtual bool is_frenzied() const
    {
        return this->has_constant_flag(MonsterConstantFlagType::FRENZY);
    }

    /*! @brief カメレオン (CHAMELEON) かどうか */
    virtual bool is_chameleon() const
    {
        return this->has_constant_flag(MonsterConstantFlagType::CHAMELEON);
    }

    /*! @brief クローン個体 (CLONED) かどうか */
    virtual bool is_cloned() const
    {
        return this->has_constant_flag(MonsterConstantFlagType::CLONED);
    }

    /*! @brief ペット化禁止 (NOPET) かどうか */
    virtual bool is_nopet() const
    {
        return this->has_constant_flag(MonsterConstantFlagType::NOPET);
    }

    /*! @brief 巨大サイズ (HUGE) 個体修飾子 */
    virtual bool is_huge() const
    {
        return this->has_constant_flag(MonsterConstantFlagType::HUGE);
    }

    /*! @brief 大型サイズ (LARGE) 個体修飾子 */
    virtual bool is_large() const
    {
        return this->has_constant_flag(MonsterConstantFlagType::LARGE);
    }

    /*! @brief 小型サイズ (SMALL) 個体修飾子 */
    virtual bool is_small() const
    {
        return this->has_constant_flag(MonsterConstantFlagType::SMALL);
    }

    /*! @brief 太め (FAT) 個体修飾子 */
    virtual bool is_fat() const
    {
        return this->has_constant_flag(MonsterConstantFlagType::FAT);
    }

    /*! @brief 痩せ (GAUNT) 個体修飾子 */
    virtual bool is_gaunt() const
    {
        return this->has_constant_flag(MonsterConstantFlagType::GAUNT);
    }

    /*! @brief 軽量 (LIGHTWEIGHT) 個体修飾子 */
    virtual bool is_lightweight() const
    {
        return this->has_constant_flag(MonsterConstantFlagType::LIGHTWEIGHT);
    }

    /*! @brief 全裸 (NAKED) 個体修飾子 */
    virtual bool is_naked() const
    {
        return this->has_constant_flag(MonsterConstantFlagType::NAKED);
    }

    /*! @brief ゾンビ化 (ZOMBIFIED) 個体修飾子 */
    virtual bool is_zombified() const
    {
        return this->has_constant_flag(MonsterConstantFlagType::ZOMBIFIED);
    }

    /*! @brief 不正改造個体 (ILLEGAL_MODIFIED) */
    virtual bool is_illegal_modified() const
    {
        return this->has_constant_flag(MonsterConstantFlagType::ILLEGAL_MODIFIED);
    }

    /*! @brief サンタ化 (SANTA) 個体 */
    virtual bool is_santa() const
    {
        return this->has_constant_flag(MonsterConstantFlagType::SANTA);
    }

    /*! @brief 怒り (ANGER) 状態 */
    virtual bool is_angered() const
    {
        return this->has_constant_flag(MonsterConstantFlagType::ANGER);
    }

    /*! @brief 嫁化 (WAIFUIZED) 個体 */
    virtual bool is_waifuized() const
    {
        return this->has_constant_flag(MonsterConstantFlagType::WAIFUIZED);
    }

    /*! @brief クイルスラグ産まれ (QUYLTHLUG_BORN) */
    virtual bool is_quylthlug_born() const
    {
        return this->has_constant_flag(MonsterConstantFlagType::QUYLTHLUG_BORN);
    }

    /*! @brief 排泄済み (DEFECATED) */
    virtual bool is_defecated() const
    {
        return this->has_constant_flag(MonsterConstantFlagType::DEFECATED);
    }

    /*! @brief 嘔吐済み (VOMITED) */
    virtual bool is_vomited() const
    {
        return this->has_constant_flag(MonsterConstantFlagType::VOMITED);
    }

    /*! @brief 流路追跡を行わない (NOFLOW) */
    virtual bool has_noflow() const
    {
        return this->has_constant_flag(MonsterConstantFlagType::NOFLOW);
    }

    /*! @brief 抹殺対象外 (NOGENO) */
    virtual bool is_nogeno() const
    {
        return this->has_constant_flag(MonsterConstantFlagType::NOGENO);
    }

    /*!
     * @brief プレイヤーに対する学習フラグ（smart_learn）を取得する
     * @return 学習フラグ群。プレイヤー側は空のフラグ集合を返す
     */
    virtual const EnumClassFlagGroup<MonsterSmartLearnType> &get_smart_flags() const
    {
        if (this->has_monster_profile()) {
            return this->get_monster_profile().smart;
        }
        static const EnumClassFlagGroup<MonsterSmartLearnType> empty{};
        return empty;
    }

    /*!
     * @brief 指定インデックスの呪文熟練度を取得する
     * @param spell_idx 呪文インデックス (0..63)
     * @return SUB_EXP 値。プレイヤーは spell_exp[spell_idx]、モンスターは default 実装で
     *         同フィールドを参照 (現状は意味を持たないが将来モンスター呪文熟練を導入する余地)
     */
    virtual SUB_EXP get_spell_exp(int spell_idx) const
    {
        return this->spell_exp[spell_idx];
    }

    /*!
     * @brief 指定スキル種別のスキル熟練度を取得する
     * @param skill スキル種別
     * @return SUB_EXP 値。プレイヤーは skill_exp に存在すれば返し、無ければ 0。
     */
    virtual SUB_EXP get_skill_exp(PlayerSkillKindType skill) const
    {
        const auto it = this->skill_exp.find(skill);
        return (it != this->skill_exp.end()) ? it->second : 0;
    }

    /*!
     * @brief 指定武器種別・サブ種別の武器熟練度を取得する
     * @param tval ItemKindType
     * @param sval サブ種別 (0..63)
     * @return SUB_EXP 値。プレイヤーは weapon_exp[tval][sval]、なければ 0
     */
    virtual SUB_EXP get_weapon_exp(ItemKindType tval, int sval) const
    {
        const auto it = this->weapon_exp.find(tval);
        return (it != this->weapon_exp.end()) ? it->second[sval] : 0;
    }

    /*!
     * @brief 指定武器種別・サブ種別の武器熟練度の上限値を取得する (提案 10/36)
     */
    virtual SUB_EXP get_weapon_exp_max(ItemKindType tval, int sval) const
    {
        const auto it = this->weapon_exp_max.find(tval);
        return (it != this->weapon_exp_max.end()) ? it->second[sval] : 0;
    }

    // [提案 10/36] 熟練度書込みの setter virtual 群。
    // 参照取得 (auto &) パターンは player-skill.cpp / wizard-special-process.cpp
    // 等の一部で残存するため、フィールドの private 化はせず、API として
    // 提供のみ行う。
    virtual void set_spell_exp(int spell_idx, SUB_EXP value)
    {
        this->spell_exp[spell_idx] = value;
    }
    virtual void add_spell_exp(int spell_idx, SUB_EXP delta)
    {
        this->spell_exp[spell_idx] += delta;
    }

    virtual void set_skill_exp(PlayerSkillKindType skill, SUB_EXP value)
    {
        this->skill_exp[skill] = value;
    }
    virtual void add_skill_exp(PlayerSkillKindType skill, SUB_EXP delta)
    {
        this->skill_exp[skill] += delta;
    }

    virtual void set_weapon_exp(ItemKindType tval, int sval, SUB_EXP value)
    {
        this->weapon_exp[tval][sval] = value;
    }
    virtual void add_weapon_exp(ItemKindType tval, int sval, SUB_EXP delta)
    {
        this->weapon_exp[tval][sval] += delta;
    }

    virtual void set_weapon_exp_max(ItemKindType tval, int sval, SUB_EXP value)
    {
        this->weapon_exp_max[tval][sval] = value;
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

    /*!
     * @brief 材質 (副種族) 一覧を取得する
     */
    virtual const std::vector<CreatureMaterialType> &get_materials() const;

    /*!
     * @brief 指定した材質を保持しているか
     */
    virtual bool has_material(CreatureMaterialType material) const;

    /*!
     * @brief 材質を追加する (既に保持していれば何もしない)
     */
    virtual void add_material(CreatureMaterialType material);

    /*!
     * @brief 材質を取り除く
     */
    virtual void remove_material(CreatureMaterialType material);

    /*!
     * @brief 材質を全て取り除く
     */
    virtual void clear_materials();

    /*!
     * @brief 材質一覧を一括設定する (savefile ロード用)
     */
    virtual void set_materials(const std::vector<CreatureMaterialType> &new_materials);

    /*!
     * @brief 全材質の能力値修正の合計を取得する (内部 10 単位)
     * @param stat 能力値インデックス (A_STR 等)
     */
    virtual int get_material_stat_modifier(int stat) const;

    /*!
     * @brief 全材質の AC 修正の合計を取得する
     */
    virtual int get_material_ac_modifier() const;

    /*!
     * @brief 材質に基づく金銭ドロップ額の倍率 (% 単位) を取得する
     * @return 保持する材質のうち最も高い gold_drop_percent。材質を持たなければ 100
     * @details 複数材質を持つ場合は最も貴重な材質の倍率を採用する (合算しない)。
     */
    virtual int get_material_gold_drop_percent() const;

    /*!
     * @brief モンスター種族定義の材質指定および材質系 kind_flags に基づいて材質を初期化する
     * @details モンスター以外では何もしない。
     */
    virtual void initialize_materials();

    /*!
     * @brief 保持する材質の能力値修正を能力値に適用する
     * @details stat_max / stat_cur / stat_use を材質修正分だけ加算し [30, 2000] にクランプする。
     *          モンスター生成時に stat_modifiers 適用後へ続けて呼ぶことを想定。
     */
    virtual void apply_material_stat_modifiers();

    /*!
     * @brief 性格を指定値に設定する
     * @details ppersonality と personality ポインタの双方を更新する。
     * @param value 設定する性格
     */
    void set_personality(player_personality_type value);

    /*!
     * @brief 性格をランダムに設定する (いかさまは除外)
     * @details 性別制限のある性格は psex に合致する場合のみ選ばれる。
     *          ppersonality と personality ポインタの双方を更新する。
     *          モンスター種族に性格が固定指定されている場合は常にそれを使う。
     */
    void assign_random_personality();

    /*!
     * @brief 職業が魔法領域を持つ場合に第一領域を全領域から完全ランダムに設定する
     * @details 領域選択不可の職業 (戦士・忍者等) では何もしない。
     */
    void assign_random_realm();

    /*!
     * @brief モンスター種族に固定指定されたプレイヤー種族・職業を付与する (提案C1)
     * @details JSON `player_race` / `player_class` が指定されたモンスターに限り
     *          prace / pclass を設定する。未指定 (NONE) なら何もしない。
     *          現状 種族・職業の効果 (耐性・特典) は未反映で、フィールド付与のみ。
     */
    void assign_fixed_player_race_and_class();

    /*!
     * @brief モンスター種族に固定指定された突然変異を付与する (提案C5)
     * @details JSON `mutations` で指定された突然変異を生成時にモンスターへ付与する。
     *          未指定 (空) なら何もしない。付与のみで、per-turn 処理の発火は別段。
     */
    void assign_fixed_mutations();

    byte get_temporary_speed() const;

    /*!
     * @brief モンスター固有データ（MonsterProfile）を初期化する
     * @details monster_profile を emplace し、時限効果を 0 で初期化する。
     * モンスター生成時にフロアの m_list 初期化等から呼ぶ。
     */
    void init_monster_profile();

    /*!
     * @brief クリーチャーの状態をデフォルト（空）にリセットする
     * @details モンスター（monster_profile を持つ）の場合は再初期化も行う。
     */
    virtual void wipe();

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

    // 耐性系 virtual メソッド (提案 4)。
    // プレイヤーは装備・職業・種族・時限効果から集計、モンスターは
    // 将来 MonsterProfile / MonraceDefinition 経由で override 可能。
    // 戻り値は BIT_FLAGS_CAUSE_* のビット集合（0 なら非耐性）。
    virtual BIT_FLAGS has_resist_fire();
    virtual BIT_FLAGS has_resist_cold();
    virtual BIT_FLAGS has_resist_elec();
    virtual BIT_FLAGS has_resist_acid();
    virtual BIT_FLAGS has_resist_pois();
    virtual BIT_FLAGS has_resist_conf();
    virtual BIT_FLAGS has_resist_sound();
    virtual BIT_FLAGS has_resist_lite();
    virtual BIT_FLAGS has_resist_dark();
    virtual BIT_FLAGS has_resist_chaos();
    virtual BIT_FLAGS has_resist_disen();
    virtual BIT_FLAGS has_resist_shard();
    virtual BIT_FLAGS has_resist_nexus();
    virtual BIT_FLAGS has_resist_blind();
    virtual BIT_FLAGS has_resist_neth();
    virtual BIT_FLAGS has_resist_time();
    virtual BIT_FLAGS has_resist_water();
    virtual BIT_FLAGS has_resist_fear();
    virtual BIT_FLAGS has_resist_curse();
    virtual BIT_FLAGS has_vuln_curse();
    virtual BIT_FLAGS has_vuln_acid();
    virtual BIT_FLAGS has_vuln_elec();
    virtual BIT_FLAGS has_vuln_fire();
    virtual BIT_FLAGS has_vuln_cold();
    virtual BIT_FLAGS has_vuln_lite();
    virtual BIT_FLAGS has_immune_fire();
    virtual BIT_FLAGS has_immune_cold();
    virtual BIT_FLAGS has_immune_acid();
    virtual BIT_FLAGS has_immune_elec();
    virtual BIT_FLAGS has_immune_dark();
    virtual BIT_FLAGS has_immune_lite();

    // その他の装備集計系 virtual メソッド (提案 4)。
    virtual bool has_pass_wall();
    virtual bool has_kill_wall();
    virtual BIT_FLAGS has_reflect();
    virtual bool has_two_handed_weapons();
    virtual BIT_FLAGS has_sh_fire();
    virtual BIT_FLAGS has_sh_elec();
    virtual BIT_FLAGS has_sh_cold();
    virtual BIT_FLAGS has_down_saving();
    virtual BIT_FLAGS has_no_ac();
    virtual BIT_FLAGS has_easy2_weapon();

    /*!
     * @brief 汎用テレパシーの有無を返す
     * @details プレイヤーは装備由来 (telepathy フィールドを player-status.cpp が更新)。
     *          モンスター側は対応概念なし（モンスターの感知は別経路）のため常に false。
     */
    virtual bool has_telepathy() const
    {
        return this->telepathy != 0;
    }
    // 種別 ESP 群: プレイヤーは装備由来 (esp_* フィールドを player-status.cpp が更新)。
    // モンスター側に対応概念なしのため、フィールド初期値 0 で常に false を返す。
    virtual bool has_esp_animal() const
    {
        return this->esp_animal != 0;
    }
    virtual bool has_esp_nasty() const
    {
        return this->esp_nasty != 0;
    }
    virtual bool has_esp_homo() const
    {
        return this->esp_homo != 0;
    }
    virtual bool has_esp_undead() const
    {
        return this->esp_undead != 0;
    }
    virtual bool has_esp_demon() const
    {
        return this->esp_demon != 0;
    }
    virtual bool has_esp_orc() const
    {
        return this->esp_orc != 0;
    }
    virtual bool has_esp_troll() const
    {
        return this->esp_troll != 0;
    }
    virtual bool has_esp_giant() const
    {
        return this->esp_giant != 0;
    }
    virtual bool has_esp_dragon() const
    {
        return this->esp_dragon != 0;
    }
    virtual bool has_esp_human() const
    {
        return this->esp_human != 0;
    }
    virtual bool has_esp_evil() const
    {
        return this->esp_evil != 0;
    }
    virtual bool has_esp_good() const
    {
        return this->esp_good != 0;
    }
    virtual bool has_esp_nonliving() const
    {
        return this->esp_nonliving != 0;
    }
    virtual bool has_esp_unique() const
    {
        return this->esp_unique != 0;
    }

    /*!
     * @brief 透明視認の可否
     * @details プレイヤーは装備由来 (see_inv フィールド)。モンスター側は
     *          対応フラグなし（透明モンスターの認知は AI 側で別判定）。
     */
    virtual bool can_see_invisible() const
    {
        return this->see_inv != 0;
    }
    /*!
     * @brief 水中遊泳の可否
     * @details プレイヤーは装備由来 (can_swim フィールド)、モンスターは
     *          種族 feature_flags の CAN_SWIM から判定。
     */
    virtual bool has_can_swim() const;
    /*!
     * @brief 浮遊能力の有無
     * @details プレイヤーは装備由来 (levitation フィールド)、モンスターは
     *          種族 feature_flags の CAN_FLY から判定。
     */
    virtual bool has_levitation() const;
    /*!
     * @brief 麻痺耐性の有無
     * @details プレイヤーは装備由来。モンスターは状態耐性が個別フラグ
     *          (NO_SLEEP / NO_STUN) で表現されるためこの統一値での
     *          単純な対応は持たず、現状 false 相当を返す。
     */
    virtual bool has_free_act() const
    {
        return this->free_act != 0;
    }
    /*!
     * @brief 反魔法能力の有無
     * @details プレイヤーは装備由来。モンスター側に対応フラグなし。
     */
    virtual bool has_anti_magic() const
    {
        return this->anti_magic != 0;
    }
    /*!
     * @brief テレポート阻害の有無
     * @details プレイヤーは装備由来、モンスターは MonsterResistanceType::RESIST_TELEPORT。
     */
    virtual bool has_anti_tele() const;
    /*!
     * @brief 自動再生能力の有無（フィールド値）
     * @details プレイヤーは装備由来、モンスターは MonsterMiscType::REGENERATE。
     */
    virtual bool has_regen_flag() const;
    /*!
     * @brief 経験値吸収耐性の有無
     */
    virtual bool has_hold_exp() const
    {
        return this->hold_exp != 0;
    }
    /*!
     * @brief 低速消化能力の有無
     */
    virtual bool has_slow_digest_flag() const
    {
        return this->slow_digest != 0;
    }
    /*!
     * @brief 夜間視能力の有無
     */
    virtual bool has_see_nocto() const
    {
        return this->see_nocto != 0;
    }
    /*!
     * @brief 特殊攻撃ビットの保有判定
     * @param flag 対象の ATTACK_* ビット
     */
    virtual bool has_special_attack(BIT_FLAGS flag) const
    {
        return (this->special_attack & flag) != 0;
    }
    /*!
     * @brief 特殊防御ビットの保有判定
     * @param flag 対象の DEFENSE_* ビット
     */
    virtual bool has_special_defense(BIT_FLAGS flag) const
    {
        return (this->special_defense & flag) != 0;
    }
    /*!
     * @brief 光源能力の有無 (フィールド値)
     * @details プレイヤーは装備由来 (lite フィールド)、モンスターは
     *          MonsterBrightnessType::HAS_LITE_1/2 / SELF_LITE_1/2 から判定。
     */
    virtual bool has_lite_flag() const;
    /*!
     * @brief 警告能力の有無
     */
    virtual bool has_warning_flag() const
    {
        return this->warning != 0;
    }
    /*!
     * @brief 衝撃付与攻撃の有無 (装備起因)
     */
    virtual bool has_impact_flag() const
    {
        return this->impact != 0;
    }
    /*!
     * @brief 地震付与攻撃の有無 (装備起因)
     */
    virtual bool has_earthquake_flag() const
    {
        return this->earthquake != 0;
    }
    /*!
     * @brief 魔法消費軽減能力の有無
     */
    virtual bool has_dec_mana() const
    {
        return this->dec_mana != 0;
    }
    /*!
     * @brief 易しい呪文能力の有無
     */
    virtual bool has_easy_spell() const
    {
        return this->easy_spell != 0;
    }
    /*!
     * @brief 難しい呪文フラグの有無
     */
    virtual bool has_hard_spell() const
    {
        return this->hard_spell != 0;
    }
    /*!
     * @brief 強力投擲能力の有無
     */
    virtual bool has_mighty_throw() const
    {
        return this->mighty_throw != 0;
    }
    /*!
     * @brief 追加威力弓能力の有無
     */
    virtual bool has_xtra_might() const
    {
        return this->xtra_might != 0;
    }
    /*!
     * @brief 装備由来の祝福武器有無
     */
    virtual bool has_bless_blade() const
    {
        return this->bless_blade != 0;
    }

    // [提案 33] BIT_FLAGS フィールドの setter virtual 群。
    // すべて player-status.cpp の update_creature() から
    // 装備状態を再計算した結果を書き込むのみ。
    virtual void set_telepathy(BIT_FLAGS value)
    {
        this->telepathy = value;
    }
    virtual void set_esp_animal(BIT_FLAGS value)
    {
        this->esp_animal = value;
    }
    virtual void set_esp_nasty(BIT_FLAGS value)
    {
        this->esp_nasty = value;
    }
    virtual void set_esp_homo(BIT_FLAGS value)
    {
        this->esp_homo = value;
    }
    virtual void set_esp_undead(BIT_FLAGS value)
    {
        this->esp_undead = value;
    }
    virtual void set_esp_demon(BIT_FLAGS value)
    {
        this->esp_demon = value;
    }
    virtual void set_esp_orc(BIT_FLAGS value)
    {
        this->esp_orc = value;
    }
    virtual void set_esp_troll(BIT_FLAGS value)
    {
        this->esp_troll = value;
    }
    virtual void set_esp_giant(BIT_FLAGS value)
    {
        this->esp_giant = value;
    }
    virtual void set_esp_dragon(BIT_FLAGS value)
    {
        this->esp_dragon = value;
    }
    virtual void set_esp_human(BIT_FLAGS value)
    {
        this->esp_human = value;
    }
    virtual void set_esp_evil(BIT_FLAGS value)
    {
        this->esp_evil = value;
    }
    virtual void set_esp_good(BIT_FLAGS value)
    {
        this->esp_good = value;
    }
    virtual void set_esp_nonliving(BIT_FLAGS value)
    {
        this->esp_nonliving = value;
    }
    virtual void set_esp_unique(BIT_FLAGS value)
    {
        this->esp_unique = value;
    }

    virtual void set_can_swim(bool value)
    {
        this->can_swim = value;
    }
    virtual void set_levitation(BIT_FLAGS value)
    {
        this->levitation = value;
    }
    virtual void set_free_act(BIT_FLAGS value)
    {
        this->free_act = value;
    }
    virtual void set_see_inv(BIT_FLAGS value)
    {
        this->see_inv = value;
    }
    virtual void set_regenerate(BIT_FLAGS value)
    {
        this->regenerate = value;
    }
    virtual void set_hold_exp(BIT_FLAGS value)
    {
        this->hold_exp = value;
    }
    virtual void set_slow_digest(BIT_FLAGS value)
    {
        this->slow_digest = value;
    }
    virtual void set_lite_flags(BIT_FLAGS value)
    {
        this->lite = value;
    }
    virtual void set_warning_flags(BIT_FLAGS value)
    {
        this->warning = value;
    }
    virtual void set_impact_flags(BIT_FLAGS value)
    {
        this->impact = value;
    }
    virtual void set_earthquake_flags(BIT_FLAGS value)
    {
        this->earthquake = value;
    }
    virtual void set_dec_mana(BIT_FLAGS value)
    {
        this->dec_mana = value;
    }
    virtual void set_easy_spell(BIT_FLAGS value)
    {
        this->easy_spell = value;
    }
    virtual void set_hard_spell(BIT_FLAGS value)
    {
        this->hard_spell = value;
    }
    virtual void set_mighty_throw(BIT_FLAGS value)
    {
        this->mighty_throw = value;
    }
    virtual void set_see_nocto(BIT_FLAGS value)
    {
        this->see_nocto = value;
    }
    virtual void set_anti_magic(BIT_FLAGS value)
    {
        this->anti_magic = value;
    }
    virtual void set_anti_tele(BIT_FLAGS value)
    {
        this->anti_tele = value;
    }
    virtual void set_bless_blade(BIT_FLAGS value)
    {
        this->bless_blade = value;
    }
    virtual void set_xtra_might(BIT_FLAGS value)
    {
        this->xtra_might = value;
    }

    // impact / earthquake は装備別ハンドフラグを保持しているため
    // BIT_FLAGS 値そのものを返す getter を提供 (any_bits 等に渡す用)。
    virtual BIT_FLAGS get_impact_flags() const
    {
        return this->impact;
    }
    virtual BIT_FLAGS get_earthquake_flags() const
    {
        return this->earthquake;
    }

    // [提案 46] 知覚系フラグ (テレパシー / ESP 12 種 / 透明視認 / 強投擲) の
    // 差分検出スナップショット。update_bonuses() の再計算前後で
    // capture_perception_flags() を取得・比較することで、個別の BIT_FLAGS
    // getter を外部公開せず、差分検出を呼出側の関心事に閉じる。
    struct PerceptionFlagsSnapshot {
        BIT_FLAGS telepathy{};
        BIT_FLAGS esp_animal{};
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
        BIT_FLAGS see_inv{};
        BIT_FLAGS mighty_throw{};

        //!< 強投擲フラグが変化したか (変化時はインベントリ再描画が必要)。
        bool mighty_throw_differs_from(const PerceptionFlagsSnapshot &other) const
        {
            return this->mighty_throw != other.mighty_throw;
        }

        //!< テレパシー / ESP / 透明視認のいずれかが変化したか
        //!< (変化時はモンスター状態の再計算が必要)。
        bool monster_perception_differs_from(const PerceptionFlagsSnapshot &other) const
        {
            return (this->telepathy != other.telepathy) || (this->esp_animal != other.esp_animal) || (this->esp_undead != other.esp_undead) || (this->esp_demon != other.esp_demon) || (this->esp_orc != other.esp_orc) || (this->esp_troll != other.esp_troll) || (this->esp_giant != other.esp_giant) || (this->esp_dragon != other.esp_dragon) || (this->esp_human != other.esp_human) || (this->esp_evil != other.esp_evil) || (this->esp_good != other.esp_good) || (this->esp_nonliving != other.esp_nonliving) || (this->esp_unique != other.esp_unique) || (this->see_inv != other.see_inv);
        }
    };

    PerceptionFlagsSnapshot capture_perception_flags() const
    {
        return {
            this->telepathy,
            this->esp_animal,
            this->esp_undead,
            this->esp_demon,
            this->esp_orc,
            this->esp_troll,
            this->esp_giant,
            this->esp_dragon,
            this->esp_human,
            this->esp_evil,
            this->esp_good,
            this->esp_nonliving,
            this->esp_unique,
            this->see_inv,
            this->mighty_throw,
        };
    }

    // 特殊攻撃 / 特殊防御フラグ。compound assignment が複数箇所にあるため
    // add / remove / set / get の 4 種を提供。has_special_attack(flag) は既存。
    virtual void set_special_attack_flags(BIT_FLAGS value)
    {
        this->special_attack = value;
    }
    virtual BIT_FLAGS get_special_attack_flags() const
    {
        return this->special_attack;
    }
    virtual void add_special_attack(BIT_FLAGS flag)
    {
        this->special_attack |= flag;
    }
    virtual void remove_special_attack(BIT_FLAGS flag)
    {
        this->special_attack &= ~flag;
    }

    virtual void set_special_defense_flags(BIT_FLAGS value)
    {
        this->special_defense = value;
    }
    virtual BIT_FLAGS get_special_defense_flags() const
    {
        return this->special_defense;
    }
    virtual void add_special_defense(BIT_FLAGS flag)
    {
        this->special_defense |= flag;
    }
    virtual void remove_special_defense(BIT_FLAGS flag)
    {
        this->special_defense &= ~flag;
    }

    /*!
     * @brief 突然変異フラグ集合への参照
     * @details プレイヤーは装備・レベル・クラス起因の変異、モンスターは
     *          将来的に種族由来の変異を返す想定。
     */
    virtual const EnumClassFlagGroup<PlayerMutationType> &get_mutations() const
    {
        return this->muta;
    }
    /*!
     * @brief 後天特性フラグ集合への参照
     */
    virtual const EnumClassFlagGroup<PlayerMutationType> &get_traits() const
    {
        return this->trait;
    }
    /*!
     * @brief 呪いフラグ集合への参照（装備由来）
     */
    virtual const EnumClassFlagGroup<CurseTraitType> &get_cursed_flags() const
    {
        return this->cursed;
    }
    /*!
     * @brief 特殊呪いフラグ集合への参照
     */
    virtual const EnumClassFlagGroup<CurseSpecialTraitType> &get_cursed_special_flags() const
    {
        return this->cursed_special;
    }

    // [提案 44] 突然変異/特性/呪いフラグの write-side virtual API。
    // 直接 `creature.muta.set(X)` / `creature.cursed.clear()` 等のフィールド
    // アクセスを置き換える。一括代入 (`set_X()`) は savefile load 用。
    virtual bool has_mutation(PlayerMutationType m) const
    {
        return this->muta.has(m);
    }
    virtual void add_mutation(PlayerMutationType m)
    {
        this->muta.set(m);
    }
    virtual void remove_mutation(PlayerMutationType m)
    {
        this->muta.reset(m);
    }
    virtual void clear_mutations()
    {
        this->muta.clear();
    }
    virtual void set_mutations(const EnumClassFlagGroup<PlayerMutationType> &flags)
    {
        this->muta = flags;
    }
    virtual bool has_trait(PlayerMutationType t) const
    {
        return this->trait.has(t);
    }
    virtual void add_trait(PlayerMutationType t)
    {
        this->trait.set(t);
    }
    virtual void remove_trait(PlayerMutationType t)
    {
        this->trait.reset(t);
    }
    virtual void clear_traits()
    {
        this->trait.clear();
    }
    virtual void set_traits(const EnumClassFlagGroup<PlayerMutationType> &flags)
    {
        this->trait = flags;
    }
    virtual bool has_curse(CurseTraitType c) const
    {
        return this->cursed.has(c);
    }
    virtual void add_curse(CurseTraitType c)
    {
        this->cursed.set(c);
    }
    virtual void add_curses(const EnumClassFlagGroup<CurseTraitType> &flags)
    {
        this->cursed.set(flags);
    }
    virtual void remove_curse(CurseTraitType c)
    {
        this->cursed.reset(c);
    }
    virtual void clear_curses()
    {
        this->cursed.clear();
    }
    virtual void set_curses(const EnumClassFlagGroup<CurseTraitType> &flags)
    {
        this->cursed = flags;
    }
    virtual bool has_curse_special(CurseSpecialTraitType c) const
    {
        return this->cursed_special.has(c);
    }
    virtual void add_curse_special(CurseSpecialTraitType c)
    {
        this->cursed_special.set(c);
    }
    virtual void remove_curse_special(CurseSpecialTraitType c)
    {
        this->cursed_special.reset(c);
    }
    virtual void clear_curses_special()
    {
        this->cursed_special.clear();
    }
    virtual void set_curses_special(const EnumClassFlagGroup<CurseSpecialTraitType> &flags)
    {
        this->cursed_special = flags;
    }

    /*!
     * @brief 種族情報ポインタ
     * @details プレイヤー時は race_info[] の該当エントリへのポインタ、
     *          モンスター時は nullptr。将来モンスター側でも種族情報を
     *          返す形に override できる。
     */
    virtual const player_race_info *get_race_info() const;
    /*!
     * @brief 性格情報ポインタ
     */
    virtual const player_personality *get_personality_info() const;

    /*!
     * @brief 職業情報ポインタ
     */
    virtual const player_class_info *get_class_info() const;
    /*!
     * @brief 赤外線視能力の強さ
     */
    virtual ACTION_SKILL_POWER get_infravision() const
    {
        return this->see_infra;
    }
    virtual void set_infravision(ACTION_SKILL_POWER value)
    {
        this->see_infra = value;
    }
    /*!
     * @brief 解除能力スキル
     */
    virtual ACTION_SKILL_POWER get_skill_disarm() const
    {
        return this->skill_dis;
    }
    virtual void set_skill_disarm(ACTION_SKILL_POWER value)
    {
        this->skill_dis = value;
    }
    /*!
     * @brief 魔道具使用スキル
     */
    virtual ACTION_SKILL_POWER get_skill_device() const
    {
        return this->skill_dev;
    }
    virtual void set_skill_device(ACTION_SKILL_POWER value)
    {
        this->skill_dev = value;
    }
    /*!
     * @brief 魔法防御スキル
     */
    virtual ACTION_SKILL_POWER get_skill_save() const
    {
        return this->skill_sav;
    }
    virtual void set_skill_save(ACTION_SKILL_POWER value)
    {
        this->skill_sav = value;
    }

    /*!
     * @brief 攻撃威力に対する魔法防御セービングスロー判定 (提案D1)
     * @param power セーヴィングスローの難易度 (通常は攻撃側のレベル / rlev)
     * @return セーヴィングスローに成功したら true (効果を防いだ)
     * @details `randint0(100 + power/2) < get_skill_save()` の共通イディオムを集約。
     *          get_skill_save() は CreatureEntity の virtual のため、プレイヤー・
     *          モンスターのどちらが対象でも同一経路で判定できる。定義は
     *          creature-entity.cpp (randit0 を本ヘッダに持ち込まないため)。
     */
    bool does_save_against(int power) const;

    /*!
     * @brief 隠密スキル
     */
    virtual ACTION_SKILL_POWER get_skill_stealth() const
    {
        return this->skill_stl;
    }
    virtual void set_skill_stealth(ACTION_SKILL_POWER value)
    {
        this->skill_stl = value;
    }
    /*!
     * @brief 知覚スキル
     */
    virtual ACTION_SKILL_POWER get_skill_search() const
    {
        return this->skill_srh;
    }
    virtual void set_skill_search(ACTION_SKILL_POWER value)
    {
        this->skill_srh = value;
    }
    /*!
     * @brief 探索スキル
     */
    virtual ACTION_SKILL_POWER get_skill_perception() const
    {
        return this->skill_fos;
    }
    virtual void set_skill_perception(ACTION_SKILL_POWER value)
    {
        this->skill_fos = value;
    }
    /*!
     * @brief 打撃命中スキル
     */
    virtual ACTION_SKILL_POWER get_skill_to_hit_melee() const
    {
        return this->skill_thn;
    }
    virtual void set_skill_to_hit_melee(ACTION_SKILL_POWER value)
    {
        this->skill_thn = value;
    }
    /*!
     * @brief 射撃命中スキル
     */
    virtual ACTION_SKILL_POWER get_skill_to_hit_bow() const
    {
        return this->skill_thb;
    }
    virtual void set_skill_to_hit_bow(ACTION_SKILL_POWER value)
    {
        this->skill_thb = value;
    }
    /*!
     * @brief 投射命中スキル
     */
    virtual ACTION_SKILL_POWER get_skill_to_hit_throw() const
    {
        return this->skill_tht;
    }
    virtual void set_skill_to_hit_throw(ACTION_SKILL_POWER value)
    {
        this->skill_tht = value;
    }
    /*!
     * @brief 掘削スキル
     */
    virtual ACTION_SKILL_POWER get_skill_dig() const
    {
        return this->skill_dig;
    }
    virtual void set_skill_dig(ACTION_SKILL_POWER value)
    {
        this->skill_dig = value;
    }

    /*!
     * @brief クリーチャーのコピーを返す
     * @return コピーされたクリーチャー
     */
    CreatureEntity clone() const
    {
        return *this;
    }

    /*!
     * @brief 現在の変身形態を取得する
     * @return 変身形態（NONE なら通常状態）
     */
    virtual MimicKindType get_mimic_form() const
    {
        return this->mimic_form;
    }

    /*!
     * @brief 変身形態を設定する
     * @param form 変身形態
     */
    virtual void set_mimic_form(MimicKindType form)
    {
        this->mimic_form = form;
    }

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
    // [提案 29] r_idx / ap_r_idx は private 化済。アクセスは get_r_idx() /
    //          get_ap_r_idx() / set_r_idx() / set_ap_r_idx() / polymorph_to() を介して行う。
private:
    MonraceId r_idx{}; /*!< モンスターの実種族ID (これが0の時は死亡扱いになる) / Monster race index 0 = dead. */
    MonraceId ap_r_idx{}; /*!< モンスターの外見種族ID（あやしい影、たぬき、ジュラル星人誤認などにより変化する）Monster race appearance index */

public:
    // [提案 47] dealt_damage は private 化済。get_dealt_damage() / set_dealt_damage() / add_dealt_damage() 経由。
private:
    // 与ダメージ蓄積（プレイヤー・モンスター共通）
    int32_t dealt_damage{}; /*!< これまでに蓄積して与えてきたダメージ / Sum of damages dealt by player or to monster */

public:
    // 種族・職業・性格情報（参照風アクセス用）
    const player_race_info *race{}; /*!< 現在の種族情報 / Current race info */
    const player_personality *personality{}; /*!< 現在の性格情報 / Current personality info (accessed like reference) */
    const player_class_info *pclass_ref{}; /*!< 現在の職業情報 / Current class info (accessed like reference) */

    // [提案 37] 行動技能値 / Action skills
    //          private 化済。アクセスは get_X() / set_X() virtual 経由。
    //          値は player-status.cpp の update_creature() で装備・能力値等から再計算。
private:
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

    // [提案 47] run_py / run_px は private 化済。get_run_py() / set_run_py() / get_run_px() / set_run_px() 経由。
private:
    POSITION run_py{}; /*!< 走行中の目標Y座標 / Target Y position while running */
    POSITION run_px{}; /*!< 走行中の目標X座標 / Target X position while running */

public:
    Pos2D target{}; /*!< 攻撃目標座標 (0,0 は未設定) / Attack target position ({0,0} means none) */

    tl::optional<MonsterProfile> monster_profile{}; /*!< モンスター固有データ (モンスターの場合のみ有効) */

private:
    //! 材質 (副種族)。複数同時に保持でき、能力値修正・AC 修正が累積する。
    //! アクセスは get_materials() / add_material() 等の virtual API 経由。
    std::vector<CreatureMaterialType> materials{};

public:
    // [提案 32] private 化済。get_ambush_flag() / set_ambush_flag() 経由。
private:
    bool ambush_flag{}; /*!< 待ち伏せフラグ / Ambush flag */

public:
    // 基本情報
    // [提案 32b] private 化済。get_age()/set_age()/add_age() 等経由。
private:
    int16_t age{}; /*!< 年齢 / Age */
    int16_t ht{}; /*!< 身長 / Height */
    int16_t wt{}; /*!< 体重 / Weight */

public:
    // [提案 32] private 化済。get_prestige() / set_prestige() / add_prestige() / divide_prestige() 経由。
private:
    int16_t prestige{}; /*!< 名声 / Prestige */

public:
    int32_t death_count{}; /*!< 死亡カウント / Death count */

    // ステータス関連
    // [提案 32b] private 化済。get_stat_*(idx) / set_stat_*(idx, val) / add_stat_cur(idx, delta) 経由。
private:
    short stat_max[A_MAX]{}; /*!< 現在の最大能力値 / Current "maximal" stat values */
    short stat_max_max[A_MAX]{}; /*!< 最大の最大能力値 / Maximal "maximal" stat values */
    short stat_cur[A_MAX]{}; /*!< 現在の基本能力値 / Current "natural" stat values */
    int16_t stat_use[A_MAX]{}; /*!< 現在の修正済み能力値 / Current modified stats */
    int16_t stat_top[A_MAX]{}; /*!< 最大の修正済み能力値 / Maximal modified stats */
    int16_t stat_add[A_MAX]{}; /* Modifiers to stat values */
    int16_t stat_index[A_MAX]{}; /* Indexes into stat tables */

public:
    // 徳関連
    std::map<Virtue, int16_t> virtues; /*!< 徳の値 / Virtue values */

    // 経験値関連
    // [提案 32] max_max_exp を private 化済。get_max_max_exp() / set_max_max_exp() 経由。
private:
    EXP max_max_exp{}; /*!< 最大の最大経験値 / Max max experience (only to calculate score) */

public:
    // [提案 32b] private 化済。get_exp()/set_exp()/add_exp()/sub_exp()/get_max_exp()/set_max_exp() 等経由。
private:
    EXP max_exp{}; /*!< 最大経験値 / Max experience */
    EXP exp{}; /*!< 現在の経験値 / Current experience */

public:
    uint32_t exp_frac{}; /*!< 経験値の小数部 / Current exp frac (times 2^16) */

    // 騎乗関連
    // [提案 29] riding は private 化済。アクセスは get_riding() / set_riding() /
    //          ride_monster() を介して行う。
private:
    MONSTER_IDX riding{}; /*!< 騎乗中のモンスターID / Riding on a monster of this index */

public:
    // インベントリ関連
    std::vector<std::shared_ptr<ItemEntity>> inventory{}; /*!< 所持品リスト / The creature's inventory */
    // 所持品数 inven_cnt / 装備品数 equip_cnt は提案 25 で inventory[] から自動計算する
    // get_inven_cnt() / get_equip_cnt() に置換済み。

    // [Phase 2] 拡張装備スロット (尾の指輪、第二の首など、種族固有部位)。
    // body_structure の get_extended_slots() で定義されるスロットに対応し、
    // インデックス順に格納される。プレイヤーは HUMANOID で空。
    // モンスター生成時に init_extended_inventory() で初期化。
    std::vector<std::shared_ptr<ItemEntity>> extended_inventory{};

    // 座標関連
    POSITION oldpy{}; /*!< 前回のY座標 / Previous location (Y) */
    POSITION oldpx{}; /*!< 前回のX座標 / Previous location (X) */
    POSITION y{}; /*!< 現在のY座標 / Current location (Y) */
    POSITION x{}; /*!< 現在のX座標 / Current location (X) */

    // MP関連
    // [提案 32b] private 化済。get_max_mp()/set_max_mp()/get_current_mp()/set_current_mp()/add_current_mp()/sub_current_mp() 等経由。
private:
    MANA_POINT max_mp{}; /*!< 最大MP / Max mana pts */
    MANA_POINT current_mp{}; /*!< 現在MP / Current mana pts */

public:
    uint32_t current_mp_frac{}; /*!< MP小数部 / Current mana frac (times 2^16) */

    // HP関連
    int hp{}; /*!< 現在のHP / Current Hit points */
    int maxhp{}; /*!< 現在の最大HP (一時減少を反映した実効値) / Effective max Hit points */
    int max_maxhp{}; /*!< 本来の最大HP (一時減少前のキャップ。プレイヤー・モンスター共通) / Undiminished max Hit points */
    uint32_t hp_frac{}; /*!< HP小数部 / Current hit frac (times 2^16) */

    // 基本パラメータ（主にプレイヤー用、モンスターでは未使用）
    player_sex psex{}; /*!< 性別 / Sex index */
    PlayerRaceType prace{}; /*!< 種族 / Race index */
    PlayerClassType pclass{}; /*!< クラス / Class index */
    player_personality_type ppersonality{}; /*!< 性格 / Personality index */
    // [提案 32b] private 化済。get_town_num() / set_town_num() 経由。
private:
    int16_t town_num{}; /*!< 現在いる街番号 / Current town number */

public:
    // クラス固有データ / Class-specific data
    ClassSpecificData class_specific_data;

    // 変異関連
    // [提案 44] private 化済。has_mutation/add_mutation/remove_mutation/clear_mutations/set_mutations 経由。
    // trait についても has_trait/add_trait/remove_trait/clear_traits/set_traits 経由。
private:
    EnumClassFlagGroup<PlayerMutationType> muta{}; /*!< 突然変異 / mutations */
    EnumClassFlagGroup<PlayerMutationType> trait{}; /*!< 後天特性 / permanent trait */

    // パトロン関連（主にカオス戦士用、モンスターでは未使用）
    // [提案 44] private 化済。get_patron()/set_patron() 経由。
    int16_t patron{}; /*!< カオスパトロンのID / Chaos patron ID */

    // エネルギー関連
    // [A-2] private 化済。get_energy_need() / set_energy_need() /
    // add_energy_need() / sub_energy_need() 経由。
private:
    ACTION_ENERGY energy_need{}; /*!< 次の行動までに必要なエネルギー / Energy needed for next move */

public:
    // 速度関連
    int speed{}; /*!< クリーチャーの速度 / Creature speed */

    // レベル関連
    // [提案 32b] private 化済。get_level() / set_level() 経由。
private:
    int16_t level{}; /*!< クリーチャーのレベル / Creature level */

public:
    // アライメント関連
    int alignment{}; /*!< 善悪の属性 / Good/evil/neutral */

    // 行動状態関連
    // [提案 43] action / running は private 化済。
    // get_action() / set_action() / get_running() / set_running() 経由。
private:
    byte action{}; /*!< クリーチャーが現在取っている常時行動のID / Currently action */
    int16_t running{}; /*!< 現在の走行カウンタ / Current counter for running, if any */

public:
    // 所持金関連
    // [提案 32b] private 化済。get_au() / set_au() / add_au() / sub_au() / divide_au() 経由。
private:
    PRICE au{}; /*!< 所持金 / Current Gold */

public:
    // AC関連
    ARMOUR_CLASS ac{}; /*!< アーマークラス（プレイヤーは装備無しの基本AC、モンスターは総合AC） / Armor class (base AC for player, total AC for monster) */
    // [提案 32b] private 化済。get_to_a() / set_to_a() 経由。
private:
    ARMOUR_CLASS to_a{}; /*!< ACへのボーナス（主にプレイヤー用、装備などによるボーナス） / Bonus to AC (mainly for player, bonus from equipment) */

public:
    // 名前関連
    std::string name{}; /*!< クリーチャーの名前（プレイヤー名またはペット名） / Creature's name (player name or pet nickname) */

    // [提案 33] テレパシー・感知能力フラグ / Telepathy and ESP abilities
    // すべて player-status.cpp の update_creature() で装備状態から再計算。
    // 外部アクセスは has_X() / set_X() / get_X_flags() (旧値スナップショット用) 経由。
private:
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

public:
    // [提案 33] 地形移動能力 / Terrain movement abilities
private:
    bool can_swim{}; /* No damage in water */

public:
    // [提案 33] 特殊防御フラグ / Bit flags for the "special_defense" variable. -LM-
    // has_special_defense(flag) / add_special_defense(flag) 等経由。
private:
    BIT_FLAGS special_defense{};

public:
    // 装備・能力関連フラグ / Equipment and ability flags
private:
    bool hack_mutation{};

public:
    // [提案 51] hack_mutation を private 化。is_hack_mutation() / set_hack_mutation() 経由。
    bool is_hack_mutation() const
    {
        return this->hack_mutation;
    }
    void set_hack_mutation(bool value)
    {
        this->hack_mutation = value;
    }

    // [提案 43] is_fired (フィールド名 fired にリネーム) / level_up_message は
    // private 化済。is_fired() / set_is_fired() / has_level_up_message() /
    // set_level_up_message() 経由。
private:
    bool fired{};
    bool level_up_message{};

public:
    // [提案 33] 装備由来 BIT_FLAGS 群。
    // 全て has_X() / set_X() 経由でアクセス。
    // [提案 44] cursed / cursed_special も private 化済。
    // has_curse / add_curse / add_curses / remove_curse / clear_curses / set_curses 経由。
    // cursed_special 側は has_curse_special / add_curse_special / remove_curse_special /
    // clear_curses_special / set_curses_special 経由。
private:
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

public:
private:
    bool invoking_midnight_curse{};

public:
    // [提案 51] invoking_midnight_curse を private 化。is_invoking_midnight_curse() / set_invoking_midnight_curse() 経由。
    bool is_invoking_midnight_curse() const
    {
        return this->invoking_midnight_curse;
    }
    void set_invoking_midnight_curse(bool value)
    {
        this->invoking_midnight_curse = value;
    }

    // インシデント記録（ツリー構造）
    std::map<std::string, int32_t> incident_tree{}; /*!< ツリー構造ID（例: "root/attack/critical"）で記録するインシデントカウント */

    // 死亡情報
    std::string died_from{}; /*!< 何によって殺されたか / What killed the creature */
    MonraceId killer_monrace_id{}; /*!< 死因となったモンスターのID / MonraceId of the killer */

    // 死亡履歴
    std::vector<DeathRecord> death_history{}; /*!< 死亡履歴リスト */

    /*!
     * @brief ツリー構造インシデント数加算
     * @param incident_id 階層構造のインシデントID（例: "root/attack/critical"）
     * @param num 加算量
     */
    void plus_incident_tree(const std::string &incident_id, int num);

    // [提案 34] 表示用既知値 dis_to_h / dis_to_d / dis_to_h_b / dis_to_a / dis_ac
    // を private 化。アクセスは get_dis_X() / set_dis_X() 経由。
private:
    HIT_PROB dis_to_h[2]{}; /*!< 判明している現在の表記上の近接武器命中修正値 /  Known bonus to hit (wield) */
    HIT_PROB dis_to_h_b{}; /*!< 判明している現在の表記上の射撃武器命中修正値 / Known bonus to hit (bow) */
    int dis_to_d[2]{}; /*!< 判明している現在の表記上の近接武器ダメージ修正値 / Known bonus to dam (wield) */
    ARMOUR_CLASS dis_to_a{}; /*!< 判明している現在の表記上の装備AC修正値 / Known bonus to ac */
    ARMOUR_CLASS dis_ac{}; /*!< 判明している現在の表記上の装備AC基礎値 / Known base ac */

public:
    // [提案 32b] to_h[] / to_d[] / to_h_b / to_h_m / to_d_m を private 化済。get_to_h(hand) / set_to_h(hand, val) 等経由。
private:
    int16_t to_h[2]{}; /* Bonus to hit (wield) */
    int16_t to_h_b{}; /* Bonus to hit (bow) */
    int16_t to_h_m{}; /* Bonus to hit (misc) */
    int16_t to_d[2]{}; /* Bonus to dam (wield) */
    int16_t to_d_m{}; /* Bonus to dam (misc) */

    // [提案 39] private 化済。get_to_m_chance() / set_to_m_chance() / get_num_blow(hand)
    // / set_num_blow(hand, value) / get_num_fire() / set_num_fire() 経由。
private:
    int16_t to_m_chance{}; /* Minusses to cast chance */

    int16_t num_blow[2]{}; /* Number of blows */
    int16_t num_fire{}; /* Number of shots */

public:
    // [提案 32b] private 化済。get_food() / set_food() 経由。
private:
    int16_t food{}; /*!< ゲーム中の滋養度の型定義 / Current nutrition */

    // [提案 39] cur_lite は private 化済。get_cur_lite() / set_cur_lite() 経由。
    // [提案 42] old_lite は private 化済。get_old_lite() / set_old_lite() 経由。
private:
    POSITION cur_lite{}; /* Radius of lite (if any) */
    POSITION old_lite{}; /* Radius of lite (if any) */

public:
    // [提案 33] private 化済。has_special_attack(flag) / add_special_attack(flag)
    // / remove_special_attack(flag) / set_special_attack_flags(BIT_FLAGS) /
    // get_special_attack_flags() 経由。
private:
    BIT_FLAGS special_attack{};

public:
    SUB_EXP spell_exp[64]{}; /* Proficiency of spells */
    std::map<ItemKindType, std::array<SUB_EXP, 64>> weapon_exp{}; /* Proficiency of weapons */
    std::map<ItemKindType, std::array<SUB_EXP, 64>> weapon_exp_max{}; /* Maximum proficiency of weapons */
    std::map<PlayerSkillKindType, SUB_EXP> skill_exp{}; /* Proficiency of misc. skill */
    MartialArtsStyleType martial_arts_style{ MartialArtsStyleType::TRADITIONAL }; /* Martial arts fighting style */

    RealmType realm1{}; /* First magic realm */
    RealmType realm2{}; /* Second magic realm */

    // [A-2] private 化済。get_element_realm() / set_element_realm() 経由。
private:
    ElementRealmType element_realm{}; //!< 元素使い領域

public:
    Dice hit_dice{}; /* Hit dice */
    uint16_t expfact{}; /* Experience factor
                         * Note: was byte, causing overflow for Amberite
                         * characters (such as Amberite Paladins)
                         */

    std::map<INCIDENT, int32_t> incident{}; /*!< これまでに行った出来事カウント（従来型、enumベース） */

    // [A-1] private 化済。get_mutant_regenerate_mod() / set_mutant_regenerate_mod() 経由。
private:
    PERCENTAGE mutant_regenerate_mod{};

    // [提案 32] private 化済。get_max_plv() / set_max_plv() 経由。
    int16_t max_plv{}; /* Max Player Level */

    // [A-1] private 化済。get_learned_spells() / set_learned_spells() /
    // get_add_spells() / set_add_spells() 経由。
    int16_t learned_spells{};
    int16_t add_spells{};

    // [A-3] private 化済。get_count() / set_count() 経由。
private:
    uint32_t count{};

public:
    // [提案 43] timewalk / resting は private 化済。
    // is_timewalking() / set_timewalking() / get_resting() / set_resting() 経由。
private:
    bool timewalk{};

public:
#define COMMAND_ARG_REST_UNTIL_DONE -2 /*!<休憩コマンド引数 … 必要な分だけ回復 */
#define COMMAND_ARG_REST_FULL_HEALING -1 /*!<休憩コマンド引数 … HPとMPが全回復するまで */
private:
    GAME_TURN resting{}; /* Current counter for resting, if any */

private:
    // [提案 48] private 化済。get_recall_dungeon() / set_recall_dungeon() 経由。
    DungeonId recall_dungeon{}; /* Dungeon set to be recalled */

    // [提案 48] private 化済。get_enchant_energy_need() / set_enchant_energy_need() /
    // add_enchant_energy_need() / sub_enchant_energy_need() 経由。
    ENERGY enchant_energy_need{}; /* Energy needed for next upkeep effect	 */

public:
    /*
     * creature.special_attackによるプレイヤーの攻撃状態の定義 / Bit flags for the "creature.special_attack" variable. -LM-
     *
     * Note:  The elemental and poison attacks should be managed using the
     * function "set_ele_attack", in spell2.c.  This provides for timeouts and
     * prevents the player from getting more than one at a time.
     */

    // [提案 41] 呪文マスクを private 化。
    // realm_idx は 0 (第 1 領域) / 1 (第 2 領域)、spell_id は 0..31。
    // savefile load/save 時は get_spell_*_flags() / set_spell_*_flags() を使用。
    // 通常アクセスは has_*_spell() / set_*_spell() を使用。
private:
    BIT_FLAGS spell_learned1{}; /* bit mask of spells learned */
    BIT_FLAGS spell_learned2{}; /* bit mask of spells learned */
    BIT_FLAGS spell_worked1{}; /* bit mask of spells tried and worked */
    BIT_FLAGS spell_worked2{}; /* bit mask of spells tried and worked */
    BIT_FLAGS spell_forgotten1{}; /* bit mask of spells learned but forgotten */
    BIT_FLAGS spell_forgotten2{}; /* bit mask of spells learned but forgotten */

public:
    // [提案 41] 呪文マスクの virtual API。
    // realm_idx: 0 (realm1) または 1 (realm2)。
    // spell_id: 0..31 (realm 内呪文番号)。
    virtual BIT_FLAGS get_spell_learned_flags(int realm_idx) const
    {
        return (realm_idx == 0) ? this->spell_learned1 : this->spell_learned2;
    }
    virtual BIT_FLAGS get_spell_worked_flags(int realm_idx) const
    {
        return (realm_idx == 0) ? this->spell_worked1 : this->spell_worked2;
    }
    virtual BIT_FLAGS get_spell_forgotten_flags(int realm_idx) const
    {
        return (realm_idx == 0) ? this->spell_forgotten1 : this->spell_forgotten2;
    }
    virtual void set_spell_learned_flags(int realm_idx, BIT_FLAGS value)
    {
        (realm_idx == 0 ? this->spell_learned1 : this->spell_learned2) = value;
    }
    virtual void set_spell_worked_flags(int realm_idx, BIT_FLAGS value)
    {
        (realm_idx == 0 ? this->spell_worked1 : this->spell_worked2) = value;
    }
    virtual void set_spell_forgotten_flags(int realm_idx, BIT_FLAGS value)
    {
        (realm_idx == 0 ? this->spell_forgotten1 : this->spell_forgotten2) = value;
    }

    virtual bool has_learned_spell(int realm_idx, int spell_id) const
    {
        return (this->get_spell_learned_flags(realm_idx) & (1UL << spell_id)) != 0;
    }
    virtual bool has_worked_spell(int realm_idx, int spell_id) const
    {
        return (this->get_spell_worked_flags(realm_idx) & (1UL << spell_id)) != 0;
    }
    virtual bool has_forgotten_spell(int realm_idx, int spell_id) const
    {
        return (this->get_spell_forgotten_flags(realm_idx) & (1UL << spell_id)) != 0;
    }
    virtual void set_learned_spell(int realm_idx, int spell_id, bool value)
    {
        const auto bit = 1UL << spell_id;
        auto flags = this->get_spell_learned_flags(realm_idx);
        flags = value ? (flags | bit) : (flags & ~bit);
        this->set_spell_learned_flags(realm_idx, flags);
    }
    virtual void set_worked_spell(int realm_idx, int spell_id, bool value)
    {
        const auto bit = 1UL << spell_id;
        auto flags = this->get_spell_worked_flags(realm_idx);
        flags = value ? (flags | bit) : (flags & ~bit);
        this->set_spell_worked_flags(realm_idx, flags);
    }
    virtual void set_forgotten_spell(int realm_idx, int spell_id, bool value)
    {
        const auto bit = 1UL << spell_id;
        auto flags = this->get_spell_forgotten_flags(realm_idx);
        flags = value ? (flags | bit) : (flags & ~bit);
        this->set_spell_forgotten_flags(realm_idx, flags);
    }
    std::vector<int> spell_order_learned{}; /* order spells learned */

    // [A-3] private 化済。get_hp_table(level_index) / set_hp_table(level_index, value) 経由。
    // CreatureEntity 内部の roll_hp_table() 等は this->hp_table を直接操作。
private:
    int hp_table[PY_MAX_LEVEL]{};

public:
    std::string last_message = ""; /* Last message on death or retirement */
    char history[4][60]{}; /* Textual "history" for the Player */

    bool is_dead_{}; /* Player is dead */

    // [提案 43] now_damaged は private 化済。is_now_damaged() / set_now_damaged() 経由。
private:
    bool now_damaged{};

public:
#define KNOW_STAT 0x01
#define KNOW_HPRATE 0x02
    // [A-2] private 化済。has_knowledge() / add_knowledge() / remove_knowledge() /
    // get_knowledge() / set_knowledge() 経由。
private:
    BIT_FLAGS8 knowledge{}; /* Knowledge about yourself */

public:
    // [提案 42] old_race1/2 / old_realm は private 化済。
    // get_old_race_flags1/2() / set_old_race_flags1/2() / get_old_realm() / set_old_realm() 経由。
private:
    BIT_FLAGS old_race1{}; /* Record of race changes */
    BIT_FLAGS old_race2{}; /* Record of race changes */
    int16_t old_realm{}; /* Record of realm changes */

public:
    // [提案 40] ペット関連フィールドは private 化済。
    // get_pet_follow_distance() / set_pet_follow_distance() /
    // has_pet_extra_flag() / add_pet_extra_flag() / remove_pet_extra_flag() /
    // get_pet_extra_flags() / set_pet_extra_flags() 経由。
private:
    int16_t pet_follow_distance{}; /* Length of the imaginary "leash" for pets */
    BIT_FLAGS16 pet_extra_flags{}; /* Various flags for controling pets */

private:
    // [提案 48] private 化済。is_dtrap() / set_dtrap() 経由。
    bool dtrap{}; /* Whether you are on trap-safe grids */

public:
    FLOOR_IDX floor_id{}; /* Current floor location */

private:
    // [提案 48] private 化済。is_autopick_autoregister() / set_autopick_autoregister() 経由。
    bool autopick_autoregister{}; /* auto register is in-use or not */

public:
    /*** Temporary fields ***/

private:
    bool select_ring_slot{};

public:
    // [提案 51] select_ring_slot を private 化。is_select_ring_slot() / set_select_ring_slot() 経由。
    bool is_select_ring_slot() const
    {
        return this->select_ring_slot;
    }
    void set_select_ring_slot(bool value)
    {
        this->select_ring_slot = value;
    }

    // [提案 43] playing / leaving / monk_notify_aux / teleport_town は private 化済。
    // is_playing() / set_playing() / is_leaving() / set_leaving() /
    // get_monk_notify_aux() / set_monk_notify_aux() /
    // is_teleport_town() / set_teleport_town() 経由。
private:
    bool playing{}; /* True if player is playing */
    bool leaving{}; /* True if player is leaving */

    // [提案 47] vanish_stairs_flag は private 化済。is_vanish_stairs_flag() / set_vanish_stairs_flag() 経由。
    bool vanish_stairs_flag{}; /* True if stairs should vanish after floor change */

    bool monk_notify_aux{};

    bool teleport_town{};

    // [提案 47] tracking_bi_id は private 化済。get_tracking_bi_id() / set_tracking_bi_id() 経由。
    short tracking_bi_id{}; /* Object kind trackee */

public:
    int16_t new_spells{}; /* Number of spells available */

    // [提案 42] 差分検出キャッシュ (old_*) は private 化済。
    // get_old_spells() / set_old_spells() / was_cumber_armor() /
    // set_was_cumber_armor() / was_heavy_wield(hand) / 等のアクセサ経由。
    // old_food_aux は宣言以外で未使用のデッドフィールドだったため削除。
private:
    int16_t old_spells{};

    bool old_cumber_armor{};
    bool old_cumber_glove{};
    bool old_heavy_wield[2]{};
    bool old_heavy_shoot{};
    bool old_icky_wield[2]{};
    bool old_riding_wield[2]{};
    bool old_riding_ryoute{};
    bool old_monlite{};

    // [A-3] private 化済。get_extra_blows(hand) / set_extra_blows(hand, value) /
    // add_extra_blows(hand, delta) 経由。
private:
    int extra_blows[2]{};

public:
    // [提案 39] 装備派生キャッシュフラグは private 化済。
    // is_cumber_armor() / set_cumber_armor() / is_cumber_glove() / set_cumber_glove() /
    // is_heavy_wield(hand) / set_heavy_wield(hand, value) / is_icky_wield(hand) /
    // set_icky_wield(hand, value) / is_icky_riding_wield(hand) /
    // set_icky_riding_wield(hand, value) / is_riding_ryoute() / set_riding_ryoute() /
    // is_monlite() / set_monlite() 経由。
private:
    bool cumber_armor{}; /* Mana draining armor */
    bool cumber_glove{}; /* Mana draining gloves */
    bool heavy_wield[2]{}; /* Heavy weapon */
    bool icky_wield[2]{}; /* クラスにふさわしくない装備をしている / Icky weapon */
    bool icky_riding_wield[2]{}; /* 乗馬中に乗馬にふさわしくない装備をしている / Riding weapon */
    bool riding_ryoute{}; /* Riding weapon */
    bool monlite{};

public:
    // [提案 43] yoiyami / sutemi / fishing_dir は private 化済。
    // get_yoiyami() / set_yoiyami() / is_sutemi() / set_sutemi() /
    // get_fishing_dir() / set_fishing_dir() 経由。
private:
    BIT_FLAGS yoiyami{};

    // [A-1] private 化済。get_easy_2weapon() / set_easy_2weapon() /
    // get_down_saving() / set_down_saving() 経由。
    BIT_FLAGS easy_2weapon{};
    BIT_FLAGS down_saving{};

    bool sutemi{};

public:
private:
    bool counter{};

public:
    // [提案 51] counter を private 化。is_counter() / set_counter() 経由。
    bool is_counter() const
    {
        return this->counter;
    }
    void set_counter(bool value)
    {
        this->counter = value;
    }

private:
    DIRECTION fishing_dir{};

public:
    // [提案 40] ペット/騎乗ターゲット m_idx は private 化済。
    // get_pet_t_m_idx() / set_pet_t_m_idx() / get_riding_t_m_idx() /
    // set_riding_t_m_idx() 経由。
private:
    MONSTER_IDX pet_t_m_idx{};
    MONSTER_IDX riding_t_m_idx{};

public:
    /*** Extracted fields ***/

    // [提案 47] suppress_multi_reward は private 化済。is_suppress_multi_reward() / set_suppress_multi_reward() 経由。
    // tval_xtra は宣言以外で未使用のデッドフィールドだったため削除。
private:
    bool suppress_multi_reward{}; /*!< 複数レベルアップ時のパトロンからの報酬多重受け取りを防止 */

public:
    Dice damage_dice_bonus[2]{}; /* Extra damage dice num/sides */

private:
    bool no_flowed{};

public:
    // [提案 51] no_flowed を private 化。is_no_flowed() / set_no_flowed() 経由。
    bool is_no_flowed() const
    {
        return this->no_flowed;
    }
    void set_no_flowed(bool value)
    {
        this->no_flowed = value;
    }

private:
    // [提案 48] private 化済。get_tval_ammo() / set_tval_ammo() 経由。
    ItemKindType tval_ammo{}; /* Correct ammo tval */

    // [提案 48] private 化済。get_energy_use() / set_energy_use() /
    // add_energy_use() / sub_energy_use() / mul_energy_use() / div_energy_use() 経由。
    ENERGY energy_use{}; /*!< 直近のターンに消費したエネルギー / Energy use this turn */

public:
    std::string base_name{}; /*!< Stripped version of "player_name" */

protected:
    // 時限効果の統一ストレージ（外部からは get/set_timed_effect() 経由でアクセスすること）
    // プレイヤー・モンスター共通。提案 5 完了で全効果が map 単一管理。
    std::map<CreatureTimedEffect, TIME_EFFECT> timed_effects_map{};

    // 変身形態（外部からは get_mimic_form() / set_mimic_form() 経由でアクセスすること）
    MimicKindType mimic_form{};

    // フロア情報（外部からは get_floor() / set_floor() 経由でアクセスすること）
    FloorType *current_floor_ptr{}; /*!< 現在所属しているフロアへのポインタ / Current floor pointer */

private:
    std::string build_damage_description() const;
    std::string build_attitude_description() const;
    tl::optional<bool> order_pet_named(const CreatureEntity &other) const;
    tl::optional<bool> order_pet_hp(const CreatureEntity &other) const;
};
