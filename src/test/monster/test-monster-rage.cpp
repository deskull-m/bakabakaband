/*!
 * @file test-monster-rage.cpp
 * @brief 植木鉢くんへの激怒 (ENRAGED_BY_BROKEN_POT) とプレイヤーの罪悪感
 *        (BROKEN_POT_GUILT) の派生値 (Issue #180)
 */

#include "monster/monster-flag-types.h"
#include "monster/monster-rage.h"
#include "system/creature-entity.h"
#include "system/creature-timed-effect-types.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/body-structure-types.h"
#include "system/monrace/monrace-definition.h"
#include "util/enum-converter.h"
#include <algorithm>
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

TEST_CASE("Rage speech gate is the humanoid body, not CAN_SPEAK")
{
    MonraceDefinition monrace;

    SUBCASE("Humanoid bodies can snarl")
    {
        for (const auto body : { BodyStructureType::HUMANOID, BodyStructureType::WINGED_HUMANOID, BodyStructureType::WING_ARMED_HUMANOID }) {
            monrace.body_structure = body;
            CHECK(monrace.is_humanoid_body());
        }
    }

    SUBCASE("Every other body structure cannot")
    {
        // 竜体は全 13 スロットを持つが人型ではないので含めない
        for (const auto body : { BodyStructureType::DRACONIC, BodyStructureType::BIPEDAL, BodyStructureType::QUADRUPED,
                 BodyStructureType::SERPENTINE, BodyStructureType::AMORPHOUS, BodyStructureType::INCORPOREAL,
                 BodyStructureType::FORMLESS, BodyStructureType::GASEOUS, BodyStructureType::VORTEX,
                 BodyStructureType::XAREN, BodyStructureType::AVIAN, BodyStructureType::WORM,
                 BodyStructureType::INSECTOID, BodyStructureType::SPHERE }) {
            monrace.body_structure = body;
            CHECK_FALSE(monrace.is_humanoid_body());
        }
    }

    SUBCASE("The default body structure is humanoid")
    {
        MonraceDefinition fresh;
        CHECK(fresh.is_humanoid_body());
    }
}

TEST_CASE("Rage speech chance")
{
    // 毎ターン喋ると鬱陶しいので 1/N に抑える
    CHECK(RAGE_SPEECH_CHANCE > 1);
}

TEST_CASE("Guilt of breaking the plant pot")
{
    auto floor_ptr = std::make_unique<FloorType>();
    auto &creature = floor_ptr->m_list[1];

    SUBCASE("Nobody is guilty by default")
    {
        CHECK(creature.get_timed_effect(CreatureTimedEffect::BROKEN_POT_GUILT) == 0);
    }

    SUBCASE("The timed effect round-trips")
    {
        creature.set_timed_effect(CreatureTimedEffect::BROKEN_POT_GUILT, 123);
        CHECK(creature.get_timed_effect(CreatureTimedEffect::BROKEN_POT_GUILT) == 123);

        // 他の時限効果を巻き込まない (列挙の末尾追加なので隣と混ざりやすい)
        CHECK(creature.get_timed_effect(CreatureTimedEffect::ALTER_REALITY) == 0);
        CHECK(creature.get_timed_effect(CreatureTimedEffect::WORD_RECALL) == 0);

        creature.set_timed_effect(CreatureTimedEffect::BROKEN_POT_GUILT, 0);
        CHECK(creature.get_timed_effect(CreatureTimedEffect::BROKEN_POT_GUILT) == 0);
    }

    SUBCASE("The penalties are a real handicap but bounded")
    {
        CHECK(BROKEN_POT_GUILT_DURATION_BASE > 0);
        CHECK(BROKEN_POT_GUILT_DURATION_DICE > 0);
        CHECK(BROKEN_POT_GUILT_AC_PENALTY > 0);
        CHECK(BROKEN_POT_GUILT_CRITICAL_SIDES > 0);

        // 回復手段が時間経過しか無いので、1 回で何万ターンも続いては困る
        CHECK(BROKEN_POT_GUILT_DURATION_BASE + BROKEN_POT_GUILT_DURATION_DICE < 10000);
    }

    SUBCASE("Raising the critical die sides can only raise the critical rate")
    {
        // SUPERHURT のクリティカル判定 randint1(sides) > ac + 200 は
        // 面数が増えるほど閾値を越えやすくなる (単調増加) ことが本ペナルティの根拠
        const auto critical_rate = [](int rlev, int ac, int bonus) {
            const auto sides = rlev * 2 + 300 + bonus;
            const auto winners = std::max(0, sides - (ac + 200));
            return static_cast<double>(winners) / sides;
        };

        for (const auto rlev : { 1, 20, 60, 99 }) {
            for (const auto ac : { 0, 50, 100, 150 }) {
                CHECK(critical_rate(rlev, ac, BROKEN_POT_GUILT_CRITICAL_SIDES) >= critical_rate(rlev, ac, 0));
            }
        }

        // AC が高い個体ほど効きが大きい (壁を越えられるようになるため)
        CHECK(critical_rate(20, 150, BROKEN_POT_GUILT_CRITICAL_SIDES) > critical_rate(20, 150, 0));
    }

    SUBCASE("The new timed effect is appended at the tail of the enum")
    {
        // セーブデータは列挙順にダンプされる (v60 以降は件数前置)。
        // 中間挿入すると全時限効果がずれるため、必ず MAX の直前であること。
        CHECK(enum2i(CreatureTimedEffect::BROKEN_POT_GUILT) == enum2i(CreatureTimedEffect::MAX) - 1);
    }
}
