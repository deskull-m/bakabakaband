#pragma once

#include "system/creature-entity.h"
#include <array>

/*!
 * @brief フロア移動時にペットを保存するコンテナクラス
 */
class PartyMonsters {
public:
    static constexpr int MAX_SIZE = 21; /*!< フロア移動時に先のフロアに連れて行けるペットの最大数 */

    CreatureEntity &operator[](int i);
    const CreatureEntity &operator[](int i) const;

    void invalidate_all();
    void wipe_all();
    void precalc_cur_num_of_pet() const;

    auto begin()
    {
        return monsters_.begin();
    }
    auto end()
    {
        return monsters_.end();
    }
    auto begin() const
    {
        return monsters_.begin();
    }
    auto end() const
    {
        return monsters_.end();
    }

private:
    std::array<CreatureEntity, MAX_SIZE> monsters_{};
};

extern PartyMonsters party_monsters;
