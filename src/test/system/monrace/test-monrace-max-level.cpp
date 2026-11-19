/*!
 * @brief モンスターの生成上限階層 (max_level) 判定のテスト
 *
 * JSON の "max_level" (任意指定) は MonraceDefinition::max_level (tl::optional) に格納され、
 * MonraceDefinition::is_too_deep_to_generate() が「現在階が上限より深いか」を判定する。
 * 未指定なら常に false (上限なし)、指定時は上限階までは生成可 (inclusive) で
 * それより深い階では true (生成禁止) となることを検証する。
 */

#include "system/monrace/monrace-definition.h"

#include "floor/floor-base-definitions.h"

#include <doctest/doctest.h>

TEST_CASE("MonraceDefinition::is_too_deep_to_generate: max_level 未指定なら常に生成可")
{
    MonraceDefinition monrace;
    monrace.level = 10;
    CHECK_FALSE(monrace.max_level.has_value());
    CHECK_FALSE(monrace.is_too_deep_to_generate(0));
    CHECK_FALSE(monrace.is_too_deep_to_generate(10));
    CHECK_FALSE(monrace.is_too_deep_to_generate(MAX_DEPTH - 1));
}

TEST_CASE("MonraceDefinition::is_too_deep_to_generate: max_level は inclusive な上限")
{
    MonraceDefinition monrace;
    monrace.level = 5;
    monrace.max_level = 20;
    CHECK_FALSE(monrace.is_too_deep_to_generate(0));
    CHECK_FALSE(monrace.is_too_deep_to_generate(5));
    CHECK_FALSE(monrace.is_too_deep_to_generate(19));
    CHECK_FALSE(monrace.is_too_deep_to_generate(20));
    CHECK(monrace.is_too_deep_to_generate(21));
    CHECK(monrace.is_too_deep_to_generate(MAX_DEPTH - 1));
}

TEST_CASE("MonraceDefinition::is_too_deep_to_generate: max_level == level なら単一階のみ生成可")
{
    MonraceDefinition monrace;
    monrace.level = 3;
    monrace.max_level = 3;
    CHECK_FALSE(monrace.is_too_deep_to_generate(3));
    CHECK(monrace.is_too_deep_to_generate(4));
}
