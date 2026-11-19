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

class ItemEntity;

/*!
 * @brief フロアに遺された死体
 * @details **床に横たわる死体・骨はこのクラスだけが表す**。死体・骨のアイテム
 *          (MONSTER_REMAINS の SV_CORPSE / SV_SKELETON) は持ち運んでいる間だけの姿で、
 *          持ち去るときに make_remains_item() でアイテムになり、床へ置かれると
 *          drop_near() 等が死体へ戻す (monster-corpse-leaver.h の lay_remains_as_corpses())。
 */
class MonsterCorpse {
public:
    MonsterCorpse();
    explicit MonsterCorpse(const CreatureEntity &monster);

    CreatureEntity body; //!< 死亡時点のモンスター実体 (所持品を含む)
    short next_corpse_idx = 0; //!< 同一グリッドの次の死体 (0 で終端)
    int remains_sval; //!< 亡骸の形 (SV_CORPSE = 死体 / SV_SKELETON = 骨)
    bool released = false; //!< フロアから取り除かれ、添字を再利用してよい状態か

    bool is_valid() const;
    bool is_skeleton() const;
    bool has_belongings() const;
    Pos2D get_position() const;
    std::string describe() const;
    ItemEntity make_remains_item() const;

    static bool is_remains_item(const ItemEntity &item);
};
