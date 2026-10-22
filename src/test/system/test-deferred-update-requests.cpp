/*!
 * @brief DeferredUpdateRequests のテスト
 */

#include "system/deferred-update-requests.h"
#include "util/finalizer.h"
#include <doctest/doctest.h>

namespace {

/*!
 * @brief フラグを元の状態に戻す
 * @param dur 更新フラグ
 * @param flag 戻すフラグ
 * @param was_set 元の状態でフラグが立っていたか
 */
template <typename Flag>
void restore_flag(DeferredUpdateRequests &dur, Flag flag, bool was_set)
{
    if (was_set) {
        dur.set_flag(flag);
    } else {
        dur.reset_flag(flag);
    }
}

}

TEST_CASE("DeferredUpdateRequests::request_inventory_arrangement requests combining, reordering and redrawing the inventory")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    const auto restore = util::make_finalizer([&dur,
                                                  auto_destruction = dur.has(InventoryArrangementFlag::AUTO_DESTRUCTION),
                                                  combination = dur.has(InventoryArrangementFlag::COMBINATION),
                                                  reorder = dur.has(InventoryArrangementFlag::REORDER),
                                                  inventory = dur.has(SubWindowRedrawingFlag::INVENTORY)] {
        restore_flag(dur, InventoryArrangementFlag::AUTO_DESTRUCTION, auto_destruction);
        restore_flag(dur, InventoryArrangementFlag::COMBINATION, combination);
        restore_flag(dur, InventoryArrangementFlag::REORDER, reorder);
        restore_flag(dur, SubWindowRedrawingFlag::INVENTORY, inventory);
    });
    static constexpr auto flags_iaf = {
        InventoryArrangementFlag::AUTO_DESTRUCTION,
        InventoryArrangementFlag::COMBINATION,
        InventoryArrangementFlag::REORDER,
    };
    dur.reset_flags(flags_iaf);
    dur.reset_flag(SubWindowRedrawingFlag::INVENTORY);

    dur.request_inventory_arrangement();

    CHECK(dur.has(InventoryArrangementFlag::COMBINATION));
    CHECK(dur.has(InventoryArrangementFlag::REORDER));
    CHECK(dur.has(SubWindowRedrawingFlag::INVENTORY));
    CHECK_FALSE(dur.has(InventoryArrangementFlag::AUTO_DESTRUCTION));
}

TEST_CASE("DeferredUpdateRequests::set_item_related_sub_window_flags requests redrawing the windows showing items")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    const auto restore = util::make_finalizer([&dur,
                                                  inventory = dur.has(SubWindowRedrawingFlag::INVENTORY),
                                                  equipment = dur.has(SubWindowRedrawingFlag::EQUIPMENT),
                                                  player = dur.has(SubWindowRedrawingFlag::PLAYER),
                                                  floor_items = dur.has(SubWindowRedrawingFlag::FLOOR_ITEMS),
                                                  found_items = dur.has(SubWindowRedrawingFlag::FOUND_ITEMS)] {
        restore_flag(dur, SubWindowRedrawingFlag::INVENTORY, inventory);
        restore_flag(dur, SubWindowRedrawingFlag::EQUIPMENT, equipment);
        restore_flag(dur, SubWindowRedrawingFlag::PLAYER, player);
        restore_flag(dur, SubWindowRedrawingFlag::FLOOR_ITEMS, floor_items);
        restore_flag(dur, SubWindowRedrawingFlag::FOUND_ITEMS, found_items);
    });
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::INVENTORY,
        SubWindowRedrawingFlag::EQUIPMENT,
        SubWindowRedrawingFlag::PLAYER,
        SubWindowRedrawingFlag::FLOOR_ITEMS,
        SubWindowRedrawingFlag::FOUND_ITEMS,
    };
    dur.reset_flags(flags_swrf);

    dur.set_item_related_sub_window_flags();

    for (const auto flag : flags_swrf) {
        CHECK(dur.has(flag));
    }
}

