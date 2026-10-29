/*!
 * @brief 制御サーバの state 出力のテスト
 */

#include "bot/bot-json-output.h"

#include "locale/localized-string.h"
#include "system/creature-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "test/system/monrace-records-test-access.h"
#include "util/enum-converter.h"

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

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
