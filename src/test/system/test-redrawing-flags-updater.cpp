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

TEST_CASE("RedrawingFlagsUpdater::request_riding_status_redrawing requests redrawing the riding status and the mount's health")
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    const auto restore = util::make_finalizer([&rfu,
                                                  extra = rfu.has(MainWindowRedrawingFlag::EXTRA),
                                                  uhealth = rfu.has(MainWindowRedrawingFlag::UHEALTH),
                                                  health = rfu.has(MainWindowRedrawingFlag::HEALTH),
                                                  map = rfu.has(MainWindowRedrawingFlag::MAP),
                                                  bonus = rfu.has(StatusRecalculatingFlag::BONUS),
                                                  overhead = rfu.has(SubWindowRedrawingFlag::OVERHEAD),
                                                  dungeon = rfu.has(SubWindowRedrawingFlag::DUNGEON)] {
        restore_flag(rfu, MainWindowRedrawingFlag::EXTRA, extra);
        restore_flag(rfu, MainWindowRedrawingFlag::UHEALTH, uhealth);
        restore_flag(rfu, MainWindowRedrawingFlag::HEALTH, health);
        restore_flag(rfu, MainWindowRedrawingFlag::MAP, map);
        restore_flag(rfu, StatusRecalculatingFlag::BONUS, bonus);
        restore_flag(rfu, SubWindowRedrawingFlag::OVERHEAD, overhead);
        restore_flag(rfu, SubWindowRedrawingFlag::DUNGEON, dungeon);
    });
    rfu.reset_flag(MainWindowRedrawingFlag::EXTRA);
    rfu.reset_flag(MainWindowRedrawingFlag::UHEALTH);
    rfu.reset_flag(MainWindowRedrawingFlag::HEALTH);
    rfu.reset_flag(MainWindowRedrawingFlag::MAP);
    rfu.reset_flag(StatusRecalculatingFlag::BONUS);
    rfu.reset_flag(SubWindowRedrawingFlag::OVERHEAD);
    rfu.reset_flag(SubWindowRedrawingFlag::DUNGEON);

    rfu.request_riding_status_redrawing();

    CHECK(rfu.has(MainWindowRedrawingFlag::EXTRA));
    CHECK(rfu.has(MainWindowRedrawingFlag::UHEALTH));
    // 追跡しているモンスターの体力や、乗っているモンスターで変わる能力の再計算は、それぞれの箇所で要求する
    CHECK_FALSE(rfu.has(MainWindowRedrawingFlag::HEALTH));
    CHECK_FALSE(rfu.has(StatusRecalculatingFlag::BONUS));
    // 乗り降りのコマンドで一緒に要求するメイン画面の地図 (MAP) と、落馬で一緒に要求するサブウィンドウの地図
    // (OVERHEAD・DUNGEON) の描き直しも、それぞれの箇所で要求する
    CHECK_FALSE(rfu.has(MainWindowRedrawingFlag::MAP));
    CHECK_FALSE(rfu.has(SubWindowRedrawingFlag::OVERHEAD));
    CHECK_FALSE(rfu.has(SubWindowRedrawingFlag::DUNGEON));
}

TEST_CASE("RedrawingFlagsUpdater::request_hp_mp_redrawing requests redrawing the HP and the MP")
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    const auto restore = util::make_finalizer([&rfu,
                                                  hp = rfu.has(MainWindowRedrawingFlag::HP),
                                                  mp = rfu.has(MainWindowRedrawingFlag::MP),
                                                  basic = rfu.has(MainWindowRedrawingFlag::BASIC),
                                                  extra = rfu.has(MainWindowRedrawingFlag::EXTRA),
                                                  spell = rfu.has(SubWindowRedrawingFlag::SPELL),
                                                  item_knowledge = rfu.has(SubWindowRedrawingFlag::ITEM_KNOWLEDGE),
                                                  player = rfu.has(SubWindowRedrawingFlag::PLAYER)] {
        restore_flag(rfu, MainWindowRedrawingFlag::HP, hp);
        restore_flag(rfu, MainWindowRedrawingFlag::MP, mp);
        restore_flag(rfu, MainWindowRedrawingFlag::BASIC, basic);
        restore_flag(rfu, MainWindowRedrawingFlag::EXTRA, extra);
        restore_flag(rfu, SubWindowRedrawingFlag::SPELL, spell);
        restore_flag(rfu, SubWindowRedrawingFlag::ITEM_KNOWLEDGE, item_knowledge);
        restore_flag(rfu, SubWindowRedrawingFlag::PLAYER, player);
    });
    rfu.reset_flag(MainWindowRedrawingFlag::HP);
    rfu.reset_flag(MainWindowRedrawingFlag::MP);
    rfu.reset_flag(MainWindowRedrawingFlag::BASIC);
    rfu.reset_flag(MainWindowRedrawingFlag::EXTRA);
    rfu.reset_flag(SubWindowRedrawingFlag::SPELL);
    rfu.reset_flag(SubWindowRedrawingFlag::ITEM_KNOWLEDGE);
    rfu.reset_flag(SubWindowRedrawingFlag::PLAYER);

    rfu.request_hp_mp_redrawing();

    CHECK(rfu.has(MainWindowRedrawingFlag::HP));
    CHECK(rfu.has(MainWindowRedrawingFlag::MP));
    // 基本ステータスの全体や拡張ステータスは描き直さない
    CHECK_FALSE(rfu.has(MainWindowRedrawingFlag::BASIC));
    CHECK_FALSE(rfu.has(MainWindowRedrawingFlag::EXTRA));
    // 呪文の情報は HP・MP を描き直すときに、プレイヤーの情報は前に描いた HP・MP と比べて要求される
    CHECK_FALSE(rfu.has(SubWindowRedrawingFlag::SPELL));
    CHECK_FALSE(rfu.has(SubWindowRedrawingFlag::ITEM_KNOWLEDGE));
    CHECK_FALSE(rfu.has(SubWindowRedrawingFlag::PLAYER));
}

