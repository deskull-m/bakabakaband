/*!
 * @brief 固定クエスト JSONC 読み込みのマップ範囲検証テスト
 * @details hengband#5727 相当。マップ行数・行バイト幅がフロア上限を超える場合に
 * 読み込みを拒否することを確認する。bakaba の QuestReader は上流と異なり出力を
 * 直接書き込む (失敗時に旧出力を保持する staging を持たない) ため、各ケースは
 * 毎回新しい QuestFixedMap を用意してエラーコードと生成結果のみを検証する。
 */

#include "floor/floor-base-definitions.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/quest-reader.h"
#include "system/dungeon/quest-definition.h"
#include "system/dungeon/quest-fixed-map.h"
#include <doctest/doctest.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace {
/*!
 * @brief マップ検証まで到達できる最小限のクエスト定義を作る
 * @param line 説明文の行 (日本語・英語の両方に使う)
 */
nlohmann::json make_quest_with_description(const std::string &line)
{
    return {
        { "name", { { "ja", "Test" }, { "en", "Test" } } },
        { "definition", { { "type", "KILL_ALL" }, { "level", 5 } } },
        { "descriptions", { { { "text", { { "ja", { line } }, { "en", { line } } } } } } },
    };
}
}

TEST_CASE("QuestReader rejects map extent overflow")
{
    auto data = make_quest_with_description("New description");
    std::vector<std::string> rows;
    SUBCASE("Too many rows")
    {
        rows.assign(MAX_HGT + 1, ".");
    }
    SUBCASE("Too wide row")
    {
        rows = { ".", std::string(MAX_WID + 1, '.') };
    }
    SUBCASE("UTF-8 width is counted in bytes rather than code points")
    {
        std::string row;
        for (int i = 0; i <= MAX_WID / 2; ++i) {
            row += "\xc3\xa9";
        }
        REQUIRE(row.size() == MAX_WID + 2);
        rows = { std::move(row) };
    }
    for (const auto use_variants : { false, true }) {
        CAPTURE(use_variants);
        if (use_variants) {
            data.erase("map");
            data["mapVariants"] = { std::vector<std::string>{ "." }, rows };
        } else {
            data["map"] = rows;
        }
        QuestType quest;
        QuestFixedMap fixed_map;
        CHECK(QuestReader(data, quest, fixed_map).read() == ParseErrorType::OUT_OF_BOUNDS);
    }
}

TEST_CASE("QuestReader rejects map rows containing an embedded NUL")
{
    // parse_qtw_D() は行を C 文字列として扱うため、埋め込み NUL があるとそこで黙って
    // 切り詰められ、以降のセルが欠落する。読み込み時に不正値として弾くことを確認する。
    auto data = make_quest_with_description("New description");
    const std::vector<std::string> rows = { std::string("A\0B", 3) };
    for (const auto use_variants : { false, true }) {
        CAPTURE(use_variants);
        if (use_variants) {
            data.erase("map");
            data["mapVariants"] = { std::vector<std::string>{ "." }, rows };
        } else {
            data["map"] = rows;
        }
        QuestType quest;
        QuestFixedMap fixed_map;
        CHECK(QuestReader(data, quest, fixed_map).read() == ParseErrorType::INVALID_VALUE);
    }
}

TEST_CASE("QuestReader accepts map extents at the floor boundary")
{
    const std::vector<std::string> rows(MAX_HGT, std::string(MAX_WID, '.'));
    for (const auto use_variants : { false, true }) {
        CAPTURE(use_variants);
        auto data = make_quest_with_description("Description");
        if (use_variants) {
            data["mapVariants"] = { std::vector<std::string>{ "." }, rows };
        } else {
            data["map"] = rows;
        }
        QuestType quest;
        QuestFixedMap fixed_map;
        REQUIRE(QuestReader(data, quest, fixed_map).read() == ParseErrorType::NONE);
        REQUIRE(fixed_map.maps.size() == (use_variants ? 2 : 1));
        CHECK(fixed_map.maps.back() == rows);
    }
}

