/*!
 * @file monster-corpse.h
 * @brief フロアに遺されたモンスターの死体
 * @details 死亡したモンスターの実体 (CreatureEntity) をそのまま保持する。
 *          m_list に残さず専用コンテナ (FloorType::corpses) へ退避するのは、
 *          m_list が 75 箇所の m_max ループ・337 箇所の is_valid() 呼出から
 *          参照されており、「死んだまま残る個体」を混ぜると AI・ターゲット・
 *          compaction・添字再利用の全てを監査する必要があるため。
 */

#pragma once

#include "system/creature-entity.h"
#include "util/point-2d.h"
#include <string>

class MonsterCorpse {
public:
    MonsterCorpse();
    explicit MonsterCorpse(const CreatureEntity &monster);

    CreatureEntity body; //!< 死亡時点のモンスター実体 (所持品を含む)
    short next_corpse_idx = 0; //!< 同一グリッドの次の死体 (0 で終端)

    bool is_valid() const;
    Pos2D get_position() const;
    std::string describe() const;
};
