/*!
 * @file test-wormhole.cpp
 * @brief ワームホール地形の対の導出と面積比例の生成率
 */

#include "floor/floor-base-definitions.h"
#include "floor/wormhole.h"
#include <doctest/doctest.h>
#include <set>

TEST_CASE("calc_wormhole_partner_index: 前半と後半を相互に結ぶ")
{
    SUBCASE("1 対なら互いを指す")
    {
        CHECK(calc_wormhole_partner_index(2, 0) == 1);
        CHECK(calc_wormhole_partner_index(2, 1) == 0);
    }

    SUBCASE("2 対なら北半分と南半分が結ばれる")
    {
        CHECK(calc_wormhole_partner_index(4, 0) == 2);
        CHECK(calc_wormhole_partner_index(4, 1) == 3);
        CHECK(calc_wormhole_partner_index(4, 2) == 0);
        CHECK(calc_wormhole_partner_index(4, 3) == 1);
    }

    SUBCASE("対応は常に相互 (A の相手の相手は A)")
    {
        for (const auto count : { 2, 4, 6, 8, 20 }) {
            for (auto i = 0; i < count; i++) {
                const auto partner = calc_wormhole_partner_index(count, i);
                REQUIRE(partner.has_value());
                CHECK(*partner != i);
                CHECK(calc_wormhole_partner_index(count, *partner) == i);
            }
        }
    }

    SUBCASE("偶数個なら全員が相手を持ち、相手は一意に割り当てられる")
    {
        constexpr auto count = 8;
        std::set<int> partners;
        for (auto i = 0; i < count; i++) {
            const auto partner = calc_wormhole_partner_index(count, i);
            REQUIRE(partner.has_value());
            partners.insert(*partner);
        }

        CHECK(partners.size() == count);
    }

    SUBCASE("奇数個なら座標順で最後の 1 個だけが相手を持たない")
    {
        // 生成は必ず対単位で行うので通常は起きないが、起きても落ちないこと
        CHECK(calc_wormhole_partner_index(3, 0) == 1);
        CHECK(calc_wormhole_partner_index(3, 1) == 0);
        CHECK_FALSE(calc_wormhole_partner_index(3, 2).has_value());
        CHECK(calc_wormhole_partner_index(5, 4) == tl::nullopt);
    }

    SUBCASE("1 個以下 / 範囲外の添字は相手を持たない")
    {
        CHECK_FALSE(calc_wormhole_partner_index(1, 0).has_value());
        CHECK_FALSE(calc_wormhole_partner_index(0, 0).has_value());
        CHECK_FALSE(calc_wormhole_partner_index(4, 4).has_value());
        CHECK_FALSE(calc_wormhole_partner_index(4, -1).has_value());
    }
}

TEST_CASE("calc_wormhole_appearance_chance: 面積に比例し広いフロアほど出やすい")
{
    constexpr auto max_h = MAX_DUNGEON_HGT;
    constexpr auto max_w = MAX_DUNGEON_WID;

    SUBCASE("最大面積で上限の確率になる")
    {
        CHECK(calc_wormhole_appearance_chance(max_h, max_w) == WORMHOLE_MAX_AREA_CHANCE_PERCENT);
    }

    SUBCASE("面積について単調非減少")
    {
        auto previous = -1;
        for (auto side = 3; side <= max_h; side += 3) {
            const auto chance = calc_wormhole_appearance_chance(side, side);
            CHECK(chance >= previous);
            previous = chance;
        }
    }

    SUBCASE("狭いフロアは明確に出にくい")
    {
        // 最小フロア (45x45) は最大フロアの 1/4 以下の確率
        const auto smallest = calc_wormhole_appearance_chance(45, 45);
        const auto largest = calc_wormhole_appearance_chance(max_h, max_w);
        CHECK(smallest > 0);
        CHECK(smallest * 4 < largest);
    }

    SUBCASE("常に百分率の範囲に収まる")
    {
        CHECK(calc_wormhole_appearance_chance(0, 0) == 0);
        CHECK(calc_wormhole_appearance_chance(-10, -10) == 0);
        CHECK(calc_wormhole_appearance_chance(MAX_HGT, MAX_WID) <= 100);
    }
}

TEST_CASE("calc_wormhole_pairs: 最低 1 対で、広いフロアほど増える")
{
    CHECK(calc_wormhole_pairs(45, 45) == 1);
    CHECK(calc_wormhole_pairs(MAX_DUNGEON_HGT, MAX_DUNGEON_WID) >= 2);

    SUBCASE("面積について単調非減少、かつ必ず 1 対以上")
    {
        auto previous = 0;
        for (auto side = 1; side <= MAX_DUNGEON_HGT; side++) {
            const auto pairs = calc_wormhole_pairs(side, side);
            CHECK(pairs >= 1);
            CHECK(pairs >= previous);
            previous = pairs;
        }
    }

    SUBCASE("異常値でも 1 対に落ちる")
    {
        CHECK(calc_wormhole_pairs(0, 0) == 1);
        CHECK(calc_wormhole_pairs(-5, 10) == 1);
    }
}
