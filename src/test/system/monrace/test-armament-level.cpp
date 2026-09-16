/*!
 * @brief モンスター種族の武装度 (armament_level) のテスト
 *
 * 武装度は「生成時にどれだけ上質な装備を与えるか」を表す種族固有の値で、
 * JSON で明示指定できる。未指定の種族には種族レベルから算出した既定値
 * (level * ARMAMENT_LEVEL_PER_LEVEL) が与えられる。
 * 既定値の算出はレベルに依存するため、フィールドを直接読まず
 * get_armament_level() を経由することが前提となっている。
 */

#include "system/monrace/monrace-definition.h"

#include <doctest/doctest.h>

TEST_CASE("MonraceDefinition::get_armament_level")
{
    SUBCASE("未指定なら種族レベル比例の既定値を返す")
    {
        MonraceDefinition monrace;
        REQUIRE_FALSE(monrace.armament_level.has_value());

        monrace.level = 0;
        CHECK(monrace.get_armament_level() == 0);

        monrace.level = 1;
        CHECK(monrace.get_armament_level() == MonraceDefinition::ARMAMENT_LEVEL_PER_LEVEL);

        monrace.level = 40;
        CHECK(monrace.get_armament_level() == 40 * MonraceDefinition::ARMAMENT_LEVEL_PER_LEVEL);

        // レベル上限 (255) でも既定値は指定可能上限に収まる
        monrace.level = 255;
        CHECK(monrace.get_armament_level() == 255 * MonraceDefinition::ARMAMENT_LEVEL_PER_LEVEL);
        CHECK(monrace.get_armament_level() <= MonraceDefinition::ARMAMENT_LEVEL_MAX);
    }

    SUBCASE("明示指定があればレベルに依らずその値を返す")
    {
        MonraceDefinition monrace;
        monrace.level = 40;
        monrace.armament_level = 123;
        CHECK(monrace.get_armament_level() == 123);

        // レベルを変えても明示指定値は変わらない
        monrace.level = 1;
        CHECK(monrace.get_armament_level() == 123);
    }

    SUBCASE("0 の明示指定は既定値ではなく 0 として扱う")
    {
        MonraceDefinition monrace;
        monrace.level = 40;
        monrace.armament_level = 0;
        CHECK(monrace.get_armament_level() == 0);
    }
}
