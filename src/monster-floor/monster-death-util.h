#pragma once

#include "util/point-2d.h"
#include <cstdint>
#include <memory>

// @todo PlayerType に依存するオブジェクトメソッドを追加予定.
class FloorType;
class CreatureEntity;
class MonraceDefinition;
class MonsterDeath {
public:
    MonsterDeath(FloorType &floor, short m_idx, bool drop_item);
    short m_idx;
    CreatureEntity *m_ptr;
    std::shared_ptr<MonraceDefinition> monrace;
    std::shared_ptr<MonraceDefinition> apparent_monrace;
    bool do_gold;
    bool do_item;
    bool cloned;
    bool is_chameleon;
    bool drop_chosen_item;

    /*!
     * @brief この個体が死体を遺すか
     * @details drop_corpse() が確率抽選を含む判定を行って立てる。乱数を 2 度
     *          消費しないよう、死体の実体生成はこのフラグを見て後段で行う。
     */
    bool leaves_corpse = false;
    int md_y = 0;
    int md_x = 0;
    uint32_t mo_mode = 0;

    Pos2D get_position() const;
};
