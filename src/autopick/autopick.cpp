/*!
 * @file autopick.c
 * @brief 自動拾い機能の実装 / Object Auto-picker/Destroyer
 * @date 2014/01/02
 * @author
 * Copyright (c) 2002  Mogami\n
 *\n
 * This software may be copied and distributed for educational, research, and\n
 * not for profit purposes provided that this copyright and statement are\n
 * included in all such copies.\n
 * 2014 Deskull rearranged comment for Doxygen.\n
 */

#include "autopick/autopick.h"
#include "autopick/autopick-destroyer.h"
#include "autopick/autopick-finder.h"
#include "autopick/autopick-menu-data-table.h"
#include "autopick/autopick-methods-table.h"
#include "autopick/autopick-util.h"
#include "core/asking-player.h"
#include "core/disturbance.h"
#include "flavor/flavor-describer.h"
#include "floor/floor-object.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "inventory/player-inventory.h"
#include "object/object-info.h"
#include "object/object-mark-types.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "term/screen-processor.h"
#include "view/display-messages.h"
#include "window/display-sub-windows.h"
#include <sstream>

/*!
 * @brief Auto-destroy marked item
 */
static void autopick_delayed_alter_aux(CreatureEntity &creature, INVENTORY_IDX i_idx)
{
    const auto &item = ref_item(creature, i_idx);
    if (!item->is_valid() || item->marked.has_not(OmType::AUTODESTROY)) {
        return;
    }

    const auto item_name = describe_flavor(creature, *item, 0);
    if (i_idx >= 0) {
        inven_item_increase(creature, i_idx, -(item->number));
        inven_item_optimize(creature, i_idx);
    } else {
        delete_object_idx(creature, 0 - i_idx);
    }

    msg_format(_("%sを自動破壊します。", "Auto-destroying %s."), item_name.data());
}

/*!
 * @brief Auto-destroy marked items in inventry and on floor
 * @details
 * Scan inventry in reverse order to prevent
 * skipping after inven_item_optimize()
 */
void autopick_delayed_alter(CreatureEntity &creature)
{
    for (INVENTORY_IDX i_idx = INVEN_TOTAL - 1; i_idx >= 0; i_idx--) {
        autopick_delayed_alter_aux(creature, i_idx);
    }

    const auto p_pos = creature.get_position();
    auto &grid = creature.get_floor()->get_grid(p_pos);
    for (auto it = grid.o_idx_list.begin(); it != grid.o_idx_list.end();) {
        INVENTORY_IDX i_idx = *it++;
        autopick_delayed_alter_aux(creature, -i_idx);
    }

    // PW_FLOOR_ITEM_LISTは遅れるので即時更新
    fix_floor_item_list(creature, p_pos);
}

/*!
 * @brief Auto-inscription and/or destroy
 * @details
 * Auto-destroyer works only on inventory or on floor stack only when
 * requested.
 */
void autopick_alter_item(CreatureEntity &creature, INVENTORY_IDX i_idx, bool destroy)
{
    auto item = ref_item(creature, i_idx);
    int idx = find_autopick_list(creature, item.get());
    auto_inscribe_item(item.get(), idx);
    if (destroy && i_idx <= INVEN_PACK) {
        auto_destroy_item(creature, item.get(), idx);
    }
}

/*!
 * @brief Automatically pickup/destroy items in this grid.
 */
void autopick_pickup_items(CreatureEntity &creature, const Grid &grid)
{
    for (auto it = grid.o_idx_list.begin(); it != grid.o_idx_list.end();) {
        OBJECT_IDX this_o_idx = *it++;
        auto &item = *creature.get_floor()->o_list[this_o_idx];
        int idx = find_autopick_list(creature, &item);
        auto_inscribe_item(&item, idx);
        if ((idx < 0) || (autopick_list[idx].action.has_none_of({ AutopickMethod::AUTOPICK, AutopickMethod::QUERY_AUTOPICK }))) {
            auto_destroy_item(creature, &item, idx);
            continue;
        }

        disturb(creature, false, false);
        const auto item_name = describe_flavor(creature, item, 0);

        if (!check_get_item(&item)) {
            msg_format(_("%sを持ち運ぶことはできない。", "You can't carry %s."), item_name.data());
            item.marked.set(OmType::SUPRESS_MESSAGE);
            continue;
        }

        if (!creature.can_store_item(item)) {
            msg_format(_("ザックには%sを入れる隙間がない。", "You have no room for %s."), item_name.data());
            item.marked.set(OmType::SUPRESS_MESSAGE);
            continue;
        }

        if (!(autopick_list[idx].action.has(AutopickMethod::QUERY_AUTOPICK))) {
            process_player_pickup_item(creature, this_o_idx);
            continue;
        }

        if (item.marked.has(OmType::NO_QUERY)) {
            continue;
        }

        std::stringstream ss;
        ss << _(item_name, "Pick up ") << _("を拾いますか", item_name) << "? ";
        if (!input_check(ss.str())) {
            item.marked.set({ OmType::SUPRESS_MESSAGE, OmType::NO_QUERY });
            continue;
        }

        process_player_pickup_item(creature, this_o_idx);
    }
}
