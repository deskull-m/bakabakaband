#pragma once

#include "mutation/mutation-flag-types.h"
#include "player-ability/player-ability-types.h"
#include "player/player-personality-types.h"
#include "system/angband.h"
#include "util/flag-group.h"
#include "util/point-2d.h"
#include <map>
#include <memory>
#include <vector>

class FloorType;
class ItemEntity;

// Forward declarations for race/class/personality info
struct player_race_info;
struct player_personality;
struct player_class_info;
enum class MonraceId : int16_t;
enum class Virtue : short;
enum player_sex : byte;
enum class PlayerRaceType;
enum class PlayerClassType : short;

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
    virtual Pos2D get_position() const = 0;

    /*!
     * @brief クリーチャーのX座標を取得
     * @return X座標
     */
    virtual POSITION get_x() const = 0;

    /*!
     * @brief クリーチャーのY座標を取得
     * @return Y座標
     */
    virtual POSITION get_y() const = 0;

    /*!
     * @brief クリーチャーの現在HPを取得
     * @return 現在HP
     */
    virtual int get_current_hp() const = 0;

    /*!
     * @brief クリーチャーの最大HPを取得
     * @return 最大HP
     */
    virtual int get_max_hp() const = 0;

    /*!
     * @brief クリーチャーの速度を取得
     * @return 速度値
     */
    virtual int get_speed() const
    {
        return this->speed;
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
     */
    virtual bool is_valid() const = 0;

    /*!
     * @brief クリーチャーが死亡しているかどうかを判定
     * @return 死亡していればtrue
     */
    virtual bool is_dead() const = 0;

    /*!
     * @brief クリーチャーが所属するフロアを取得
     * @return フロアへのポインタ
     */
    virtual FloorType *get_floor() const = 0;

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
     * @return レベル値
     */
    virtual int get_level() const
    {
        return this->level;
    }

    /*!
     * @brief クリーチャーがプレイヤーかどうかを判定
     * @return プレイヤーならtrue、モンスターならfalse
     */
    virtual bool is_player() const = 0;

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

    // 所持金関連
    PRICE au{}; /*!< 所持金 / Current Gold */

    // 名前関連
    std::string name{}; /*!< クリーチャーの名前（プレイヤー名またはペット名） / Creature's name (player name or pet nickname) */
};
