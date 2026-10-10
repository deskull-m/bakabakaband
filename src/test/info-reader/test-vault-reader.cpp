#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/vault-reader.h"
#include "room/rooms-vault.h"
#include "room/vault-flag-types.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-list.h"
#include "util/enum-converter.h"
#include "util/finalizer.h"
#include <doctest/doctest.h>
#include <limits>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
nlohmann::json make_vault()
{
    return { { "id", 0 }, { "name", "Test vault" }, { "type", 7 }, { "rating", 5 }, { "height", 2 }, { "width", 4 }, { "layout", { " %: ", "\\\"# " } } };
}

auto preserve_vaults()
{
    return util::make_finalizer([saved = vaults_info, index = error_idx] {
        vaults_info = saved;
        error_idx = index;
    });
}
}

TEST_CASE("VaultReader preserves metadata and every layout byte")
{
    const auto restore = preserve_vaults();
    vaults_info.clear();
    error_idx = -1;
    auto data = make_vault();
    for (const auto type : { 7, 8, 17, 18, 19, 20 }) {
        data["id"] = type; // Sparse IDs remain supported.
        data["type"] = type;
        REQUIRE(VaultReader(data).read() == PARSE_ERROR_NONE);
        const auto &vault = vaults_info.at(type);
        CHECK(vault.idx == type);
        CHECK(vault.typ == type);
        CHECK(vault.rat == 5);
        CHECK(vault.hgt == 2);
        CHECK(vault.wid == 4);
        CHECK(vault.name == "Test vault");
        CHECK(vault.text == " %: \\\"# ");
        CHECK(error_idx == type);
    }
}

TEST_CASE("VaultReader reads bakabakaband extensions (depth, rarity, flags, features)")
{
    const auto restore = preserve_vaults();
    vaults_info.clear();
    error_idx = -1;
    auto data = make_vault();
    data["min_depth"] = 10;
    data["max_depth"] = 60;
    data["rarity"] = 2;
    data["flags"] = { "NO_ROTATION" };
    // A numeric monster id is only accepted when it is present in the loaded list,
    // so seed the one this case uses. MonraceDefinition::tag defaults to empty and
    // therefore never matches the token, which keeps the numeric path under test.
    MonraceList::get_instance().emplace(i2enum<MonraceId>(5));
    data["features"] = { { { "symbol", "G" }, { "feat", 1 }, { "monster", "5" } },
        { { "symbol", ";" }, { "feat", 327 }, { "appearance", 1 } } };
    REQUIRE(VaultReader(data).read() == PARSE_ERROR_NONE);
    const auto &vault = vaults_info.at(0);
    CHECK(vault.min_depth == 10);
    CHECK(vault.max_depth == 60);
    CHECK(vault.rarity == 2);
    CHECK(vault.flags.has(VaultFeatureType::NO_ROTATION));
    CHECK(vault.feature_list.at('G') == 1);
    CHECK(vault.feature_ap_list.at('G') == 1);
    CHECK(vault.place_monster_list.at('G') == i2enum<MonraceId>(5));
    CHECK(vault.feature_list.at(';') == 327);
    CHECK(vault.feature_ap_list.at(';') == 1);
}

