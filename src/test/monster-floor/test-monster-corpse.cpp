/*!
 * @file test-monster-corpse.cpp
 * @brief フロアに遺した死体のコンテナとグリッドチェインの境界条件 (Issue #9275)
 */

#include "monster-floor/monster-corpse-leaver.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/floor/monster-corpse.h"
#include "system/gamevalue.h"
#include "system/grid-type-definition.h"
#include "util/enum-converter.h"
#include <doctest/doctest.h>
#include <memory>

namespace {
/*!
 * @brief 指定座標に死体を 1 体積む (モンスターを用意せずにチェインだけを作る)
 * @return 積んだ死体の添字
 */
short push_corpse_at(FloorType &floor, const Pos2D &pos, MonraceId monrace_id)
{
    const auto corpse_idx = floor.pop_empty_index_corpse();
    if (corpse_idx == 0) {
        return 0;
    }

    auto &corpse = floor.get_corpse(corpse_idx);
    corpse.body.set_r_idx(monrace_id);
    corpse.body.y = pos.y;
    corpse.body.x = pos.x;

    auto &grid = floor.get_grid(pos);
    corpse.next_corpse_idx = grid.corpse_idx;
    grid.corpse_idx = corpse_idx;
    return corpse_idx;
}
}

TEST_CASE("Floor corpse container")
{
    auto floor_ptr = std::make_unique<FloorType>();
    auto &floor = *floor_ptr;
    floor.height = 9;
    floor.width = 9;

    SUBCASE("The container starts with a single dummy entry at index 0")
    {
        REQUIRE(floor.corpses.size() == 1);
        REQUIRE(floor.corpses[0] == nullptr);
    }

    SUBCASE("A fresh corpse has its monster profile initialised")
    {
        // これを怠るとセーブデータ読込 (rd_monster) が tl::bad_optional_access で落ちる
        const auto corpse_idx = floor.pop_empty_index_corpse();
        REQUIRE(corpse_idx == 1);
        REQUIRE(floor.get_corpse(corpse_idx).body.has_monster_profile());
    }

    SUBCASE("Indices are handed out in order and never reused")
    {
        REQUIRE(floor.pop_empty_index_corpse() == 1);
        REQUIRE(floor.pop_empty_index_corpse() == 2);
        REQUIRE(floor.pop_empty_index_corpse() == 3);
    }

    SUBCASE("A grid with no corpse reports none")
    {
        const Pos2D pos(3, 3);
        REQUIRE_FALSE(floor.get_grid(pos).has_corpse());
        REQUIRE(collect_corpses_at(floor, pos).empty());
    }

    SUBCASE("Corpses on the same grid are chained newest first")
    {
        const Pos2D pos(4, 5);
        const auto first = push_corpse_at(floor, pos, i2enum<MonraceId>(11));
        const auto second = push_corpse_at(floor, pos, i2enum<MonraceId>(22));
        const auto third = push_corpse_at(floor, pos, i2enum<MonraceId>(33));
        REQUIRE(floor.get_grid(pos).has_corpse());

        const auto collected = collect_corpses_at(floor, pos);
        REQUIRE(collected.size() == 3);
        REQUIRE(collected[0] == third);
        REQUIRE(collected[1] == second);
        REQUIRE(collected[2] == first);
    }

    SUBCASE("Corpses on different grids do not leak into each other")
    {
        const Pos2D pos1(2, 2);
        const Pos2D pos2(6, 7);
        push_corpse_at(floor, pos1, i2enum<MonraceId>(11));
        push_corpse_at(floor, pos2, i2enum<MonraceId>(22));
        REQUIRE(collect_corpses_at(floor, pos1).size() == 1);
        REQUIRE(collect_corpses_at(floor, pos2).size() == 1);
    }

    SUBCASE("Positions outside the floor are rejected instead of throwing")
    {
        REQUIRE(collect_corpses_at(floor, Pos2D(-1, -1)).empty());
        REQUIRE(collect_corpses_at(floor, Pos2D(MAX_HGT + 10, MAX_WID + 10)).empty());
    }

    SUBCASE("Wiping clears both the container and the grid chains")
    {
        const Pos2D pos(4, 4);
        push_corpse_at(floor, pos, i2enum<MonraceId>(11));
        push_corpse_at(floor, pos, i2enum<MonraceId>(22));
        REQUIRE(floor.get_grid(pos).has_corpse());

        wipe_corpses(floor);
        REQUIRE(floor.corpses.size() == 1);
        REQUIRE_FALSE(floor.get_grid(pos).has_corpse());
        REQUIRE(collect_corpses_at(floor, pos).empty());
        // 破棄後も添字を払い出せる
        REQUIRE(floor.pop_empty_index_corpse() == 1);
    }

    SUBCASE("The container stops handing out indices at the cap")
    {
        while (std::ssize(floor.corpses) < MAX_FLOOR_CORPSES) {
            REQUIRE(floor.pop_empty_index_corpse() > 0);
        }

        // 満杯時は例外を投げずに 0 を返す (呼出側が死体を諦め、品を床へ逃がす)
        REQUIRE(floor.pop_empty_index_corpse() == 0);
    }
}
