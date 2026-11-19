/*!
 * @file monster-corpse-leaver.h
 * @brief モンスターの死体をフロアへ遺す処理
 */

#pragma once

#include "util/point-2d.h"
#include <memory>
#include <utility>
#include <vector>

class CreatureEntity;
class FloorType;
class ItemEntity;
class MonsterCorpse;

short leave_monster_corpse(CreatureEntity &creature, short m_idx);
std::vector<short> collect_corpses_at(const FloorType &floor, const Pos2D &pos);
void wipe_corpses(FloorType &floor);
bool lay_remains_as_corpses(FloorType &floor, const ItemEntity &item, const Pos2D &pos);
void remove_corpse(CreatureEntity &creature, short corpse_idx, bool spill_belongings);

/*!
 * @brief 足元の死体を、コマンドの間だけ床のアイテムとして並べる
 * @details 食べる・壊すコマンドで、足元の死体を床のアイテムと同じ選択肢 ('-' で床) に
 *          出すためのもの。生成時に死体・骨のアイテムを床へ置き、破棄時に
 *          - アイテムが残っていれば (選ばれなかった) 黙って取り除いて死体はそのまま
 *          - アイテムが無くなっていれば (食べた・壊した) 死体を外し、残っていた持ち物を床へ散らす
 *          を行う。床へ置くのは drop_near() を通さず直接で、死体へ戻す変換は掛からない。
 */
class ScopedCorpseFloorItems {
public:
    explicit ScopedCorpseFloorItems(CreatureEntity &creature);
    ~ScopedCorpseFloorItems();
    ScopedCorpseFloorItems(const ScopedCorpseFloorItems &) = delete;
    ScopedCorpseFloorItems &operator=(const ScopedCorpseFloorItems &) = delete;

private:
    CreatureEntity &creature;
    //! (床に置いたアイテムの実体, 死体の添字)。床のアイテムの添字は delete_object_idx() が
    //! 末尾の品を詰めるたびに変わるので、添字ではなく実体で追跡する
    std::vector<std::pair<std::shared_ptr<ItemEntity>, short>> placed;
};