TEST_CASE("VaultReader rejects invalid records atomically with field diagnostics")
{
    const auto restore = preserve_vaults();
    vaults_info.assign(1, {});
    vaults_info[0].name = "unchanged";
    vaults_info[0].text = "sentinel";
    error_idx = -1;
    const auto original = make_vault();
    auto check_invalid = [&](const nlohmann::json &data, std::string_view path) {
        VaultReader reader(data);
        CHECK(reader.read() != PARSE_ERROR_NONE);
        REQUIRE(reader.error().has_value());
        CHECK(reader.error()->path == path);
        CHECK_FALSE(reader.error()->reason.empty());
        REQUIRE(vaults_info.size() == 1);
        CHECK(vaults_info[0].name == "unchanged");
        CHECK(vaults_info[0].text == "sentinel");
        CHECK(error_idx == -1);
    };
    check_invalid(nlohmann::json::array(), "$");
    for (const auto key : { "id", "name", "type", "rating", "height", "width", "layout" }) {
        auto data = original;
        data.erase(key);
        check_invalid(data, std::string("$.") + key);
    }
    for (const auto key : { "id", "type", "rating", "height", "width" }) {
        for (const auto &value : std::vector<nlohmann::json>{ nullptr, true, "1", 1.5, -1, std::numeric_limits<uint64_t>::max() }) {
            auto data = original;
            data[key] = value;
            check_invalid(data, std::string("$.") + key);
        }
    }
    for (const auto &value : std::vector<nlohmann::json>{ "", nullptr, 42 }) {
        auto data = original;
        data["name"] = value;
        check_invalid(data, "$.name");
    }
    for (const auto &value : std::vector<nlohmann::json>{ nullptr, "abcd", nlohmann::json::array(), { "only" } }) {
        auto data = original;
        data["layout"] = value;
        check_invalid(data, "$.layout");
    }
    for (const auto &value : std::vector<nlohmann::json>{ nullptr, 1, "", "abc", "abcde", "ab\tc", std::string("a\0bc", 4), "\xc3\xa9"
                                                                                                                            "ab" }) {
        auto data = original;
        data["layout"][1] = value;
        check_invalid(data, "$.layout[1]");
    }
    for (const auto &[key, value] : std::vector<std::pair<std::string, int>>{ { "id", 32768 }, { "type", 9 }, { "height", 0 }, { "width", 0 }, { "height", 243 }, { "width", 243 } }) {
        auto data = original;
        auto &field = data.at(key);
        field = value;
        check_invalid(data, "$." + key);
    }
    auto data = original;
    data["typo"] = 0;
    check_invalid(data, "$.typo");
}

TEST_CASE("VaultReader rejects duplicate and descending IDs and permits retry")
{
    const auto restore = preserve_vaults();
    vaults_info.clear();
    error_idx = -1;
    auto data = make_vault();
    data["id"] = 2;
    REQUIRE(VaultReader(data).read() == PARSE_ERROR_NONE);
    CHECK(VaultReader(data).read() == PARSE_ERROR_NON_SEQUENTIAL_RECORDS);
    data["id"] = 1;
    CHECK(VaultReader(data).read() == PARSE_ERROR_NON_SEQUENTIAL_RECORDS);
    data["id"] = 3;
    data["layout"][0] = "bad";
    VaultReader reader(data);
    CHECK(reader.read() != PARSE_ERROR_NONE);
    CHECK(error_idx == 2);
    data["layout"][0] = "good";
    CHECK(reader.read() == PARSE_ERROR_NONE);
    CHECK_FALSE(reader.error().has_value());
    CHECK(error_idx == 3);
}

TEST_CASE("VaultReader ignores a monster token which is not a usable monrace id")
{
    const auto restore = preserve_vaults();
    vaults_info.clear();
    error_idx = -1;
    MonraceList::get_instance().emplace(i2enum<MonraceId>(5));
    // Every token below is silently ignored: the record still loads, but the symbol
    // gets no entry in place_monster_list.
    for (const auto *const monster : { "5abc", "99999", "-5", "6", "NO-SUCH-TAG", " 5", "+5" }) {
        CAPTURE(monster);
        auto data = make_vault();
        data["features"] = { { { "symbol", "G" }, { "feat", 1 }, { "monster", monster } } };
        vaults_info.clear();
        error_idx = -1;
        REQUIRE(VaultReader(data).read() == PARSE_ERROR_NONE);
        const auto &vault = vaults_info.at(0);
        CHECK(vault.feature_list.at('G') == 1);
        CHECK(vault.place_monster_list.count('G') == 0);
    }
}