TEST_CASE("DeferredUpdateRequests::request_floor_item_lists_redrawing requests redrawing the windows showing floor items")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    const auto restore = util::make_finalizer([&dur,
                                                  floor_items = dur.has(SubWindowRedrawingFlag::FLOOR_ITEMS),
                                                  found_items = dur.has(SubWindowRedrawingFlag::FOUND_ITEMS)] {
        restore_flag(dur, SubWindowRedrawingFlag::FLOOR_ITEMS, floor_items);
        restore_flag(dur, SubWindowRedrawingFlag::FOUND_ITEMS, found_items);
    });
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::FLOOR_ITEMS,
        SubWindowRedrawingFlag::FOUND_ITEMS,
    };
    dur.reset_flags(flags_swrf);

    dur.request_floor_item_lists_redrawing();

    for (const auto flag : flags_swrf) {
        CHECK(dur.has(flag));
    }
}

TEST_CASE("DeferredUpdateRequests::request_inscription_change_updates requests recalculating the bonuses and combining the inventory")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    const auto restore = util::make_finalizer([&dur,
                                                  bonus = dur.has(StatusRecalculatingFlag::BONUS),
                                                  combination = dur.has(InventoryArrangementFlag::COMBINATION),
                                                  reorder = dur.has(InventoryArrangementFlag::REORDER)] {
        restore_flag(dur, StatusRecalculatingFlag::BONUS, bonus);
        restore_flag(dur, InventoryArrangementFlag::COMBINATION, combination);
        restore_flag(dur, InventoryArrangementFlag::REORDER, reorder);
    });
    dur.reset_flag(StatusRecalculatingFlag::BONUS);
    dur.reset_flag(InventoryArrangementFlag::COMBINATION);
    dur.reset_flag(InventoryArrangementFlag::REORDER);

    dur.request_inscription_change_updates();

    CHECK(dur.has(StatusRecalculatingFlag::BONUS));
    CHECK(dur.has(InventoryArrangementFlag::COMBINATION));
    CHECK_FALSE(dur.has(InventoryArrangementFlag::REORDER));
}

TEST_CASE("DeferredUpdateRequests::request_equipment_change_updates requests recalculating the bonuses, the light radius and the max MP")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    const auto restore = util::make_finalizer([&dur,
                                                  bonus = dur.has(StatusRecalculatingFlag::BONUS),
                                                  torch = dur.has(StatusRecalculatingFlag::TORCH),
                                                  mp = dur.has(StatusRecalculatingFlag::MP),
                                                  hp = dur.has(StatusRecalculatingFlag::HP)] {
        restore_flag(dur, StatusRecalculatingFlag::BONUS, bonus);
        restore_flag(dur, StatusRecalculatingFlag::TORCH, torch);
        restore_flag(dur, StatusRecalculatingFlag::MP, mp);
        restore_flag(dur, StatusRecalculatingFlag::HP, hp);
    });
    dur.reset_flag(StatusRecalculatingFlag::BONUS);
    dur.reset_flag(StatusRecalculatingFlag::TORCH);
    dur.reset_flag(StatusRecalculatingFlag::MP);
    dur.reset_flag(StatusRecalculatingFlag::HP);

    dur.request_equipment_change_updates();

    CHECK(dur.has(StatusRecalculatingFlag::BONUS));
    CHECK(dur.has(StatusRecalculatingFlag::TORCH));
    CHECK(dur.has(StatusRecalculatingFlag::MP));
    CHECK_FALSE(dur.has(StatusRecalculatingFlag::HP));
}