TEST_CASE("RedrawingFlagsUpdater::request_gold_redrawing requests redrawing the gold and the player window")
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    const auto restore = util::make_finalizer([&rfu,
                                                  gold = rfu.has(MainWindowRedrawingFlag::GOLD),
                                                  basic = rfu.has(MainWindowRedrawingFlag::BASIC),
                                                  player = rfu.has(SubWindowRedrawingFlag::PLAYER)] {
        restore_flag(rfu, MainWindowRedrawingFlag::GOLD, gold);
        restore_flag(rfu, MainWindowRedrawingFlag::BASIC, basic);
        restore_flag(rfu, SubWindowRedrawingFlag::PLAYER, player);
    });
    rfu.reset_flag(MainWindowRedrawingFlag::GOLD);
    rfu.reset_flag(MainWindowRedrawingFlag::BASIC);
    rfu.reset_flag(SubWindowRedrawingFlag::PLAYER);

    rfu.request_gold_redrawing();

    CHECK(rfu.has(MainWindowRedrawingFlag::GOLD));
    CHECK(rfu.has(SubWindowRedrawingFlag::PLAYER));
    // 所持金のほかの基本ステータスは描き直さない
    CHECK_FALSE(rfu.has(MainWindowRedrawingFlag::BASIC));
}

TEST_CASE("RedrawingFlagsUpdater::request_light_fuel_change_updates requests recalculating the light and redrawing the equipment")
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    const auto restore = util::make_finalizer([&rfu,
                                                  torch = rfu.has(StatusRecalculatingFlag::TORCH),
                                                  bonus = rfu.has(StatusRecalculatingFlag::BONUS),
                                                  equipment = rfu.has(SubWindowRedrawingFlag::EQUIPMENT),
                                                  inventory = rfu.has(SubWindowRedrawingFlag::INVENTORY)] {
        restore_flag(rfu, StatusRecalculatingFlag::TORCH, torch);
        restore_flag(rfu, StatusRecalculatingFlag::BONUS, bonus);
        restore_flag(rfu, SubWindowRedrawingFlag::EQUIPMENT, equipment);
        restore_flag(rfu, SubWindowRedrawingFlag::INVENTORY, inventory);
    });
    rfu.reset_flag(StatusRecalculatingFlag::TORCH);
    rfu.reset_flag(StatusRecalculatingFlag::BONUS);
    rfu.reset_flag(SubWindowRedrawingFlag::EQUIPMENT);
    rfu.reset_flag(SubWindowRedrawingFlag::INVENTORY);

    rfu.request_light_fuel_change_updates();

    CHECK(rfu.has(StatusRecalculatingFlag::TORCH));
    CHECK(rfu.has(SubWindowRedrawingFlag::EQUIPMENT));
    // 能力 (燃料には依らない) の再計算や、所持品の一覧の描き直しは要求しない
    CHECK_FALSE(rfu.has(StatusRecalculatingFlag::BONUS));
    CHECK_FALSE(rfu.has(SubWindowRedrawingFlag::INVENTORY));
}

