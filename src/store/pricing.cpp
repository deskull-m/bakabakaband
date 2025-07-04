#include "store/pricing.h"
#include "object/object-value.h"
#include "player/player-status-table.h"
#include "store/gold-magnification-table.h"
#include "store/store-owners.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "util/enum-converter.h"

/*!
 * @brief 店舗価格を決定する. 無料にはならない /
 * Determine the price of an item (qty one) in a store.
 * @param o_ptr 店舗に並べるオブジェクト構造体の参照ポインタ
 * @param greed 店主の強欲度
 * @param flip TRUEならば店主にとっての買取価格、FALSEなら売出価格を計算
 * @return アイテムの店舗価格
 * @details
 * <pre>
 * This function takes into account the player's charisma, and the
 * shop-keepers friendliness, and the shop-keeper's base greed, but
 * never lets a shop-keeper lose money in a transaction.
 * The "greed" value should exceed 100 when the player is "buying" the
 * item, and should be less than 100 when the player is "selling" it.
 * Hack -- the black market always charges twice as much as it should.
 * Charisma adjustment runs from 80 to 130
 * Racial adjustment runs from 95 to 130
 * Since greed/charisma/racial adjustments are centered at 100, we need
 * to adjust (by 200) to extract a usable multiplier.  Note that the
 * "greed" value is always something (?).
 * </pre>
 */
int price_item(PlayerType *player_ptr, const ItemEntity *o_ptr, int greed, bool flip, StoreSaleType store_num)
{
    auto price = o_ptr->calc_price();
    if (price <= 0) {
        return 0L;
    }

    int factor = rgold_adj[enum2i(ot_ptr->owner_race)][enum2i(player_ptr->prace)] - std::min(player_ptr->prestige / 10, 30);
    factor += adj_chr_gold[player_ptr->stat_index[A_CHR]];
    int adjust;
    if (flip) {
        adjust = 100 + (300 - (greed + factor));
        if (adjust > 100) {
            adjust = 100;
        }

        if (store_num == StoreSaleType::BLACK) {
            price = price / 2;
        }

        price = (price * adjust + 50L) / 100L;
    } else {
        adjust = 100 + ((greed + factor) - 300);
        if (adjust < 100) {
            adjust = 100;
        }

        if (store_num == StoreSaleType::BLACK) {
            price = price * 2;
        }

        price = (int32_t)(((uint32_t)price * (uint32_t)adjust + 50UL) / 100UL);
    }

    if (price <= 0L) {
        return 1L;
    }

    if (price >= LOW_PRICE_THRESHOLD) {
        price += (flip ? -1 : 1) * price / 10;
    }

    return price;
}
