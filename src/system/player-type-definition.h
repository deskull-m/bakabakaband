#pragma once

#include "system/creature-entity.h"
#include <string_view>

class PlayerType : public CreatureEntity {
public:
    PlayerType();

    bool is_valid() const override;
    bool is_dead() const override;

    bool is_player() const override;

    void on_take_hit(int damage) override;
    void on_death(std::string_view cause) override;
    bool calc_damage_reduction(int &damage, int damage_type) override;

    void wipe() override;

    /*!
     * @brief プレイヤー唯一のインスタンスを取得する
     * @return プレイヤーインスタンスへの参照
     * @note 原則としてエントリポイント・シグナルハンドラ・UIコールバック等の
     * 「プレイヤー参照の根」でのみ使用すること。通常のゲームロジックからは
     * CreatureEntity & 引数経由で受け取ること。
     */
    static PlayerType &get_instance();

    /*!
     * @brief プレイヤー唯一のインスタンスを取得する（ポインタ版）
     * @return プレイヤーインスタンスへのポインタ
     * @note 用途は get_instance() と同じ。ポインタが必要な場所専用。
     */
    static PlayerType *get_instance_ptr();
};
