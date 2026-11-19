#include "monster-floor/monster-corpse-leaver.h"
#include "floor/floor-object.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "object/object-mark-types.h"
#include "system/creature-entity.h"
#include "system/deferred-update-requests.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/floor/monster-corpse.h"
#include "system/gamevalue.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-list.h"
#include "util/enum-converter.h"

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

namespace {
/*!
 * @brief 空いている死体の添字の数を数える
 * @details 取り除かれた死体の添字 (released) は再利用されるので、それも空きに数える。
 */
int count_free_corpse_slots(const FloorType &floor)
{
    auto free_slots = MAX_FLOOR_CORPSES - static_cast<int>(floor.corpses.size());
    for (auto corpse_idx = 1U; corpse_idx < floor.corpses.size(); corpse_idx++) {
        if (floor.get_corpse(static_cast<short>(corpse_idx)).released) {
            free_slots++;
        }
    }

    return free_slots;
}

/*!
 * @brief 死体をグリッドのチェインから外す
 */
void unlink_corpse(FloorType &floor, short corpse_idx)
{
    const auto &corpse = floor.get_corpse(corpse_idx);
    const auto pos = corpse.get_position();
    if (!floor.contains(pos, FloorBoundary::OUTER_WALL_INCLUSIVE)) {
        return;
    }

    auto &grid = floor.get_grid(pos);
    if (grid.corpse_idx == corpse_idx) {
        grid.corpse_idx = corpse.next_corpse_idx;
        return;
    }

    for (auto idx = grid.corpse_idx; idx > 0;) {
        auto &prev = floor.get_corpse(idx);
        if (prev.next_corpse_idx == corpse_idx) {
            prev.next_corpse_idx = corpse.next_corpse_idx;
            return;
        }

        idx = prev.next_corpse_idx;
    }
}
}

/*!
 * @brief 床に置かれた死体・骨のアイテムを死体へ戻す
 * @param floor 置く先のフロア
 * @param item 置かれたアイテム。MonsterCorpse::is_remains_item() が真でなければ何もしない
 * @param pos 置く座標 (呼出側で配置可能なグリッドを選んであること)
 * @return 死体へ戻したら true。false のときは呼出側が従来どおり床のアイテムとして置く
 * @details 床に横たわる死体を MonsterCorpse だけで表すための入口。drop_near() /
 *          place_object() 等、床へアイテムを置く経路から呼ぶ。
 *          スタック (number > 1) は 1 体ずつの死体に分ける。**空き添字が個数ぶん
 *          無ければ 1 体も戻さない** (一部だけ戻すと呼出側の個数の扱いが複雑になるため)。
 *          持ち運んでいた死体アイテムは種族しか覚えていないので、戻した死体は
 *          生前の個体名や所持品を持たない、その種族の亡骸になる。
 */
bool lay_remains_as_corpses(FloorType &floor, const ItemEntity &item, const Pos2D &pos)
{
    if (!MonsterCorpse::is_remains_item(item)) {
        return false;
    }

    const auto monrace_id = i2enum<MonraceId>(item.pval);
    if (!MonraceList::is_valid(monrace_id) || !floor.contains(pos, FloorBoundary::OUTER_WALL_INCLUSIVE)) {
        return false;
    }

    if (count_free_corpse_slots(floor) < item.number) {
        return false;
    }

    auto &grid = floor.get_grid(pos);
    for (auto i = 0; i < item.number; i++) {
        const auto corpse_idx = floor.pop_empty_index_corpse();
        auto &corpse = floor.get_corpse(corpse_idx);
        corpse.body.polymorph_to(monrace_id);
        corpse.body.set_position(pos);
        corpse.body.set_floor(&floor);
        corpse.remains_sval = *item.bi_key.sval();
        corpse.next_corpse_idx = grid.corpse_idx;
        grid.corpse_idx = corpse_idx;
    }

    return true;
}

/*!
 * @brief 死体をフロアから取り除く
 * @param creature 視点となるクリーチャー (フロアの取得と、持ち物を床へ出す際に使う)
 * @param corpse_idx 取り除く死体の添字
 * @param spill_belongings 残っている持ち物を床へ出すなら true
 * @details 添字は released にして以後の死体に再利用させる。持ち去る・食べる経路は
 *          持ち物が残っている死体を受け付けないので spill_belongings は false でよく、
 *          破壊・復活のように死体が持ち主の意思に依らず消える経路だけが true を渡す
 *          (品を黙って消さないため)。
 */
void remove_corpse(CreatureEntity &creature, short corpse_idx, bool spill_belongings)
{
    auto &floor = *creature.get_floor();
    auto &corpse = floor.get_corpse(corpse_idx);
    if (corpse.released) {
        return;
    }

    const auto pos = corpse.get_position();
    unlink_corpse(floor, corpse_idx);

    std::vector<ItemEntity> belongings;
    if (spill_belongings) {
        for (auto slot = 0; slot < INVEN_TOTAL; slot++) {
            const auto &item_ptr = corpse.body.inventory[slot];
            if (item_ptr && item_ptr->is_valid()) {
                belongings.push_back(item_ptr->clone());
            }
        }
    }

    corpse = MonsterCorpse();
    corpse.released = true;

    // 死体を外してから床へ出す (drop_near() が死体の空き添字を数えることがあるため)
    for (auto &item : belongings) {
        (void)drop_near(creature, item, pos, false);
    }
}

namespace {
/*!
 * @brief 床の品の実体から現在の添字を探す (見つからなければ 0)
 */
short find_floor_item_index(const FloorType &floor, const std::shared_ptr<ItemEntity> &item_ptr)
{
    for (auto item_idx = 1U; item_idx < floor.o_list.size(); item_idx++) {
        if (floor.o_list[item_idx] == item_ptr) {
            return static_cast<short>(item_idx);
        }
    }

    return 0;
}
}

ScopedCorpseFloorItems::ScopedCorpseFloorItems(CreatureEntity &creature)
    : creature(creature)
{
    auto &floor = *creature.get_floor();
    const auto pos = creature.get_position();
    for (const auto corpse_idx : collect_corpses_at(floor, pos)) {
        if (std::ssize(floor.o_list) >= MAX_FLOOR_ITEMS) {
            break;
        }

        const auto item_idx = floor.pop_empty_index_item();
        auto &item = *floor.o_list[item_idx];
        item = floor.get_corpse(corpse_idx).make_remains_item();
        item.set_position(pos);
        item.marked.set(OmType::FOUND);
        floor.get_grid(pos).o_idx_list.add(floor, item_idx);
        this->placed.emplace_back(floor.o_list[item_idx], corpse_idx);
    }

    if (!this->placed.empty()) {
        DeferredUpdateRequests::get_instance().request_floor_item_lists_redrawing();
    }
}

ScopedCorpseFloorItems::~ScopedCorpseFloorItems()
{
    auto &floor = *this->creature.get_floor();
    for (const auto &[item_ptr, corpse_idx] : this->placed) {
        // 床に残っていれば選ばれなかったので取り除くだけ。死体はそのまま残る
        const auto item_idx = find_floor_item_index(floor, item_ptr);
        if (item_idx > 0) {
            delete_object_idx(this->creature, item_idx);
            continue;
        }

        // 床から無くなっていれば食べられた・壊された。死体を外して持ち物を周りへ散らす
        remove_corpse(this->creature, corpse_idx, true);
    }

    if (!this->placed.empty()) {
        lite_spot(this->creature, this->creature.get_position());
        DeferredUpdateRequests::get_instance().request_floor_item_lists_redrawing();
    }
}
