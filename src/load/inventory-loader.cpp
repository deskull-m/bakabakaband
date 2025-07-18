#include "load/inventory-loader.h"
#include "inventory/inventory-slot-types.h"
#include "load/item/item-loader-factory.h"
#include "load/load-util.h"
#include "load/old/item-loader-savefile50.h"
#include "object/object-mark-types.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"

/*!
 * @brief プレイヤーの所持品情報を読み込む / Read the player inventory
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * Note that the inventory changed in Angband 2.7.4.  Two extra
 * pack slots were added and the equipment was rearranged.  Note
 * that these two features combine when parsing old save-files, in
 * which items from the old "aux" slot are "carried", perhaps into
 * one of the two new "inventory" slots.
 *
 * Note that the inventory is "re-sorted" later by "dungeon()".
 */
static errr rd_inventory(PlayerType *player_ptr)
{
    player_ptr->inven_cnt = 0;
    player_ptr->equip_cnt = 0;

    int slot = 0;
    auto item_loader = ItemLoaderFactory::create_loader();
    while (true) {
        auto n = rd_u16b();

        if (n == 0xFFFF) {
            break;
        }

        ItemEntity item;
        item_loader->rd_item(&item);
        if (!item.is_valid()) {
            return 53;
        }

        if (n >= INVEN_MAIN_HAND) {
            item.marked.set(OmType::TOUCHED);
            *player_ptr->inventory[n] = std::move(item);
            player_ptr->equip_cnt++;
            continue;
        }

        if (player_ptr->inven_cnt == INVEN_PACK) {
            load_note(_("持ち物の中のアイテムが多すぎる！", "Too many items in the inventory"));
            return 54;
        }

        n = slot++;
        item.marked.set(OmType::TOUCHED);
        *player_ptr->inventory[n] = std::move(item);
        player_ptr->inven_cnt++;
    }

    return 0;
}

errr load_inventory(PlayerType *player_ptr)
{
    for (auto i = 0; i < 64; i++) {
        if (const auto spell_id = rd_byte(); spell_id < 64) {
            player_ptr->spell_order_learned.push_back(spell_id);
        }
    }

    int errr = rd_inventory(player_ptr);
    if (!errr) {
        return 0;
    }

    load_note(_("持ち物情報を読み込むことができません", "Unable to read inventory"));
    return 21;
}
