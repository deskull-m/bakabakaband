/*!
 * @brief 魔道具を砕いたときの魔力暴発 (Issue #1770) の算出式のテスト
 */

#include "object/magic-device-shatter.h"
#include <doctest/doctest.h>

TEST_CASE("calc_device_shatter_damage: 残り回数と生成レベルの双方に依存する")
{
    // 威力 = 生成レベル * (土台 5 + 残り回数) / 5
    CHECK(calc_device_shatter_damage(0, 100) == 100); // 魔力が尽きていても生成レベルぶんは暴発する
    CHECK(calc_device_shatter_damage(5, 100) == 200);
    CHECK(calc_device_shatter_damage(5, 85) == 170);
    CHECK(calc_device_shatter_damage(4, 75) == 135);
    CHECK(calc_device_shatter_damage(20, 1) == 5); // 浅い階の魔道具は回数が多くても軽い

    SUBCASE("残り回数について単調非減少")
    {
        auto previous = 0;
        for (auto charges = 0; charges <= 40; ++charges) {
            const auto damage = calc_device_shatter_damage(charges, 50);
            CHECK(damage >= previous);
            previous = damage;
        }
    }

    SUBCASE("生成レベルについて単調非減少")
    {
        auto previous = 0;
        for (auto level = 0; level <= 100; ++level) {
            const auto damage = calc_device_shatter_damage(5, level);
            CHECK(damage >= previous);
            previous = damage;
        }
    }
}

TEST_CASE("calc_device_shatter_damage: 下限 1 と上限で飽和する")
{
    CHECK(calc_device_shatter_damage(0, 0) == 1);
    CHECK(calc_device_shatter_damage(-10, -10) == 1); // 異常値でも 1 に落ちる
    CHECK(calc_device_shatter_damage(10000, 100) == 500);
    CHECK(calc_device_shatter_damage(100, 1000) == 500);
}

TEST_CASE("calc_device_shatter_self_damage: 威力の 1/3 で最低 1")
{
    CHECK(calc_device_shatter_self_damage(300) == 100);
    CHECK(calc_device_shatter_self_damage(200) == 66);
    CHECK(calc_device_shatter_self_damage(1) == 1);
    CHECK(calc_device_shatter_self_damage(0) == 1);

    SUBCASE("常に威力より小さい")
    {
        for (auto damage = 1; damage <= 500; ++damage) {
            CHECK(calc_device_shatter_self_damage(damage) <= damage);
        }
    }
}

TEST_CASE("calc_device_shatter_radius: 生成レベル 50 ごとに 1 伸びる")
{
    CHECK(calc_device_shatter_radius(0) == 2);
    CHECK(calc_device_shatter_radius(49) == 2);
    CHECK(calc_device_shatter_radius(50) == 3);
    CHECK(calc_device_shatter_radius(99) == 3);
    CHECK(calc_device_shatter_radius(100) == 4);
    CHECK(calc_device_shatter_radius(-5) == 2); // 異常値でも基本半径に落ちる
}
