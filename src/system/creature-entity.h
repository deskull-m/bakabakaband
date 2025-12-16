#pragma once

#include "player-ability/player-ability-types.h"
#include "system/angband.h"
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
    virtual int get_speed() const = 0;

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
    virtual ACTION_ENERGY get_energy_need() const = 0;

    /*!
     * @brief 次の行動までに必要なエネルギーを設定
     * @param energy エネルギー値
     */
    virtual void set_energy_need(ACTION_ENERGY energy) = 0;

    /*!
     * @brief クリーチャーのレベルを取得
     * @return レベル値
     */
    virtual int get_level() const = 0;

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

    // インベントリ関連
    std::vector<std::shared_ptr<ItemEntity>> inventory{}; /*!< 所持品リスト / The creature's inventory */
    int16_t inven_cnt{}; /*!< 所持品数 / Number of items in inventory */
    int16_t equip_cnt{}; /*!< 装備品数 / Number of items in equipment */
};