TEST_CASE("QuestReader preserves empty, ragged, and skipped map row behavior")
{
    auto data = make_quest_with_description("Description");
    const std::vector<std::string> ragged_rows = { "", "...", "." };
    data["mapVariants"] = { nlohmann::json::array(), { "", nullptr, "...", 7, "." } };
    QuestType quest;
    QuestFixedMap fixed_map;
    REQUIRE(QuestReader(data, quest, fixed_map).read() == ParseErrorType::NONE);
    CHECK(fixed_map.maps == std::vector<std::vector<std::string>>{ {}, ragged_rows });

    // map が存在すれば mapVariants は読まない。空の map も従来どおり保持する。
    QuestFixedMap fixed_map_empty;
    data["map"] = nlohmann::json::array();
    data["mapVariants"] = { { std::string(MAX_WID + 1, '.') } };
    REQUIRE(QuestReader(data, quest, fixed_map_empty).read() == ParseErrorType::NONE);
    CHECK(fixed_map_empty.maps == std::vector<std::vector<std::string>>{ {} });

    // 非文字列行は配置されないため、行数の上限に含めない。
    QuestFixedMap fixed_map_full;
    data["map"] = std::vector<std::string>(MAX_HGT, ".");
    data["map"].push_back(nullptr);
    REQUIRE(QuestReader(data, quest, fixed_map_full).read() == ParseErrorType::NONE);
    CHECK(fixed_map_full.maps[0].size() == MAX_HGT);
}

TEST_CASE("QuestReader checks every declared start against every map before publishing")
{
    // hengband#5774 相当。開始位置がマップ候補の範囲外なら読み込みを拒否する。
    // bakaba は失敗時の旧出力保持 (staging) を持たないため、エラーコードのみ検証する。
    const std::vector<nlohmann::json> invalid = {
        { { "start", { { "y", 0 }, { "x", 0 } } } }, // マップ無し
        { { "map", { "..." } }, { "start", { { "y", 1 }, { "x", 0 } } } }, // 行数超過
        { { "map", { "..." } }, { "start", { { "y", 0 }, { "x", 3 } } } }, // 行幅超過
        { { "mapVariants", { { "..." }, { "." } } }, { "start", { { "y", 0 }, { "x", 1 } } } }, // 一部のマップ候補で範囲外
        { { "map", { "..." } }, { "startVariants", { { { "y", 0 }, { "x", 0 } }, { { "leavingQuest", 49 }, { "y", 0 }, { "x", 3 } } } } }, // 後続バリアントが範囲外
    };
    for (const auto &fields : invalid) {
        CAPTURE(fields);
        auto data = make_quest_with_description("Description");
        data.update(fields);
        QuestType quest;
        QuestFixedMap fixed_map;
        CHECK(QuestReader(data, quest, fixed_map).read() == ParseErrorType::OUT_OF_BOUNDS);
    }
}

TEST_CASE("QuestReader accepts starts within every map candidate")
{
    auto data = make_quest_with_description("Description");
    data["mapVariants"] = { { ".....", "....." }, { "...", "...", "..." } };
    // 全候補で y<行数 かつ x<先頭行幅 を満たす座標 (y<=1, x<=2)
    data["startVariants"] = { { { "y", 0 }, { "x", 0 } }, { { "leavingQuest", 49 }, { "y", 1 }, { "x", 2 } } };
    QuestType quest;
    QuestFixedMap fixed_map;
    REQUIRE(QuestReader(data, quest, fixed_map).read() == ParseErrorType::NONE);
    REQUIRE(fixed_map.starts.size() == 2);
    CHECK(fixed_map.starts[1].leaving_quest == 49);
    CHECK(fixed_map.starts[1].y == 1);
    CHECK(fixed_map.starts[1].x == 2);
}