TEST_CASE("DeferredUpdateRequests::request_whole_status_recalculation requests recalculating the status, the view and the lights")
{
    auto &dur = DeferredUpdateRequests::get_instance();
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
    const auto restore = util::make_finalizer([&dur,
                                                  flow = dur.has(StatusRecalculatingFlag::FLOW),
                                                  distance = dur.has(StatusRecalculatingFlag::DISTANCE)] {
        restore_flag(dur, StatusRecalculatingFlag::FLOW, flow);
        restore_flag(dur, StatusRecalculatingFlag::DISTANCE, distance);
    });
    dur.reset_flags(flags_srf);
    dur.reset_flag(StatusRecalculatingFlag::FLOW);
    dur.reset_flag(StatusRecalculatingFlag::DISTANCE);

    dur.request_whole_status_recalculation();

    for (const auto flag : flags_srf) {
        CHECK(dur.has(flag));
    }

    CHECK_FALSE(dur.has(StatusRecalculatingFlag::FLOW));
    CHECK_FALSE(dur.has(StatusRecalculatingFlag::DISTANCE));
}

TEST_CASE("DeferredUpdateRequests::request_sight_recalculation requests forgetting and recalculating the view and the lights")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::UN_VIEW,
        StatusRecalculatingFlag::UN_LITE,
        StatusRecalculatingFlag::VIEW,
        StatusRecalculatingFlag::LITE,
        StatusRecalculatingFlag::MONSTER_LITE,
    };
    const auto restore = util::make_finalizer([&dur,
                                                  un_view = dur.has(StatusRecalculatingFlag::UN_VIEW),
                                                  un_lite = dur.has(StatusRecalculatingFlag::UN_LITE),
                                                  view = dur.has(StatusRecalculatingFlag::VIEW),
                                                  lite = dur.has(StatusRecalculatingFlag::LITE),
                                                  monster_lite = dur.has(StatusRecalculatingFlag::MONSTER_LITE),
                                                  flow = dur.has(StatusRecalculatingFlag::FLOW)] {
        restore_flag(dur, StatusRecalculatingFlag::UN_VIEW, un_view);
        restore_flag(dur, StatusRecalculatingFlag::UN_LITE, un_lite);
        restore_flag(dur, StatusRecalculatingFlag::VIEW, view);
        restore_flag(dur, StatusRecalculatingFlag::LITE, lite);
        restore_flag(dur, StatusRecalculatingFlag::MONSTER_LITE, monster_lite);
        restore_flag(dur, StatusRecalculatingFlag::FLOW, flow);
    });
    dur.reset_flags(flags_srf);
    dur.reset_flag(StatusRecalculatingFlag::FLOW);

    dur.request_sight_recalculation();

    for (const auto flag : flags_srf) {
        CHECK(dur.has(flag));
    }

    CHECK_FALSE(dur.has(StatusRecalculatingFlag::FLOW));
}

TEST_CASE("DeferredUpdateRequests::request_riding_status_redrawing requests redrawing the riding status and the mount's health")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    const auto restore = util::make_finalizer([&dur,
                                                  extra = dur.has(MainWindowRedrawingFlag::EXTRA),
                                                  uhealth = dur.has(MainWindowRedrawingFlag::UHEALTH),
                                                  health = dur.has(MainWindowRedrawingFlag::HEALTH),
                                                  map = dur.has(MainWindowRedrawingFlag::MAP),
                                                  bonus = dur.has(StatusRecalculatingFlag::BONUS),
                                                  overhead = dur.has(SubWindowRedrawingFlag::OVERHEAD),
                                                  dungeon = dur.has(SubWindowRedrawingFlag::DUNGEON)] {
        restore_flag(dur, MainWindowRedrawingFlag::EXTRA, extra);
        restore_flag(dur, MainWindowRedrawingFlag::UHEALTH, uhealth);
        restore_flag(dur, MainWindowRedrawingFlag::HEALTH, health);
        restore_flag(dur, MainWindowRedrawingFlag::MAP, map);
        restore_flag(dur, StatusRecalculatingFlag::BONUS, bonus);
        restore_flag(dur, SubWindowRedrawingFlag::OVERHEAD, overhead);
        restore_flag(dur, SubWindowRedrawingFlag::DUNGEON, dungeon);
    });
    dur.reset_flag(MainWindowRedrawingFlag::EXTRA);
    dur.reset_flag(MainWindowRedrawingFlag::UHEALTH);
    dur.reset_flag(MainWindowRedrawingFlag::HEALTH);
    dur.reset_flag(MainWindowRedrawingFlag::MAP);
    dur.reset_flag(StatusRecalculatingFlag::BONUS);
    dur.reset_flag(SubWindowRedrawingFlag::OVERHEAD);
    dur.reset_flag(SubWindowRedrawingFlag::DUNGEON);

    dur.request_riding_status_redrawing();

    CHECK(dur.has(MainWindowRedrawingFlag::EXTRA));
    CHECK(dur.has(MainWindowRedrawingFlag::UHEALTH));
    // 追跡しているモンスターの体力や、乗っているモンスターで変わる能力の再計算は、それぞれの箇所で要求する
    CHECK_FALSE(dur.has(MainWindowRedrawingFlag::HEALTH));
    CHECK_FALSE(dur.has(StatusRecalculatingFlag::BONUS));
    // 乗り降りのコマンドで一緒に要求するメイン画面の地図 (MAP) と、落馬で一緒に要求するサブウィンドウの地図
    // (OVERHEAD・DUNGEON) の描き直しも、それぞれの箇所で要求する
    CHECK_FALSE(dur.has(MainWindowRedrawingFlag::MAP));
    CHECK_FALSE(dur.has(SubWindowRedrawingFlag::OVERHEAD));
    CHECK_FALSE(dur.has(SubWindowRedrawingFlag::DUNGEON));
}

