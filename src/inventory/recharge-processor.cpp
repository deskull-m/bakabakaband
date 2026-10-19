#include "inventory/recharge-processor.h"
#include "core/disturbance.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "hpmp/hp-mp-regenerator.h"
#include "inventory/inventory-slot-types.h"
#include "object/object-mark-types.h"
#include "object/tval-types.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/item-entity.h"
#include "system/redrawing-flags-updater.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

/*!
 * @brief
 * !!を刻んだ魔道具の時間経過による再充填を知らせる処理 /
 * If creature has inscribed the object with "!!", let him know when it's recharged. -LM-
 * @param o_ptr 対象オブジェクトの構造体参照ポインタ
 */
static void recharged_notice(CreatureEntity &creature, const ItemEntity &item)
{
    if (!item.is_inscribed()) {
        return;
    }

    auto s = angband_strchr(item.inscription->data(), '!');
    while (s) {
        if (s[1] == '!') {
            const auto item_name = describe_flavor(creature, item, (OD_OMIT_PREFIX | OD_NAME_ONLY));
#ifdef JP
            msg_print("{}は再充填された。", item_name);
#else
            if (item.number > 1) {
                msg_print("Your {} are recharged.", item_name);
            } else {
                msg_print("Your {} is recharged.", item_name);
            }
#endif
            disturb(creature, false, false);
            return;
        }

        s = angband_strchr(s + 1, '!');
    }
}

/*!
 * @brief 表示上の充填中の本数を求める (ロッドの一覧表示の「(n本 充填中)」に使う)
 * @param item 対象のアイテム
 * @return 表示上の充填中の本数
 */
static int calc_displayed_charging_count(const ItemEntity &item)
{
    if (item.timeout <= 0) {
        return 0;
    }

    if (item.number <= 1) {
        return 1;
    }

    const auto timeout_per_one = item.get_baseitem_pval();
    auto num_of_charging = (item.timeout + (timeout_per_one - 1)) / timeout_per_one;
    if (num_of_charging > item.number) {
        num_of_charging = item.number;
    }

    return num_of_charging;
}

/*!
 * @brief 10ゲームターンが進行するごとに魔道具の自然充填を行う処理
 * / Handle recharging objects once every 10 game turns
 */
void recharge_magic_items(CreatureEntity &creature)
{
    bool changed = false;
    for (const auto i_idx : INVEN_WIELDING_SLOTS) {
        auto &item = *creature.inventory[i_idx];
        if (!item.is_valid()) {
            continue;
        }

        if (item.timeout > 0) {
            item.timeout--;
            if (!item.timeout) {
                recharged_notice(creature, item);
                changed = true;
            }
        }
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (changed) {
        rfu.set_flag(SubWindowRedrawingFlag::EQUIPMENT);
        wild_regen = 20;
    }

    /*
     * Recharge rods.  Rods now use timeout to control charging status,
     * and each charging rod in a stack decreases the stack's timeout by
     * one per turn. -LM-
     */
    changed = false;
    for (const auto i_idx : INVEN_PACK_SLOTS) {
        auto &item = *creature.inventory[i_idx];
        if (!item.is_valid()) {
            continue;
        }

        if ((item.bi_key.tval() != ItemKindType::ROD) || (item.timeout == 0)) {
            continue;
        }

        const auto base_pval = item.get_baseitem_pval();
        short temp = (item.timeout + (base_pval - 1)) / base_pval;
        if (temp > item.number) {
            temp = static_cast<short>(item.number);
        }

        item.timeout -= temp;
        if (item.timeout < 0) {
            item.timeout = 0;
        }

        if (!(item.timeout)) {
            recharged_notice(creature, item);
            changed = true;
        } else if (item.timeout % base_pval) {
            changed = true;
        }
    }

    if (changed) {
        rfu.set_flag(SubWindowRedrawingFlag::INVENTORY);
        wild_regen = 20;
    }

    for (const auto &item_ptr : creature.get_floor()->o_list) {
        if (!item_ptr->is_valid()) {
            continue;
        }

        if ((item_ptr->bi_key.tval() == ItemKindType::ROD) && (item_ptr->timeout)) {
            // 一覧に出るロッド (モンスターが拾うと FOUND は消える) の充填中の表示が変わったときだけ描き直す
            const auto is_listed = item_ptr->marked.has(OmType::FOUND) && item_ptr->is_known();
            const auto old_charging_count = is_listed ? calc_displayed_charging_count(*item_ptr) : 0;
            item_ptr->timeout -= (TIME_EFFECT)item_ptr->number;
            if (item_ptr->timeout < 0) {
                item_ptr->timeout = 0;
            }

            if (is_listed && (calc_displayed_charging_count(*item_ptr) != old_charging_count)) {
                rfu.request_floor_item_lists_redrawing();
            }
        }
    }
}
