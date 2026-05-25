#pragma once

#include "system/enums/store-sale-type.h"
#include "system/item-entity.h"
#include "util/enum-converter.h"
#include "util/enum-range.h"
#include <memory>
#include <tl/optional.hpp>
#include <vector>

constexpr DEPTH STORE_OBJ_STD_LEVEL = 5; //!< 通常店舗の標準階層レベル / Magic Level for normal stores
constexpr auto MAX_STORES = enum2i(StoreSaleType::MAX); /*!< 店舗の種類最大数 / Total number of stores (see "store.c", etc) */
constexpr auto STORE_SALE_TYPE_LIST = EnumRange(StoreSaleType::GENERAL, StoreSaleType::MAX);

/*!
 * @brief 店舗の情報構造体
 */
class Store {
public:
    Store() = default;
    Store(const Store &) = delete;
    Store(Store &&) = delete;
    Store &operator=(const Store &) = delete;
    Store &operator=(Store &&) = delete;

    uint8_t type{}; //!< Store type
    uint8_t owner{}; //!< Owner index
    uint8_t extra{}; //!< Unused for now
    short insult_cur{}; //!< Insult counter
    short good_buy{}; //!< Number of "good" buys
    short bad_buy{}; //!< Number of "bad" buys
    int store_open{}; //!< Closed until this turn
    int last_visit{}; //!< Last visited on this turn
    std::vector<short> regular{}; //!< Table -- Legal regular item kinds
    std::vector<short> table{}; //!< Table -- Legal item kinds
    short stock_num{}; //!< Stock -- Number of entries
    short stock_size{}; //!< @todo vectorのサイズを取れば良くなったので後ほど削除する.
    std::vector<std::unique_ptr<ItemEntity>> stock{}; //!< Stock -- Actual stock items

    void increase_item(short i_idx, int item_num);
    void optimize_item(short i_idx);
    void delete_item();
    std::vector<short> collect_same_magic_device_pvals(ItemEntity &item);
    tl::optional<int> carry(ItemEntity &item);
};

extern Store *st_ptr;

std::string get_store_sale_type_tag(StoreSaleType store_type);