TEST_CASE("DeferredUpdateRequests::request_hp_mp_redrawing requests redrawing the HP and the MP")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    const auto restore = util::make_finalizer([&dur,
                                                  hp = dur.has(MainWindowRedrawingFlag::HP),
                                                  mp = dur.has(MainWindowRedrawingFlag::MP),
                                                  basic = dur.has(MainWindowRedrawingFlag::BASIC),
                                                  extra = dur.has(MainWindowRedrawingFlag::EXTRA),
                                                  spell = dur.has(SubWindowRedrawingFlag::SPELL),
                                                  item_knowledge = dur.has(SubWindowRedrawingFlag::ITEM_KNOWLEDGE),
                                                  player = dur.has(SubWindowRedrawingFlag::PLAYER)] {
        restore_flag(dur, MainWindowRedrawingFlag::HP, hp);
        restore_flag(dur, MainWindowRedrawingFlag::MP, mp);
        restore_flag(dur, MainWindowRedrawingFlag::BASIC, basic);
        restore_flag(dur, MainWindowRedrawingFlag::EXTRA, extra);
        restore_flag(dur, SubWindowRedrawingFlag::SPELL, spell);
        restore_flag(dur, SubWindowRedrawingFlag::ITEM_KNOWLEDGE, item_knowledge);
        restore_flag(dur, SubWindowRedrawingFlag::PLAYER, player);
    });
    dur.reset_flag(MainWindowRedrawingFlag::HP);
    dur.reset_flag(MainWindowRedrawingFlag::MP);
    dur.reset_flag(MainWindowRedrawingFlag::BASIC);
    dur.reset_flag(MainWindowRedrawingFlag::EXTRA);
    dur.reset_flag(SubWindowRedrawingFlag::SPELL);
    dur.reset_flag(SubWindowRedrawingFlag::ITEM_KNOWLEDGE);
    dur.reset_flag(SubWindowRedrawingFlag::PLAYER);

    dur.request_hp_mp_redrawing();

    CHECK(dur.has(MainWindowRedrawingFlag::HP));
    CHECK(dur.has(MainWindowRedrawingFlag::MP));
    // 基本ステータスの全体や拡張ステータスは描き直さない
    CHECK_FALSE(dur.has(MainWindowRedrawingFlag::BASIC));
    CHECK_FALSE(dur.has(MainWindowRedrawingFlag::EXTRA));
    // 呪文の情報は HP・MP を描き直すときに、プレイヤーの情報は前に描いた HP・MP と比べて要求される
    CHECK_FALSE(dur.has(SubWindowRedrawingFlag::SPELL));
    CHECK_FALSE(dur.has(SubWindowRedrawingFlag::ITEM_KNOWLEDGE));
    CHECK_FALSE(dur.has(SubWindowRedrawingFlag::PLAYER));
}

