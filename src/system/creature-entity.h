#pragma once

#include "system/angband.h"
#include "util/point-2d.h"
#include <memory>
#include <vector>

class FloorType;
class ItemEntity;

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

    // インベントリ関連
    std::vector<std::shared_ptr<ItemEntity>> inventory{}; /*!< 所持品リスト / The creature's inventory */
    int16_t inven_cnt{}; /*!< 所持品数 / Number of items in inventory */
    int16_t equip_cnt{}; /*!< 装備品数 / Number of items in equipment */
};
