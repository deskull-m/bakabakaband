#include "spell-realm/spells-sorcery.h"
#include "core/asking-player.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "floor/floor-object.h"
#include "game-option/input-options.h"
#include "inventory/inventory-object.h"
#include "io/input-key-requester.h"
#include "object-hook/hook-expendable.h"
#include "object/item-use-flags.h"
#include "object/object-value.h"
#include "system/creature-entity.h"
#include "system/deferred-update-requests.h"
#include "system/item-entity.h"
#include "view/display-messages.h"
#include <fmt/format.h>

/*!
 * @brief アイテムの価値に応じた錬金術処理 /
 * Turns an object into gold, gain some of its value in a shop
 * @param creature クリーチャーへの参照
 * @return 処理が実際に行われたらTRUEを返す
 */
bool alchemy(CreatureEntity &creature)
{
    bool force = false;
    if (command_arg > 0) {
        force = true;
    }

    constexpr auto q = _("どのアイテムを金に変えますか？", "Turn which item to gold? ");
    constexpr auto s = _("金に変えられる物がありません。", "You have nothing to turn to gold.");
    const auto &[item, i_idx] = choose_item(creature, q, s, (USE_INVEN | USE_FLOOR));
    if (!item) {
        return false;
    }

    auto amt = 1;
    if (item->number > 1) {
        amt = input_quantity(item->number);
        if (amt <= 0) {
            return false;
        }
    }

    const auto old_number = item->number;
    item->number = amt;
    const auto item_name = describe_flavor(creature, *item, 0);
    item->number = old_number;

    if (!force) {
        if (confirm_destroy || (item->calc_price() > 0)) {
            const auto out_val = fmt::format(_("本当に{}を金に変えますか？", "Really turn {} to gold? "), item_name);
            if (!input_check(out_val)) {
                return false;
            }
        }
    }

    if (!can_player_destroy_object(item.get())) {
        msg_print(_("{}を金に変えることに失敗した。", "You fail to turn {} to gold!"), item_name);
        return false;
    }

    auto price = object_value_real(item.get());
    if (price <= 0) {
        msg_print(_("{}をニセの金に変えた。", "You turn {} to fool's gold."), item_name);
        vary_item(creature, i_idx, -amt);
        return true;
    }

    price /= 3;

    if (amt > 1) {
        price *= amt;
    }

    if (price > 30000) {
        price = 30000;
    }

    msg_print(_("{}を＄{} の金に変えた。", "You turn {} to {} coins worth of gold."), item_name, price);
    creature.add_au(price);
    DeferredUpdateRequests::get_instance().request_gold_redrawing();
    vary_item(creature, i_idx, -amt);
    return true;
}
