/*!
 * @brief 16 方位の方角名ユーティリティのテスト
 */

#include "util/compass-direction.h"
#include "util/point-2d.h"
#include <array>
#include <doctest/doctest.h>
#include <set>
#include <string_view>

TEST_CASE("calc_compass_direction_index: 8 方向は 2 刻みの添字になる")
{
    // フロア座標系は y が南へ増える。北を 0 として時計回りに 1 刻み = 22.5 度。
    CHECK(calc_compass_direction_index(Pos2DVec(-10, 0)) == 0); // 北
    CHECK(calc_compass_direction_index(Pos2DVec(-10, 10)) == 2); // 北東
    CHECK(calc_compass_direction_index(Pos2DVec(0, 10)) == 4); // 東
    CHECK(calc_compass_direction_index(Pos2DVec(10, 10)) == 6); // 南東
    CHECK(calc_compass_direction_index(Pos2DVec(10, 0)) == 8); // 南
    CHECK(calc_compass_direction_index(Pos2DVec(10, -10)) == 10); // 南西
    CHECK(calc_compass_direction_index(Pos2DVec(0, -10)) == 12); // 西
    CHECK(calc_compass_direction_index(Pos2DVec(-10, -10)) == 14); // 北西
}

TEST_CASE("calc_compass_direction_index: 隣接 1 マスでも 8 方向が正しく出る")
{
    CHECK(calc_compass_direction_index(Pos2DVec(-1, 0)) == 0);
    CHECK(calc_compass_direction_index(Pos2DVec(-1, 1)) == 2);
    CHECK(calc_compass_direction_index(Pos2DVec(0, 1)) == 4);
    CHECK(calc_compass_direction_index(Pos2DVec(1, 1)) == 6);
    CHECK(calc_compass_direction_index(Pos2DVec(1, 0)) == 8);
    CHECK(calc_compass_direction_index(Pos2DVec(1, -1)) == 10);
    CHECK(calc_compass_direction_index(Pos2DVec(0, -1)) == 12);
    CHECK(calc_compass_direction_index(Pos2DVec(-1, -1)) == 14);
}

TEST_CASE("calc_compass_direction_index: 中間方位 (奇数添字) も出る")
{
    // tan(22.5 度) = 0.4142 なので 100:41 がちょうど中間方位の中心付近になる。
    CHECK(calc_compass_direction_index(Pos2DVec(-100, 41)) == 1); // 北北東
    CHECK(calc_compass_direction_index(Pos2DVec(-41, 100)) == 3); // 東北東
    CHECK(calc_compass_direction_index(Pos2DVec(41, 100)) == 5); // 東南東
    CHECK(calc_compass_direction_index(Pos2DVec(100, 41)) == 7); // 南南東
    CHECK(calc_compass_direction_index(Pos2DVec(100, -41)) == 9); // 南南西
    CHECK(calc_compass_direction_index(Pos2DVec(41, -100)) == 11); // 西南西
    CHECK(calc_compass_direction_index(Pos2DVec(-41, -100)) == 13); // 西北西
    CHECK(calc_compass_direction_index(Pos2DVec(-100, -41)) == 15); // 北北西
}

TEST_CASE("calc_compass_direction_index: 常に 0～15 に収まる")
{
    for (auto dy = -20; dy <= 20; dy++) {
        for (auto dx = -20; dx <= 20; dx++) {
            const auto index = calc_compass_direction_index(Pos2DVec(dy, dx));
            REQUIRE(index >= 0);
            REQUIRE(index <= 15);
        }
    }
}

TEST_CASE("describe_compass_direction: 添字に対応する名称が空でない")
{
    // 言語ごとに文字列が異なるので内容ではなく「空でないこと」と
    // 「8 方向が互いに異なること」を確認する。
    const std::array<Pos2DVec, 8> vecs = {
        Pos2DVec(-10, 0), Pos2DVec(-10, 10), Pos2DVec(0, 10), Pos2DVec(10, 10),
        Pos2DVec(10, 0), Pos2DVec(10, -10), Pos2DVec(0, -10), Pos2DVec(-10, -10)
    };
    std::set<std::string_view> names;
    for (const auto &vec : vecs) {
        const auto name = describe_compass_direction(vec);
        CHECK(!name.empty());
        names.insert(name);
    }

    CHECK(names.size() == vecs.size());
}
