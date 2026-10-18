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

TEST_CASE("RedrawingFlagsUpdater::request_floor_item_lists_redrawing requests redrawing the windows showing floor items")
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    const auto restore = util::make_finalizer([&rfu,
                                                  floor_items = rfu.has(SubWindowRedrawingFlag::FLOOR_ITEMS),
                                                  found_items = rfu.has(SubWindowRedrawingFlag::FOUND_ITEMS)] {
        restore_flag(rfu, SubWindowRedrawingFlag::FLOOR_ITEMS, floor_items);
        restore_flag(rfu, SubWindowRedrawingFlag::FOUND_ITEMS, found_items);
    });
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::FLOOR_ITEMS,
        SubWindowRedrawingFlag::FOUND_ITEMS,
    };
    rfu.reset_flags(flags_swrf);

    rfu.request_floor_item_lists_redrawing();

    for (const auto flag : flags_swrf) {
        CHECK(rfu.has(flag));
    }
}

TEST_CASE("RedrawingFlagsUpdater::request_inscription_change_updates requests recalculating the bonuses and combining the inventory")
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    const auto restore = util::make_finalizer([&rfu,
                                                  bonus = rfu.has(StatusRecalculatingFlag::BONUS),
                                                  combination = rfu.has(InventoryArrangementFlag::COMBINATION),
                                                  reorder = rfu.has(InventoryArrangementFlag::REORDER)] {
        restore_flag(rfu, StatusRecalculatingFlag::BONUS, bonus);
        restore_flag(rfu, InventoryArrangementFlag::COMBINATION, combination);
        restore_flag(rfu, InventoryArrangementFlag::REORDER, reorder);
    });
    rfu.reset_flag(StatusRecalculatingFlag::BONUS);
    rfu.reset_flag(InventoryArrangementFlag::COMBINATION);
    rfu.reset_flag(InventoryArrangementFlag::REORDER);

    rfu.request_inscription_change_updates();

    CHECK(rfu.has(StatusRecalculatingFlag::BONUS));
    CHECK(rfu.has(InventoryArrangementFlag::COMBINATION));
    CHECK_FALSE(rfu.has(InventoryArrangementFlag::REORDER));
}

TEST_CASE("RedrawingFlagsUpdater::request_equipment_change_updates requests recalculating the bonuses, the light radius and the max MP")
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    const auto restore = util::make_finalizer([&rfu,
                                                  bonus = rfu.has(StatusRecalculatingFlag::BONUS),
                                                  torch = rfu.has(StatusRecalculatingFlag::TORCH),
                                                  mp = rfu.has(StatusRecalculatingFlag::MP),
                                                  hp = rfu.has(StatusRecalculatingFlag::HP)] {
        restore_flag(rfu, StatusRecalculatingFlag::BONUS, bonus);
        restore_flag(rfu, StatusRecalculatingFlag::TORCH, torch);
        restore_flag(rfu, StatusRecalculatingFlag::MP, mp);
        restore_flag(rfu, StatusRecalculatingFlag::HP, hp);
    });
    rfu.reset_flag(StatusRecalculatingFlag::BONUS);
    rfu.reset_flag(StatusRecalculatingFlag::TORCH);
    rfu.reset_flag(StatusRecalculatingFlag::MP);
    rfu.reset_flag(StatusRecalculatingFlag::HP);

    rfu.request_equipment_change_updates();

    CHECK(rfu.has(StatusRecalculatingFlag::BONUS));
    CHECK(rfu.has(StatusRecalculatingFlag::TORCH));
    CHECK(rfu.has(StatusRecalculatingFlag::MP));
    CHECK_FALSE(rfu.has(StatusRecalculatingFlag::HP));
}

TEST_CASE("RedrawingFlagsUpdater::request_whole_status_recalculation requests recalculating the status, the view and the lights")
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::TORCH,
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::HP,
        StatusRecalculatingFlag::MP,
        StatusRecalculatingFlag::SPELLS,
        StatusRecalculatingFlag::UN_VIEW,
        StatusRecalculatingFlag::UN_LITE,
        StatusRecalculatingFlag::VIEW,
        StatusRecalculatingFlag::LITE,
        StatusRecalculatingFlag::MONSTER_LITE,
        StatusRecalculatingFlag::MONSTER_STATUSES,
    };
    const auto restore = util::make_finalizer([&rfu,
                                                  flow = rfu.has(StatusRecalculatingFlag::FLOW),
                                                  distance = rfu.has(StatusRecalculatingFlag::DISTANCE)] {
        restore_flag(rfu, StatusRecalculatingFlag::FLOW, flow);
        restore_flag(rfu, StatusRecalculatingFlag::DISTANCE, distance);
    });
    rfu.reset_flags(flags_srf);
    rfu.reset_flag(StatusRecalculatingFlag::FLOW);
    rfu.reset_flag(StatusRecalculatingFlag::DISTANCE);

    rfu.request_whole_status_recalculation();

    for (const auto flag : flags_srf) {
        CHECK(rfu.has(flag));
    }

    CHECK_FALSE(rfu.has(StatusRecalculatingFlag::FLOW));
    CHECK_FALSE(rfu.has(StatusRecalculatingFlag::DISTANCE));
}

TEST_CASE("RedrawingFlagsUpdater::request_sight_recalculation requests forgetting and recalculating the view and the lights")
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::UN_VIEW,
        StatusRecalculatingFlag::UN_LITE,
        StatusRecalculatingFlag::VIEW,
        StatusRecalculatingFlag::LITE,
        StatusRecalculatingFlag::MONSTER_LITE,
    };
    const auto restore = util::make_finalizer([&rfu,
                                                  un_view = rfu.has(StatusRecalculatingFlag::UN_VIEW),
                                                  un_lite = rfu.has(StatusRecalculatingFlag::UN_LITE),
                                                  view = rfu.has(StatusRecalculatingFlag::VIEW),
                                                  lite = rfu.has(StatusRecalculatingFlag::LITE),
                                                  monster_lite = rfu.has(StatusRecalculatingFlag::MONSTER_LITE),
                                                  flow = rfu.has(StatusRecalculatingFlag::FLOW)] {
        restore_flag(rfu, StatusRecalculatingFlag::UN_VIEW, un_view);
        restore_flag(rfu, StatusRecalculatingFlag::UN_LITE, un_lite);
        restore_flag(rfu, StatusRecalculatingFlag::VIEW, view);
        restore_flag(rfu, StatusRecalculatingFlag::LITE, lite);
        restore_flag(rfu, StatusRecalculatingFlag::MONSTER_LITE, monster_lite);
        restore_flag(rfu, StatusRecalculatingFlag::FLOW, flow);
    });
    rfu.reset_flags(flags_srf);
    rfu.reset_flag(StatusRecalculatingFlag::FLOW);

    rfu.request_sight_recalculation();

    for (const auto flag : flags_srf) {
        CHECK(rfu.has(flag));
    }

    CHECK_FALSE(rfu.has(StatusRecalculatingFlag::FLOW));
}
