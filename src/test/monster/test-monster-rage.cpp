/*!
 * @file test-monster-rage.cpp
 * @brief 植木鉢くんへの激怒 (ENRAGED_BY_BROKEN_POT) による常時ボーナス (Issue #180)
 */

#include "monster/monster-flag-types.h"
#include "monster/monster-rage.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include <doctest/doctest.h>
#include <memory>

TEST_CASE("Rage by broken plant pot")
{
    auto floor_ptr = std::make_unique<FloorType>();
    auto &floor = *floor_ptr;
    floor.height = 9;
    floor.width = 9;

    auto &monster = floor.m_list[1];

    SUBCASE("A monster is not enraged by default")
    {
        CHECK_FALSE(monster.is_enraged_by_broken_pot());
        CHECK(monster.get_rage_melee_hit_bonus() == 0);
        CHECK(monster.get_rage_melee_damage_bonus() == 0);
        CHECK(monster.get_save_rage_bonus() == 0);
    }

    SUBCASE("Setting the flag turns on every bonus at once")
    {
        monster.set_constant_flag(MonsterConstantFlagType::ENRAGED_BY_BROKEN_POT);
        CHECK(monster.is_enraged_by_broken_pot());
        CHECK(monster.get_rage_melee_hit_bonus() == RAGE_MELEE_HIT_BONUS);
        CHECK(monster.get_rage_melee_damage_bonus() == RAGE_MELEE_DAMAGE_BONUS);
        CHECK(monster.get_save_rage_bonus() == RAGE_SAVE_BONUS);
    }

    SUBCASE("The rage flag is independent of the existing anger flag")
    {
        // Issue #180 は「周囲のモンスターを怒らせる」(ANGER) とは別枠の指定
        monster.set_constant_flag(MonsterConstantFlagType::ANGER);
        CHECK(monster.is_angered());
        CHECK_FALSE(monster.is_enraged_by_broken_pot());

        monster.reset_constant_flag(MonsterConstantFlagType::ANGER);
        monster.set_constant_flag(MonsterConstantFlagType::ENRAGED_BY_BROKEN_POT);
        CHECK(monster.is_enraged_by_broken_pot());
        CHECK_FALSE(monster.is_angered());
    }

    SUBCASE("The buff is strictly larger than the plain frenzy speed bonus")
    {
        // 既存の怒り・狂乱より「大きくバフがつく」ことが本機能の要件
        CHECK(RAGE_SPEED_BONUS > 10);
        CHECK(RAGE_MELEE_HIT_BONUS > 0);
        CHECK(RAGE_MELEE_DAMAGE_BONUS > 0);
        CHECK(RAGE_AC_BONUS > 0);
        CHECK(RAGE_SAVE_BONUS > 0);
    }
}

TEST_CASE("Rage radius")
{
    // 散開した一団が巻き込まれる程度の広さで、フロア全域には及ばない
    CHECK(RAGE_BROKEN_POT_RADIUS > 0);
    CHECK(RAGE_BROKEN_POT_RADIUS < 40);
}
