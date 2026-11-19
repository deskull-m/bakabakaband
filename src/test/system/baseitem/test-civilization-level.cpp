/*!
 * @brief 文明水準 (CivilizationLevel) のテスト
 */

#include "system/baseitem/civilization-level-types.h"

#include <doctest/doctest.h>

#include <vector>

namespace {
//! NONE を除く全水準を「文明の進み具合」の昇順に並べたもの
const std::vector<CivilizationLevel> &all_levels()
{
    static const std::vector<CivilizationLevel> levels = {
        CivilizationLevel::STONE,
        CivilizationLevel::BRONZE,
        CivilizationLevel::IRON,
        CivilizationLevel::INDUSTRIAL,
        CivilizationLevel::ATOMIC,
        CivilizationLevel::INFORMATION,
        CivilizationLevel::SOLAR,
        CivilizationLevel::GALACTIC,
    };

    return levels;
}
}

TEST_CASE("CivilizationLevel enum order follows civilization progress")
{
    // 「水準以下なら扱える」判定が enum 値の順序に依存するため、順序を固定しておく。
    const auto &levels = all_levels();
    for (auto i = 1U; i < levels.size(); i++) {
        CHECK(levels[i - 1] < levels[i]);
    }

    CHECK(CivilizationLevel::NONE < CivilizationLevel::STONE);
    CHECK(CivilizationLevel::GALACTIC < CivilizationLevel::MAX);
}

TEST_CASE("civilization_level_name returns an empty string only for NONE and MAX")
{
    for (const auto level : all_levels()) {
        CHECK_FALSE(civilization_level_name(level).empty());
    }

    CHECK(civilization_level_name(CivilizationLevel::NONE).empty());
    CHECK(civilization_level_name(CivilizationLevel::MAX).empty());
}

TEST_CASE("is_civilization_usable allows items at or below the user's level")
{
    // 鉄器水準の存在は石器・青銅器・鉄器の品を扱えるが、それより先の品は扱えない。
    CHECK(is_civilization_usable(CivilizationLevel::STONE, CivilizationLevel::IRON));
    CHECK(is_civilization_usable(CivilizationLevel::BRONZE, CivilizationLevel::IRON));
    CHECK(is_civilization_usable(CivilizationLevel::IRON, CivilizationLevel::IRON));
    CHECK_FALSE(is_civilization_usable(CivilizationLevel::INDUSTRIAL, CivilizationLevel::IRON));
    CHECK_FALSE(is_civilization_usable(CivilizationLevel::GALACTIC, CivilizationLevel::IRON));

    // 石器水準の存在は石器の品しか扱えない。
    CHECK(is_civilization_usable(CivilizationLevel::STONE, CivilizationLevel::STONE));
    CHECK_FALSE(is_civilization_usable(CivilizationLevel::BRONZE, CivilizationLevel::STONE));

    // 最高水準の存在は全ての品を扱える。
    for (const auto level : all_levels()) {
        CHECK(is_civilization_usable(level, CivilizationLevel::GALACTIC));
    }
}

TEST_CASE("is_civilization_usable is unrestricted when either side is NONE")
{
    // 品側の NONE (幻想の品) はどの水準の存在でも扱える。
    for (const auto level : all_levels()) {
        CHECK(is_civilization_usable(CivilizationLevel::NONE, level));
    }

    // 扱う側の NONE (文明に属さない存在) はどの水準の品でも扱える。
    for (const auto level : all_levels()) {
        CHECK(is_civilization_usable(level, CivilizationLevel::NONE));
    }

    CHECK(is_civilization_usable(CivilizationLevel::NONE, CivilizationLevel::NONE));
}