TEST_CASE("DeferredUpdateRequests::request_gold_redrawing requests redrawing the gold and the player window")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    const auto restore = util::make_finalizer([&dur,
                                                  gold = dur.has(MainWindowRedrawingFlag::GOLD),
                                                  basic = dur.has(MainWindowRedrawingFlag::BASIC),
                                                  player = dur.has(SubWindowRedrawingFlag::PLAYER)] {
        restore_flag(dur, MainWindowRedrawingFlag::GOLD, gold);
        restore_flag(dur, MainWindowRedrawingFlag::BASIC, basic);
        restore_flag(dur, SubWindowRedrawingFlag::PLAYER, player);
    });
    dur.reset_flag(MainWindowRedrawingFlag::GOLD);
    dur.reset_flag(MainWindowRedrawingFlag::BASIC);
    dur.reset_flag(SubWindowRedrawingFlag::PLAYER);

    dur.request_gold_redrawing();

    CHECK(dur.has(MainWindowRedrawingFlag::GOLD));
    CHECK(dur.has(SubWindowRedrawingFlag::PLAYER));
    // 所持金のほかの基本ステータスは描き直さない
    CHECK_FALSE(dur.has(MainWindowRedrawingFlag::BASIC));
}

TEST_CASE("DeferredUpdateRequests::request_light_fuel_change_updates requests recalculating the light and redrawing the equipment")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    const auto restore = util::make_finalizer([&dur,
                                                  torch = dur.has(StatusRecalculatingFlag::TORCH),
                                                  bonus = dur.has(StatusRecalculatingFlag::BONUS),
                                                  equipment = dur.has(SubWindowRedrawingFlag::EQUIPMENT),
                                                  inventory = dur.has(SubWindowRedrawingFlag::INVENTORY)] {
        restore_flag(dur, StatusRecalculatingFlag::TORCH, torch);
        restore_flag(dur, StatusRecalculatingFlag::BONUS, bonus);
        restore_flag(dur, SubWindowRedrawingFlag::EQUIPMENT, equipment);
        restore_flag(dur, SubWindowRedrawingFlag::INVENTORY, inventory);
    });
    dur.reset_flag(StatusRecalculatingFlag::TORCH);
    dur.reset_flag(StatusRecalculatingFlag::BONUS);
    dur.reset_flag(SubWindowRedrawingFlag::EQUIPMENT);
    dur.reset_flag(SubWindowRedrawingFlag::INVENTORY);

    dur.request_light_fuel_change_updates();

    CHECK(dur.has(StatusRecalculatingFlag::TORCH));
    CHECK(dur.has(SubWindowRedrawingFlag::EQUIPMENT));
    // 能力 (燃料には依らない) の再計算や、所持品の一覧の描き直しは要求しない
    CHECK_FALSE(dur.has(StatusRecalculatingFlag::BONUS));
    CHECK_FALSE(dur.has(SubWindowRedrawingFlag::INVENTORY));
}

TEST_CASE("DeferredUpdateRequests::request_bonus_and_max_hp_recalculation requests recalculating the bonuses and the max HP")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    const auto restore = util::make_finalizer([&dur,
                                                  bonus = dur.has(StatusRecalculatingFlag::BONUS),
                                                  hp = dur.has(StatusRecalculatingFlag::HP),
                                                  mp = dur.has(StatusRecalculatingFlag::MP),
                                                  spells = dur.has(StatusRecalculatingFlag::SPELLS)] {
        restore_flag(dur, StatusRecalculatingFlag::BONUS, bonus);
        restore_flag(dur, StatusRecalculatingFlag::HP, hp);
        restore_flag(dur, StatusRecalculatingFlag::MP, mp);
        restore_flag(dur, StatusRecalculatingFlag::SPELLS, spells);
    });
    dur.reset_flag(StatusRecalculatingFlag::BONUS);
    dur.reset_flag(StatusRecalculatingFlag::HP);
    dur.reset_flag(StatusRecalculatingFlag::MP);
    dur.reset_flag(StatusRecalculatingFlag::SPELLS);

    dur.request_bonus_and_max_hp_recalculation();

    CHECK(dur.has(StatusRecalculatingFlag::BONUS));
    CHECK(dur.has(StatusRecalculatingFlag::HP));
    CHECK_FALSE(dur.has(StatusRecalculatingFlag::MP));
    CHECK_FALSE(dur.has(StatusRecalculatingFlag::SPELLS));
}