TEST_CASE("RedrawingFlagsUpdater::request_bonus_and_max_hp_recalculation requests recalculating the bonuses and the max HP")
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    const auto restore = util::make_finalizer([&rfu,
                                                  bonus = rfu.has(StatusRecalculatingFlag::BONUS),
                                                  hp = rfu.has(StatusRecalculatingFlag::HP),
                                                  mp = rfu.has(StatusRecalculatingFlag::MP),
                                                  spells = rfu.has(StatusRecalculatingFlag::SPELLS)] {
        restore_flag(rfu, StatusRecalculatingFlag::BONUS, bonus);
        restore_flag(rfu, StatusRecalculatingFlag::HP, hp);
        restore_flag(rfu, StatusRecalculatingFlag::MP, mp);
        restore_flag(rfu, StatusRecalculatingFlag::SPELLS, spells);
    });
    rfu.reset_flag(StatusRecalculatingFlag::BONUS);
    rfu.reset_flag(StatusRecalculatingFlag::HP);
    rfu.reset_flag(StatusRecalculatingFlag::MP);
    rfu.reset_flag(StatusRecalculatingFlag::SPELLS);

    rfu.request_bonus_and_max_hp_recalculation();

    CHECK(rfu.has(StatusRecalculatingFlag::BONUS));
    CHECK(rfu.has(StatusRecalculatingFlag::HP));
    CHECK_FALSE(rfu.has(StatusRecalculatingFlag::MP));
    CHECK_FALSE(rfu.has(StatusRecalculatingFlag::SPELLS));
}

TEST_CASE("RedrawingFlagsUpdater::request_max_mp_and_spells_recalculation requests recalculating the max MP and the spells")
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    const auto restore = util::make_finalizer([&rfu,
                                                  bonus = rfu.has(StatusRecalculatingFlag::BONUS),
                                                  hp = rfu.has(StatusRecalculatingFlag::HP),
                                                  mp = rfu.has(StatusRecalculatingFlag::MP),
                                                  spells = rfu.has(StatusRecalculatingFlag::SPELLS)] {
        restore_flag(rfu, StatusRecalculatingFlag::BONUS, bonus);
        restore_flag(rfu, StatusRecalculatingFlag::HP, hp);
        restore_flag(rfu, StatusRecalculatingFlag::MP, mp);
        restore_flag(rfu, StatusRecalculatingFlag::SPELLS, spells);
    });
    rfu.reset_flag(StatusRecalculatingFlag::BONUS);
    rfu.reset_flag(StatusRecalculatingFlag::HP);
    rfu.reset_flag(StatusRecalculatingFlag::MP);
    rfu.reset_flag(StatusRecalculatingFlag::SPELLS);

    rfu.request_max_mp_and_spells_recalculation();

    CHECK_FALSE(rfu.has(StatusRecalculatingFlag::BONUS));
    CHECK_FALSE(rfu.has(StatusRecalculatingFlag::HP));
    CHECK(rfu.has(StatusRecalculatingFlag::MP));
    CHECK(rfu.has(StatusRecalculatingFlag::SPELLS));
}

TEST_CASE("RedrawingFlagsUpdater::request_player_status_recalculation requests recalculating the bonuses, the max HP, the max MP and the spells")
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    const auto restore = util::make_finalizer([&rfu,
                                                  bonus = rfu.has(StatusRecalculatingFlag::BONUS),
                                                  hp = rfu.has(StatusRecalculatingFlag::HP),
                                                  mp = rfu.has(StatusRecalculatingFlag::MP),
                                                  spells = rfu.has(StatusRecalculatingFlag::SPELLS),
                                                  torch = rfu.has(StatusRecalculatingFlag::TORCH)] {
        restore_flag(rfu, StatusRecalculatingFlag::BONUS, bonus);
        restore_flag(rfu, StatusRecalculatingFlag::HP, hp);
        restore_flag(rfu, StatusRecalculatingFlag::MP, mp);
        restore_flag(rfu, StatusRecalculatingFlag::SPELLS, spells);
        restore_flag(rfu, StatusRecalculatingFlag::TORCH, torch);
    });
    rfu.reset_flag(StatusRecalculatingFlag::BONUS);
    rfu.reset_flag(StatusRecalculatingFlag::HP);
    rfu.reset_flag(StatusRecalculatingFlag::MP);
    rfu.reset_flag(StatusRecalculatingFlag::SPELLS);
    rfu.reset_flag(StatusRecalculatingFlag::TORCH);

    rfu.request_player_status_recalculation();

    CHECK(rfu.has(StatusRecalculatingFlag::BONUS));
    CHECK(rfu.has(StatusRecalculatingFlag::HP));
    CHECK(rfu.has(StatusRecalculatingFlag::MP));
    CHECK(rfu.has(StatusRecalculatingFlag::SPELLS));
    // 光源の明るさは含めない
    CHECK_FALSE(rfu.has(StatusRecalculatingFlag::TORCH));
}
