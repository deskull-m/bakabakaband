#include "load/store-loader.h"
#include "avatar/avatar.h"
#include "load/angband-version-comparer.h"
#include "load/item/item-loader-factory.h"
#include "load/load-util.h"
#include "load/old/item-loader-savefile50.h"
#include "object/object-stack.h"
#include "object/object-value.h"
#include "store/store-owners.h"
#include "store/store.h"
#include "system/floor/town-info.h"
#include "system/floor/town-list.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "util/object-sort.h"
#include <cstdint>

/*!
 * @brief 店置きのアイテムオブジェクトを読み込む / Add the item "o_ptr" to the inventory of the "Home"
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param store_ptr 店舗の参照ポインタ
 * @param o_ptr アイテムオブジェクト参照ポインタ
 * @details
 * In all cases, return the slot (or -1) where the object was placed
 *
 * Note that this is a hacked up version of "store_item_to_inventory()".
 *
 * Also note that it may not correctly "adapt" to "knowledge" bacoming
 * known, the player may have to pick stuff up and drop it again.
 */
static void home_carry_load(PlayerType *player_ptr, store_type *store_ptr, ItemEntity *o_ptr)
{
    for (auto i = 0; i < store_ptr->stock_num; i++) {
        auto &item = *store_ptr->stock[i];
        if (!item.is_similar(*o_ptr)) {
            continue;
        }

        item.absorb(*o_ptr);
        return;
    }

    if (store_ptr->stock_num >= store_get_stock_max(StoreSaleType::HOME)) {
        return;
    }

    const auto first = store_ptr->stock.begin();
    const auto last = store_ptr->stock.begin() + store_ptr->stock_num;
    const auto slot_it = std::find_if(first, last,
        [&](const auto &item) { return object_sort_comp(player_ptr, *o_ptr, *item); });
    const int slot = std::distance(first, slot_it);

    std::rotate(first + slot, last, last + 1);

    store_ptr->stock_num++;
    *store_ptr->stock[slot] = o_ptr->clone();
    chg_virtue(player_ptr, Virtue::SACRIFICE, -1);
}

/*!
 * @brief 店舗情報を読み込む
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param town_number_initial 街ID (v0.3.3以降)
 * @param store_number 店舗ID
 */
static void rd_store(PlayerType *player_ptr, int town_number_initial, StoreSaleType store_number)
{
    const auto town_number = town_number_initial;
    auto &store = towns_info[town_number].get_store(store_number);
    auto sort = store.stock_num > 0;
    store.store_open = rd_s32b();
    store.insult_cur = rd_s16b();
    store.owner = rd_byte();

    if (auto num = owners.at(store_number).size();
        num <= store.owner) {
        store.owner %= num;
    }

    int16_t inven_num = rd_s16b();

    store.good_buy = rd_s16b();
    store.bad_buy = rd_s16b();
    store.last_visit = rd_s32b();

    auto item_loader = ItemLoaderFactory::create_loader();
    for (int j = 0; j < inven_num; j++) {
        ItemEntity item;
        item_loader->rd_item(&item);
        auto stock_max = store_get_stock_max(store_number);
        if (store.stock_num >= stock_max) {
            continue;
        }

        if (sort) {
            home_carry_load(player_ptr, &store, &item);
        } else {
            int k = store.stock_num++;
            *store.stock[k] = std::move(item);
        }
    }
}

/*!
 * @brief 店舗情報を読み込む
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void load_store(PlayerType *player_ptr)
{
    const int town_count = rd_u16b();
    const int store_count = rd_u16b();
    for (auto town_idx = 1; town_idx < town_count; town_idx++) {
        for (auto store_idx = 0; store_idx < store_count; store_idx++) {
            rd_store(player_ptr, town_idx, i2enum<StoreSaleType>(store_idx));
        }
    }
}