TEST_CASE("DeferredUpdateRequests::request_max_mp_and_spells_recalculation requests recalculating the max MP and the spells")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    const auto restore = util::make_finalizer([&dur,
                                                  bonus = dur.has(StatusRecalculatingFlag::BONUS),
                                                  hp = dur.has(StatusRecalculatingFlag::HP),
                                                  mp = dur.has(StatusRecalculatingFlag::MP),
                                                  spells = dur.has(StatusRecalculatingFlag::SPELLS)] {
        restore_flag(dur, StatusRecalculatingFlag::BONUS, bonus);
        restore_flag(dur, StatusRecalculatingFlag::HP, hp);
        restore_flag(dur, StatusRecalculatingFlag::MP, mp);
        restore_flag(dur, StatusRecalculatingFlag::SPELLS, spells);
    });
    dur.reset_flag(StatusRecalculatingFlag::BONUS);
    dur.reset_flag(StatusRecalculatingFlag::HP);
    dur.reset_flag(StatusRecalculatingFlag::MP);
    dur.reset_flag(StatusRecalculatingFlag::SPELLS);

    dur.request_max_mp_and_spells_recalculation();

    CHECK_FALSE(dur.has(StatusRecalculatingFlag::BONUS));
    CHECK_FALSE(dur.has(StatusRecalculatingFlag::HP));
    CHECK(dur.has(StatusRecalculatingFlag::MP));
    CHECK(dur.has(StatusRecalculatingFlag::SPELLS));
}

TEST_CASE("DeferredUpdateRequests::request_player_status_recalculation requests recalculating the bonuses, the max HP, the max MP and the spells")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    const auto restore = util::make_finalizer([&dur,
                                                  bonus = dur.has(StatusRecalculatingFlag::BONUS),
                                                  hp = dur.has(StatusRecalculatingFlag::HP),
                                                  mp = dur.has(StatusRecalculatingFlag::MP),
                                                  spells = dur.has(StatusRecalculatingFlag::SPELLS),
                                                  torch = dur.has(StatusRecalculatingFlag::TORCH)] {
        restore_flag(dur, StatusRecalculatingFlag::BONUS, bonus);
        restore_flag(dur, StatusRecalculatingFlag::HP, hp);
        restore_flag(dur, StatusRecalculatingFlag::MP, mp);
        restore_flag(dur, StatusRecalculatingFlag::SPELLS, spells);
        restore_flag(dur, StatusRecalculatingFlag::TORCH, torch);
    });
    dur.reset_flag(StatusRecalculatingFlag::BONUS);
    dur.reset_flag(StatusRecalculatingFlag::HP);
    dur.reset_flag(StatusRecalculatingFlag::MP);
    dur.reset_flag(StatusRecalculatingFlag::SPELLS);
    dur.reset_flag(StatusRecalculatingFlag::TORCH);

    dur.request_player_status_recalculation();

    CHECK(dur.has(StatusRecalculatingFlag::BONUS));
    CHECK(dur.has(StatusRecalculatingFlag::HP));
    CHECK(dur.has(StatusRecalculatingFlag::MP));
    CHECK(dur.has(StatusRecalculatingFlag::SPELLS));
    // 光源の明るさは含めない
    CHECK_FALSE(dur.has(StatusRecalculatingFlag::TORCH));
}
