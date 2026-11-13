#include "monster-floor/monster-corpse-leaver.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/floor/monster-corpse.h"
#include "system/grid-type-definition.h"

/*!
 * @brief 死亡したモンスターの死体をフロアへ遺す
 * @param creature 視点となるクリーチャー (フロアの取得に使う)
 * @param m_idx 死亡したモンスターのフロア内インデックス
 * @return 遺した死体の添字、遺せなかった場合は 0
 * @details **呼出時点のモンスターの所持品がそのまま死体へ複製される**ため、
 *          床へ放出するなら放出後に呼ぶこと。死体はグリッドのチェインへ繋ぎ、
 *          同一グリッドに複数の死体が積めるようにする (新しい死体が先頭)。
 */
short leave_monster_corpse(CreatureEntity &creature, short m_idx)
{
    auto &floor = *creature.get_floor();
    const auto &monster = floor.get_monster(m_idx);
    if (!monster.is_valid()) {
        return 0;
    }

    const auto corpse_idx = floor.pop_empty_index_corpse();
    if (corpse_idx == 0) {
        return 0;
    }

    auto &corpse = floor.get_corpse(corpse_idx);
    corpse = MonsterCorpse(monster);

    auto &grid = floor.get_grid(corpse.get_position());
    corpse.next_corpse_idx = grid.corpse_idx;
    grid.corpse_idx = corpse_idx;
    return corpse_idx;
}

/*!
 * @brief 指定グリッドに遺された死体の添字を新しい順に集める
 */
std::vector<short> collect_corpses_at(const FloorType &floor, const Pos2D &pos)
{
    std::vector<short> corpse_indices;
    if (!floor.contains(pos, FloorBoundary::OUTER_WALL_INCLUSIVE)) {
        return corpse_indices;
    }

    for (auto corpse_idx = floor.get_grid(pos).corpse_idx; corpse_idx > 0;) {
        const auto &corpse = floor.get_corpse(corpse_idx);
        corpse_indices.push_back(corpse_idx);
        corpse_idx = corpse.next_corpse_idx;
    }

    return corpse_indices;
}

/*!
 * @brief フロアの死体を全て破棄する
 * @details グリッド側のチェインも切る。clear_cave() はグリッドを個別に
 *          初期化するためそちらでも 0 を入れているが、フロア離脱のように
 *          グリッドを作り直さない経路のためにここで両方面倒を見る。
 */
void wipe_corpses(FloorType &floor)
{
    for (auto corpse_idx = 1U; corpse_idx < floor.corpses.size(); corpse_idx++) {
        const auto &corpse = floor.get_corpse(static_cast<short>(corpse_idx));
        if (!corpse.is_valid()) {
            continue;
        }

        const auto pos = corpse.get_position();
        if (floor.contains(pos, FloorBoundary::OUTER_WALL_INCLUSIVE)) {
            floor.get_grid(pos).corpse_idx = 0;
        }
    }

    floor.corpses.clear();
    floor.corpses.push_back(nullptr);
}
