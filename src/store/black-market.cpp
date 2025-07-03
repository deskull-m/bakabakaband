#include "store/black-market.h"
#include "store/store-owners.h"
#include "store/store-util.h"
#include "system/floor/town-info.h"
#include "system/floor/town-list.h"
#include "system/item-entity.h"

/*!
 * @brief ブラックマーケット用の無価値品の排除判定 /
 * This function will keep 'crap' out of the black market.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
 * @return ブラックマーケットにとって無価値な品ならばTRUEを返す
 * @details
 * <pre>
 * Crap is defined as any item that is "available" elsewhere
 * Based on a suggestion by "Lee Vogt" <lvogt@cig.mcel.mot.com>
 * </pre>
 */
bool black_market_crap(int town_num, const ItemEntity &item)
{
    if (item.is_ego()) {
        return false;
    }

    if (item.to_a > 0) {
        return false;
    }

    if (item.to_h > 0) {
        return false;
    }

    if (item.to_d > 0) {
        return false;
    }

    const auto &town = towns_info[town_num];
    for (auto sst : STORE_SALE_TYPE_LIST) {
        if ((sst == StoreSaleType::HOME) || (sst == StoreSaleType::MUSEUM)) {
            continue;
        }

        const auto &store = town.get_store(sst);
        for (auto j = 0; j < store.stock_num; j++) {
            if (item.bi_id == store.stock[j]->bi_id) {
                return true;
            }
        }
    }

    return false;
}
