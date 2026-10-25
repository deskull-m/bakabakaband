/*!
 * @brief RedrawingFlagsUpdater のテスト
 */

#include "system/redrawing-flags-updater.h"
#include "util/finalizer.h"
#include <doctest/doctest.h>

namespace {

/*!
 * @brief フラグを元の状態に戻す
 * @param rfu 更新フラグ
 * @param flag 戻すフラグ
 * @param was_set 元の状態でフラグが立っていたか
 */
template <typename Flag>
void restore_flag(RedrawingFlagsUpdater &rfu, Flag flag, bool was_set)
{
    if (was_set) {
        rfu.set_flag(flag);
    } else {
        rfu.reset_flag(flag);
    }
}

}

TEST_CASE("RedrawingFlagsUpdater::request_inventory_arrangement requests combining, reordering and redrawing the inventory")
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    const auto restore = util::make_finalizer([&rfu,
                                                  auto_destruction = rfu.has(InventoryArrangementFlag::AUTO_DESTRUCTION),
                                                  combination = rfu.has(InventoryArrangementFlag::COMBINATION),
                                                  reorder = rfu.has(InventoryArrangementFlag::REORDER),
                                                  inventory = rfu.has(SubWindowRedrawingFlag::INVENTORY)] {
        restore_flag(rfu, InventoryArrangementFlag::AUTO_DESTRUCTION, auto_destruction);
        restore_flag(rfu, InventoryArrangementFlag::COMBINATION, combination);
        restore_flag(rfu, InventoryArrangementFlag::REORDER, reorder);
        restore_flag(rfu, SubWindowRedrawingFlag::INVENTORY, inventory);
    });
    static constexpr auto flags_iaf = {
        InventoryArrangementFlag::AUTO_DESTRUCTION,
        InventoryArrangementFlag::COMBINATION,
        InventoryArrangementFlag::REORDER,
    };
    rfu.reset_flags(flags_iaf);
    rfu.reset_flag(SubWindowRedrawingFlag::INVENTORY);

    rfu.request_inventory_arrangement();

    CHECK(rfu.has(InventoryArrangementFlag::COMBINATION));
    CHECK(rfu.has(InventoryArrangementFlag::REORDER));
    CHECK(rfu.has(SubWindowRedrawingFlag::INVENTORY));
    CHECK_FALSE(rfu.has(InventoryArrangementFlag::AUTO_DESTRUCTION));
}

TEST_CASE("RedrawingFlagsUpdater::set_item_related_sub_window_flags requests redrawing the windows showing items")
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    const auto restore = util::make_finalizer([&rfu,
                                                  inventory = rfu.has(SubWindowRedrawingFlag::INVENTORY),
                                                  equipment = rfu.has(SubWindowRedrawingFlag::EQUIPMENT),
                                                  player = rfu.has(SubWindowRedrawingFlag::PLAYER),
                                                  floor_items = rfu.has(SubWindowRedrawingFlag::FLOOR_ITEMS),
                                                  found_items = rfu.has(SubWindowRedrawingFlag::FOUND_ITEMS)] {
        restore_flag(rfu, SubWindowRedrawingFlag::INVENTORY, inventory);
        restore_flag(rfu, SubWindowRedrawingFlag::EQUIPMENT, equipment);
        restore_flag(rfu, SubWindowRedrawingFlag::PLAYER, player);
        restore_flag(rfu, SubWindowRedrawingFlag::FLOOR_ITEMS, floor_items);
        restore_flag(rfu, SubWindowRedrawingFlag::FOUND_ITEMS, found_items);
    });
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::INVENTORY,
        SubWindowRedrawingFlag::EQUIPMENT,
        SubWindowRedrawingFlag::PLAYER,
        SubWindowRedrawingFlag::FLOOR_ITEMS,
        SubWindowRedrawingFlag::FOUND_ITEMS,
    };
    rfu.reset_flags(flags_swrf);

    rfu.set_item_related_sub_window_flags();

    for (const auto flag : flags_swrf) {
        CHECK(rfu.has(flag));
    }
}

TEST_CASE("RedrawingFlagsUpdater::request_autopick_inscription requests redrawing the windows showing the inscribed items")
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    const auto restore = util::make_finalizer([&rfu,
                                                  inventory = rfu.has(SubWindowRedrawingFlag::INVENTORY),
                                                  equipment = rfu.has(SubWindowRedrawingFlag::EQUIPMENT),
                                                  floor_items = rfu.has(SubWindowRedrawingFlag::FLOOR_ITEMS),
                                                  found_items = rfu.has(SubWindowRedrawingFlag::FOUND_ITEMS),
                                                  player = rfu.has(SubWindowRedrawingFlag::PLAYER),
                                                  bonus = rfu.has(StatusRecalculatingFlag::BONUS),
                                                  combination = rfu.has(InventoryArrangementFlag::COMBINATION)] {
        restore_flag(rfu, SubWindowRedrawingFlag::INVENTORY, inventory);
        restore_flag(rfu, SubWindowRedrawingFlag::EQUIPMENT, equipment);
        restore_flag(rfu, SubWindowRedrawingFlag::FLOOR_ITEMS, floor_items);
        restore_flag(rfu, SubWindowRedrawingFlag::FOUND_ITEMS, found_items);
        restore_flag(rfu, SubWindowRedrawingFlag::PLAYER, player);
        restore_flag(rfu, StatusRecalculatingFlag::BONUS, bonus);
        restore_flag(rfu, InventoryArrangementFlag::COMBINATION, combination);
    });
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::INVENTORY,
        SubWindowRedrawingFlag::EQUIPMENT,
        SubWindowRedrawingFlag::FLOOR_ITEMS,
        SubWindowRedrawingFlag::FOUND_ITEMS,
    };
    rfu.reset_flags(flags_swrf);
    rfu.reset_flag(SubWindowRedrawingFlag::PLAYER);
    rfu.reset_flag(StatusRecalculatingFlag::BONUS);
    rfu.reset_flag(InventoryArrangementFlag::COMBINATION);

    rfu.request_autopick_inscription();

    for (const auto flag : flags_swrf) {
        CHECK(rfu.has(flag));
    }

    CHECK(rfu.has(StatusRecalculatingFlag::BONUS));
    CHECK(rfu.has(InventoryArrangementFlag::COMBINATION));
    // アイテムの一覧ではないサブウィンドウは対象外
    CHECK_FALSE(rfu.has(SubWindowRedrawingFlag::PLAYER));
}
