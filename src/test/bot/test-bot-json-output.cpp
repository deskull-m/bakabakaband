/*!
 * @brief 制御サーバの state 出力のテスト
 */

#include "bot/bot-json-output.h"

#include "locale/localized-string.h"
#include "object-enchant/tr-types.h"
#include "system/creature-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "test/system/monrace-records-test-access.h"
#include "util/enum-converter.h"

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>

TEST_CASE("make_bot_pet_row_json hides the race while hallucinating")
{
    test::MonraceRecordsTestAccess monraces;
    auto &monrace = MonraceList::get_instance().emplace(static_cast<MonraceId>(1));
    monrace.name = LocalizedString("Test pet", "Test pet");
    CreatureEntity monster;
    monster.set_r_idx(static_cast<MonraceId>(1));
    CHECK(make_bot_pet_row_json(5, monster, false) == nlohmann::json{ { "index", 5 }, { "race_id", 1 }, { "name", "Test pet" } });
    CHECK(make_bot_pet_row_json(5, monster, true) == nlohmann::json{ { "index", 5 }, { "hallucinated", true } });
}

TEST_CASE("make_bot_player_ability_sources_json reports each source of the character sheet abilities")
{
    const std::vector<std::pair<std::string, tr_type>> abilities{
        { "hold_exp", TR_HOLD_EXP },
        { "warning", TR_WARNING },
        { "slow_digest", TR_SLOW_DIGEST },
        { "regenerate", TR_REGEN },
        { "levitation", TR_LEVITATION },
        { "reflect", TR_REFLECT },
        { "sustain_str", TR_SUST_STR },
        { "sustain_int", TR_SUST_INT },
        { "sustain_wis", TR_SUST_WIS },
        { "sustain_dex", TR_SUST_DEX },
        { "sustain_con", TR_SUST_CON },
        { "sustain_chr", TR_SUST_CHR },
    };
    // 供給源を 1 つずつ立て、そのキーの同じ供給源だけが true になることを確かめる
    const std::vector<std::pair<std::string, TrFlags BotPlayerKnownFlags::*>> source_members{
        { "equipment", &BotPlayerKnownFlags::equipment },
        { "permanent", &BotPlayerKnownFlags::permanent },
        { "temporary", &BotPlayerKnownFlags::temporary },
    };
    // clang 14/15 は構造化束縛を CAPTURE() 内のラムダに捕捉できない (P1091 は clang16 以降) ため通常変数で受ける
    for (const auto &ability : abilities) {
        const auto key = ability.first;
        const auto flag = ability.second;
        CAPTURE(key);
        for (const auto &source_member : source_members) {
            const auto source = source_member.first;
            const auto member = source_member.second;
            CAPTURE(source);
            BotPlayerKnownFlags flags;
            (flags.*member).set(flag);
            const auto sources = make_bot_player_ability_sources_json(flags);
            REQUIRE(sources.contains(key));
            for (const auto &other : source_members) {
                CHECK(sources.at(key).at(other.first) == (other.first == source));
            }
        }

        // 乗馬の分を持つのは浮遊だけ
        CHECK(make_bot_player_ability_sources_json({}).at(key).contains("riding") == (key == "levitation"));
    }
}

TEST_CASE("make_bot_player_ability_sources_json reports the levitation from the mount")
{
    // 乗馬の分は浮遊の riding にだけ出て、どの能力の供給源にも混ざらない
    BotPlayerKnownFlags riding_only;
    riding_only.riding.set(TR_LEVITATION);
    const auto riding_only_sources = make_bot_player_ability_sources_json(riding_only);
    for (const auto &item : riding_only_sources.items()) {
        const auto key = item.key();
        const auto &value = item.value();
        CAPTURE(key);
        CHECK(value.at("equipment") == false);
        CHECK(value.at("permanent") == false);
        CHECK(value.at("temporary") == false);
    }

    BotPlayerKnownFlags flags;
    flags.equipment.set(TR_LEVITATION);
    CHECK(make_bot_player_ability_sources_json(flags).at("levitation").at("riding").is_null());

    // 浮遊する馬に乗っている
    flags.riding.set(TR_LEVITATION);
    CHECK(make_bot_player_ability_sources_json(flags).at("levitation").at("riding") == true);

    // 浮遊しない馬に乗っていて、装備の浮遊があっても浮遊しない
    flags.riding.reset(TR_LEVITATION);
    flags.riding_negative.set(TR_LEVITATION);
    const auto sources = make_bot_player_ability_sources_json(flags);
    CHECK(sources.at("levitation").at("riding") == false);
    CHECK(sources.at("levitation").at("equipment") == true);
}
