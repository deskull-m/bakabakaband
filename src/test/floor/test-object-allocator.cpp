/*!
 * @brief 対向配置階段 (POLARIZED_STAIRS) の辺と帯の算出のテスト
 */

#include "floor/object-allocator.h"
#include "util/point-2d.h"
#include <array>
#include <doctest/doctest.h>

TEST_CASE("get_opposite_edge: 正反対の辺を返す")
{
    CHECK(get_opposite_edge(FloorEdge::NORTH) == FloorEdge::SOUTH);
    CHECK(get_opposite_edge(FloorEdge::SOUTH) == FloorEdge::NORTH);
    CHECK(get_opposite_edge(FloorEdge::WEST) == FloorEdge::EAST);
    CHECK(get_opposite_edge(FloorEdge::EAST) == FloorEdge::WEST);

    SUBCASE("2 回適用すると元に戻る")
    {
        constexpr std::array edges = { FloorEdge::NORTH, FloorEdge::SOUTH, FloorEdge::WEST, FloorEdge::EAST };
        for (const auto edge : edges) {
            CHECK(get_opposite_edge(get_opposite_edge(edge)) == edge);
        }
    }
}

TEST_CASE("calc_edge_band_depth: 辺の長さの 1/4 で最低 1")
{
    CHECK(calc_edge_band_depth(132) == 33);
    CHECK(calc_edge_band_depth(45) == 11);
    CHECK(calc_edge_band_depth(4) == 1);
    CHECK(calc_edge_band_depth(1) == 1); // 下限で潰れない
    CHECK(calc_edge_band_depth(0) == 1);
    CHECK(calc_edge_band_depth(-10) == 1); // 異常値でも 1 に落ちる
}

TEST_CASE("is_within_edge_band: 各辺の帯の境界")
{
    constexpr auto h = 132;
    constexpr auto w = 132;
    const auto depth = calc_edge_band_depth(h); // 33

    SUBCASE("北は y <= 帯の厚み")
    {
        CHECK(is_within_edge_band(Pos2D(0, 60), h, w, FloorEdge::NORTH));
        CHECK(is_within_edge_band(Pos2D(depth, 60), h, w, FloorEdge::NORTH));
        CHECK_FALSE(is_within_edge_band(Pos2D(depth + 1, 60), h, w, FloorEdge::NORTH));
        CHECK_FALSE(is_within_edge_band(Pos2D(h - 1, 60), h, w, FloorEdge::NORTH));
    }

    SUBCASE("南は北の鏡像")
    {
        CHECK(is_within_edge_band(Pos2D(h - 1, 60), h, w, FloorEdge::SOUTH));
        CHECK(is_within_edge_band(Pos2D(h - 1 - depth, 60), h, w, FloorEdge::SOUTH));
        CHECK_FALSE(is_within_edge_band(Pos2D(h - 2 - depth, 60), h, w, FloorEdge::SOUTH));
        CHECK_FALSE(is_within_edge_band(Pos2D(0, 60), h, w, FloorEdge::SOUTH));
    }

    SUBCASE("西と東は x で判定する")
    {
        CHECK(is_within_edge_band(Pos2D(60, 0), h, w, FloorEdge::WEST));
        CHECK(is_within_edge_band(Pos2D(60, depth), h, w, FloorEdge::WEST));
        CHECK_FALSE(is_within_edge_band(Pos2D(60, depth + 1), h, w, FloorEdge::WEST));
        CHECK(is_within_edge_band(Pos2D(60, w - 1), h, w, FloorEdge::EAST));
        CHECK_FALSE(is_within_edge_band(Pos2D(60, w - 2 - depth), h, w, FloorEdge::EAST));
    }

    SUBCASE("対向する帯は重ならない")
    {
        for (auto y = 0; y < h; y++) {
            const Pos2D pos(y, 60);
            const auto in_north = is_within_edge_band(pos, h, w, FloorEdge::NORTH);
            const auto in_south = is_within_edge_band(pos, h, w, FloorEdge::SOUTH);
            // doctest は && / || を含む式を分解できないため、真偽値を変数で受けてから比較する
            const auto overlaps = in_north && in_south;
            CHECK_FALSE(overlaps);
        }
    }

    SUBCASE("極小フロアでは対向する帯が重なりうる")
    {
        // 帯の厚みが 1 に丸められるため、高さ 2 では両端が互いの帯に入る。
        // 階段の置き場が無くなるより重なるほうが安全なので許容する。
        CHECK(is_within_edge_band(Pos2D(1, 0), 2, 2, FloorEdge::NORTH));
        CHECK(is_within_edge_band(Pos2D(0, 0), 2, 2, FloorEdge::SOUTH));
    }
}
