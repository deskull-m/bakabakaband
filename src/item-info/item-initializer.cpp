﻿#include "item-info/item-initializer.h"
#include "world/world.h"
#include "system/baseitem-info.h"
#include "object/tval-types.h"

/*!
 * @brief ベースアイテムの未確定名を共通tval間でシャッフルする / Shuffle flavor indices of a group of objects with given tval
 * @param tval シャッフルしたいtval
 * @details 巻物、各種魔道具などに利用される。
 */
static void shuffle_flavors(ItemKindType tval)
{
    std::vector<std::reference_wrapper<IDX>> flavor_idx_ref_list;
    for (const auto &baseitem : baseitems_info) {
        if (baseitem.bi_key.tval() != tval) {
            continue;
        }

        if (baseitem.flavor == 0) {
            continue;
        }

        if (baseitem.flags.has(TR_FIXED_FLAVOR)) {
            continue;
        }

        flavor_idx_ref_list.push_back(baseitems_info[baseitem.idx].flavor);
    }

    rand_shuffle(flavor_idx_ref_list.begin(), flavor_idx_ref_list.end());
}

/*!
 * @brief ゲーム開始時に行われるベースアイテムの初期化ルーチン
 * @param なし
 */
void flavor_init(void)
{
    const auto state_backup = w_ptr->rng.get_state();
    w_ptr->rng.set_state(w_ptr->seed_flavor);
    for (auto &baseitem : baseitems_info) {
        if (baseitem.flavor_name.empty()) {
            continue;
        }

        baseitem.flavor = baseitem.idx;
    }

    shuffle_flavors(ItemKindType::RING);
    shuffle_flavors(ItemKindType::AMULET);
    shuffle_flavors(ItemKindType::STAFF);
    shuffle_flavors(ItemKindType::WAND);
    shuffle_flavors(ItemKindType::ROD);
    shuffle_flavors(ItemKindType::FOOD);
    shuffle_flavors(ItemKindType::POTION);
    shuffle_flavors(ItemKindType::SCROLL);
    w_ptr->rng.set_state(state_backup);
    for (auto &baseitem : baseitems_info) {
        if (baseitem.idx == 0 || baseitem.name.empty()) {
            continue;
        }

        if (!baseitem.flavor) {
            baseitem.aware = true;
        }

        baseitem.easy_know = object_easy_know(baseitem.idx);
    }
}
