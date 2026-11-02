/*!
 * @brief モンスターの定義とダンジョンの出現条件に共通するフラグ名の読み込み
 * (grab_one_monster_basic_flag・grab_one_monster_spell_flag) のテスト
 *
 * 各 reader が渡す実際のフラグ群の対応は、各 reader のテストで扱う。
 * ここでは共通処理が、どの表を探し、見つからないときにどう振る舞うかを確認する。
 */

#include "info-reader/monster-flag-reader.h"
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-era-flags.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-kind-flags.h"
#include "monster-race/race-misc-flags.h"
#include "util/flag-group.h"
#include "view/display-messages.h"
#include <doctest/doctest.h>
#include <string>

namespace {
//! テスト用に全フラグ群の実体を保持し、MonsterBasicFlagGroups を組み立てるためのホルダ。
struct BasicFlagStorage {
    EnumClassFlagGroup<MonsterFeedType> meat_feed_flags;
    EnumClassFlagGroup<MonsterResistanceType> resistance_flags;
    EnumClassFlagGroup<MonsterAuraType> aura_flags;
    EnumClassFlagGroup<MonsterBehaviorType> behavior_flags;
    EnumClassFlagGroup<MonsterVisualType> visual_flags;
    EnumClassFlagGroup<MonsterKindType> kind_flags;
    EnumClassFlagGroup<MonsterEraType> era_flags;
    EnumClassFlagGroup<MonsterDropType> drop_flags;
    EnumClassFlagGroup<MonsterWildernessType> wilderness_flags;
    EnumClassFlagGroup<MonsterFeatureType> feature_flags;
    EnumClassFlagGroup<MonsterPopulationType> population_flags;
    EnumClassFlagGroup<MonsterSpeakType> speak_flags;
    EnumClassFlagGroup<MonsterBrightnessType> brightness_flags;
    EnumClassFlagGroup<MonsterSpecialType> special_flags;
    EnumClassFlagGroup<MonsterMiscType> misc_flags;

    //! @param with_era 時代フラグを扱うか (モンスターの定義なら true、ダンジョンの出現条件なら false)
    MonsterBasicFlagGroups make_groups(bool with_era)
    {
        return MonsterBasicFlagGroups{
            .meat_feed_flags = &this->meat_feed_flags,
            .resistance_flags = &this->resistance_flags,
            .aura_flags = &this->aura_flags,
            .behavior_flags = &this->behavior_flags,
            .visual_flags = &this->visual_flags,
            .kind_flags = &this->kind_flags,
            .era_flags = with_era ? &this->era_flags : nullptr,
            .drop_flags = &this->drop_flags,
            .wilderness_flags = &this->wilderness_flags,
            .feature_flags = &this->feature_flags,
            .population_flags = &this->population_flags,
            .speak_flags = &this->speak_flags,
            .brightness_flags = &this->brightness_flags,
            .special_flags = &this->special_flags,
            .misc_flags = &this->misc_flags,
        };
    }
};
}

TEST_CASE("grab_one_monster_basic_flag sets a flag found in one of the basic tables")
{
    BasicFlagStorage storage;
    const auto groups = storage.make_groups(true);
    ScopedMessageCapture diagnostics;

    CHECK(grab_one_monster_basic_flag(groups, "RES_ACID"));
    CHECK(storage.resistance_flags.has(MonsterResistanceType::RESIST_ACID));
    CHECK(storage.resistance_flags.count() == 1);

    CHECK(grab_one_monster_basic_flag(groups, "UNIQUE"));
    CHECK(storage.kind_flags.has(MonsterKindType::UNIQUE));

    CHECK(grab_one_monster_basic_flag(groups, "EAT_SLEEP"));
    CHECK(storage.meat_feed_flags.has(MonsterFeedType::SLEEP));

    CHECK(diagnostics.messages().empty());
}

TEST_CASE("grab_one_monster_basic_flag reads the era table only when era_flags is set")
{
    SUBCASE("era table is searched for the monster definition")
    {
        BasicFlagStorage storage;
        const auto groups = storage.make_groups(true);
        ScopedMessageCapture diagnostics;
        CHECK(grab_one_monster_basic_flag(groups, "ANCIENT"));
        CHECK(storage.era_flags.has(MonsterEraType::ANCIENT));
        CHECK(diagnostics.messages().empty());
    }

    SUBCASE("era table is skipped for the dungeon appearance condition")
    {
        BasicFlagStorage storage;
        const auto groups = storage.make_groups(false);
        ScopedMessageCapture diagnostics;
        CHECK_FALSE(grab_one_monster_basic_flag(groups, "ANCIENT"));
        CHECK_FALSE(storage.era_flags.any());
        REQUIRE(diagnostics.messages().size() == 1);
        // 文言は JP/EN で変わるため、報告された未知フラグ名が含まれることだけを確認する。
        CHECK(diagnostics.messages()[0].find("ANCIENT") != std::string::npos);
    }
}

TEST_CASE("grab_one_monster_basic_flag reports an unknown name once without setting anything")
{
    BasicFlagStorage storage;
    const auto groups = storage.make_groups(true);
    ScopedMessageCapture diagnostics;
    CHECK_FALSE(grab_one_monster_basic_flag(groups, "NOT_A_MONSTER_FLAG"));
    REQUIRE(diagnostics.messages().size() == 1);
    CHECK(diagnostics.messages()[0].find("NOT_A_MONSTER_FLAG") != std::string::npos);
    CHECK_FALSE(storage.resistance_flags.any());
    CHECK_FALSE(storage.kind_flags.any());
}

TEST_CASE("grab_one_monster_spell_flag sets a flag found in the ability table")
{
    EnumClassFlagGroup<MonsterAbilityType> ability_flags;
    ScopedMessageCapture diagnostics;
    CHECK(grab_one_monster_spell_flag(ability_flags, "ROCKET"));
    CHECK(ability_flags.has(MonsterAbilityType::ROCKET));
    CHECK(ability_flags.count() == 1);
    CHECK(diagnostics.messages().empty());
}

TEST_CASE("grab_one_monster_spell_flag reports an unknown name once without setting anything")
{
    EnumClassFlagGroup<MonsterAbilityType> ability_flags;
    ScopedMessageCapture diagnostics;
    CHECK_FALSE(grab_one_monster_spell_flag(ability_flags, "NOT_AN_ABILITY"));
    REQUIRE(diagnostics.messages().size() == 1);
    CHECK(diagnostics.messages()[0].find("NOT_AN_ABILITY") != std::string::npos);
    CHECK_FALSE(ability_flags.any());
}
