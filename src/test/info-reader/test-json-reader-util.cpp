/*!
 * @brief 定義ファイル(JSON)読込の共通ユーティリティのテスト
 *
 * 各リーダーがJSONから値を取り出すのに使う get_json_value / info_set_* を検証する。
 * ここが誤ると全ての定義ファイルの読込に影響するため、正常系だけでなく
 * キー欠落・型不一致・範囲外のときに返すエラーコードと、
 * そのときに格納先の変数を書き換えないことも確かめる。
 *
 * @details 検証対象の関数呼び出しを `== 期待値` と組み合わせて CHECK に直接書くと、
 * MSVCが評価順序に関する警告 (C4866) を出すことがある。この警告はエラーとして
 * 扱われるため、呼び出しの結果は一旦変数で受けてから比較する。
 * (関数呼び出しを CHECK に直接書くこと自体が警告になるわけではない。
 *  test-info-reader-util.cpp では直接書いて警告なくビルドできている)
 */

#include "info-reader/json-reader-util.h"

#include "info-reader/parse-error-types.h"
#include "info-reader/symbol-reader-util.h"
#include "term/gameterm.h"
#include "util/dice.h"

#ifdef JP
#include "locale/character-encoding.h"
#endif

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

#include <tl/expected.hpp>

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace {

/*!
 * @brief 解析結果をエラーコードとして比べられるようにする
 * @param result info_set_* などの解析結果
 * @return 成功時はParseErrorType::NONE、失敗時は解析エラーコード
 */
ParseErrorType to_error_code(const tl::expected<void, ParseErrorType> &result)
{
    return result ? ParseErrorType::NONE : result.error();
}

}

TEST_CASE("JSON definition roots require an object containing the named array")
{
    const std::vector<nlohmann::json> invalid_roots = {
        nullptr,
        true,
        1,
        "vaults",
        nlohmann::json::array(),
        nlohmann::json::object(),
        { { "other", nlohmann::json::array() } },
        { { "vaults", nullptr } },
        { { "vaults", 1 } },
        { { "vaults", "bad" } },
        { { "vaults", nlohmann::json::object() } },
    };
    for (const auto &root : invalid_roots) {
        const auto saved = root;
        const auto result = info_validate_json_array(root, "vaults");
        CHECK(to_error_code(result) != ParseErrorType::NONE);
        CHECK(root == saved);
    }
}

TEST_CASE("Empty definition arrays are rejected only when requested")
{
    const nlohmann::json empty = { { "vaults", nlohmann::json::array() } };
    const auto default_result = info_validate_json_array(empty, "vaults");
    const auto vault_result = info_validate_json_array(empty, "vaults", false);
    CHECK(to_error_code(default_result) == ParseErrorType::NONE);
    CHECK(to_error_code(vault_result) == ParseErrorType::INVALID_VALUE);
    const nlohmann::json nonempty = { { "vaults", { { { "id", 0 } } } } };
    const auto valid_result = info_validate_json_array(nonempty, "vaults", false);
    CHECK(to_error_code(valid_result) == ParseErrorType::NONE);
    const auto missing_result = info_validate_json_array(nonempty, "missing", false);
    CHECK(to_error_code(missing_result) == ParseErrorType::TOO_FEW_ARGUMENTS);
}

#if defined(JP) && defined(EUC)
// _LIBICONV_VERSION (GNU libiconv の判定に使う) を参照するため
#include <iconv.h>
#endif

namespace {

//! info_set_integer がenum型を扱えることを確かめるためのenum
enum class TestKind : int {
    NONE = 0,
    FIRST = 1,
    LAST = 10,
};

enum class TestNarrowKind : std::uint8_t {
    FIRST = 1,
};

//! 実行中のビルドで info_set_string が読むキーと、読まない方のキー
#ifdef JP
constexpr auto LANG_KEY = "ja";
constexpr auto OTHER_LANG_KEY = "en";
#else
constexpr auto LANG_KEY = "en";
constexpr auto OTHER_LANG_KEY = "ja";
#endif

//! 値が格納されなかったことを確認するための、テスト内では現れない番兵値
constexpr auto SENTINEL = -12345;

#ifdef JP
//! 文字コード変換のテストに使うUTF-8のバイト列 ("テスト")。
//! ソースの文字コード変換の影響を受けないよう、エスケープでバイト列を直接書く。
//! const char* にすると失敗時にポインタ値が表示されるため std::string で持つ
const std::string UTF8_TEST_STRING = "\xe3\x83\x86\xe3\x82\xb9\xe3\x83\x88";
#endif

}

TEST_CASE("get_json_value returns the value of the key")
{
    const auto json = nlohmann::json::parse(R"({ "level": 5, "nested": { "inner": 1 } })");

    SUBCASE("existing key")
    {
        const auto &value = get_json_value(json, "level");
        REQUIRE(value.is_number_integer());
        const auto level = value.get<int>();
        CHECK(level == 5);
    }

    SUBCASE("nested object is returned as it is")
    {
        const auto &nested = get_json_value(json, "nested");
        REQUIRE(nested.is_object());
        const auto &inner = get_json_value(nested, "inner");
        const auto inner_value = inner.get<int>();
        CHECK(inner_value == 1);
    }

    SUBCASE("missing key")
    {
        const auto &value = get_json_value(json, "unknown");
        CHECK(value.is_null());
    }
}

TEST_CASE("get_json_value returns null for a key whose value is null")
{
    // 定義ファイルにnullと書かれたキーは、書かれていないキーと同じ扱いになる。
    // info_set_* はいずれもこの性質に依存している
    const auto json = nlohmann::json::parse(R"({ "key": null })");

    const auto &value = get_json_value(json, "key");
    CHECK(value.is_null());
}

TEST_CASE("get_json_value returns null for a non-object JSON")
{
    // 定義ファイル側の記述が誤っていても、キー取得は例外ではなくnullになる
    for (const auto *const json_str : { "[ 1, 2, 3 ]", R"("string")", "42", "null" }) {
        CAPTURE(json_str);

        const auto json = nlohmann::json::parse(json_str);
        const auto &value = get_json_value(json, "key");
        CHECK(value.is_null());
    }
}

TEST_CASE("info_set_integer stores the value")
{
    auto data = SENTINEL;

    SUBCASE("positive value")
    {
        const auto result = info_set_integer(nlohmann::json(100), data, true);
        CHECK(to_error_code(result) == ParseErrorType::NONE);
        CHECK(data == 100);
    }

    SUBCASE("negative value")
    {
        const auto result = info_set_integer(nlohmann::json(-100), data, true);
        CHECK(to_error_code(result) == ParseErrorType::NONE);
        CHECK(data == -100);
    }

    SUBCASE("optional key is stored as well")
    {
        // 必須でないキーでも、値が書かれていれば格納する
        const auto result = info_set_integer(nlohmann::json(100), data, false);
        CHECK(to_error_code(result) == ParseErrorType::NONE);
        CHECK(data == 100);
    }

    SUBCASE("value stored into a short")
    {
        short short_data = 0;
        const auto result = info_set_integer(nlohmann::json(30000), short_data, true);
        CHECK(to_error_code(result) == ParseErrorType::NONE);
        CHECK(short_data == 30000);
    }

    SUBCASE("enum value")
    {
        auto kind = TestKind::NONE;
        const auto result = info_set_integer(nlohmann::json(1), kind, true);
        CHECK(to_error_code(result) == ParseErrorType::NONE);
        CHECK(kind == TestKind::FIRST);
    }

    SUBCASE("enum underlying type is checked before conversion")
    {
        auto kind = TestNarrowKind::FIRST;
        CHECK(to_error_code(info_set_integer(nlohmann::json(257), kind, true)) == ParseErrorType::INVALID_VALUE);
        CHECK(kind == TestNarrowKind::FIRST);
        CHECK(to_error_code(info_set_integer(nlohmann::json(-1), kind, true)) == ParseErrorType::INVALID_VALUE);
        CHECK(kind == TestNarrowKind::FIRST);
    }

    SUBCASE("no range still checks the destination type")
    {
        std::uint8_t narrow_data = 123;
        CHECK(to_error_code(info_set_integer(nlohmann::json(300), narrow_data, true)) == ParseErrorType::INVALID_VALUE);
        CHECK(narrow_data == 123);
        CHECK(to_error_code(info_set_integer(nlohmann::json(-1), narrow_data, false)) == ParseErrorType::INVALID_VALUE);
        CHECK(narrow_data == 123);
        CHECK(to_error_code(info_set_integer(nlohmann::json(255), narrow_data, true)) == ParseErrorType::NONE);
        CHECK(narrow_data == 255);
        CHECK(to_error_code(info_set_integer(nlohmann::json(256), narrow_data, true, Range(0, 9999))) == ParseErrorType::INVALID_VALUE);
        CHECK(narrow_data == 255);
    }
}

TEST_CASE("info_set_integer treats a null JSON as an unwritten key")
{
    auto data = SENTINEL;
    const nlohmann::json null_json;

    SUBCASE("required key is an error")
    {
        const auto result = info_set_integer(null_json, data, true);
        CHECK(to_error_code(result) == ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    SUBCASE("optional key is not an error")
    {
        const auto result = info_set_integer(null_json, data, false);
        CHECK(to_error_code(result) == ParseErrorType::NONE);
    }

    // どちらの場合も格納先は元の値のままにしておく必要がある
    CHECK(data == SENTINEL);
}

TEST_CASE("info_set_integer rejects a non-integer JSON")
{
    auto data = SENTINEL;

    SUBCASE("string")
    {
        const auto result = info_set_integer(nlohmann::json("100"), data, true);
        CHECK(to_error_code(result) == ParseErrorType::INVALID_TYPE);
    }

    SUBCASE("floating point number")
    {
        const auto result = info_set_integer(nlohmann::json(1.5), data, true);
        CHECK(to_error_code(result) == ParseErrorType::INVALID_TYPE);
    }

    SUBCASE("boolean")
    {
        const auto result = info_set_integer(nlohmann::json(true), data, true);
        CHECK(to_error_code(result) == ParseErrorType::INVALID_TYPE);
    }

    SUBCASE("optional key is an error too")
    {
        // 必須でなくても、書かれている値の型が誤っていれば見逃さない
        const auto result = info_set_integer(nlohmann::json("100"), data, false);
        CHECK(to_error_code(result) == ParseErrorType::INVALID_TYPE);
    }

    CHECK(data == SENTINEL);
}

TEST_CASE("info_set_integer checks the value range")
{
    auto data = SENTINEL;

    SUBCASE("both ends of the range are valid")
    {
        const auto lower_result = info_set_integer(nlohmann::json(0), data, true, Range(0, 128));
        CHECK(to_error_code(lower_result) == ParseErrorType::NONE);
        CHECK(data == 0);

        const auto upper_result = info_set_integer(nlohmann::json(128), data, true, Range(0, 128));
        CHECK(to_error_code(upper_result) == ParseErrorType::NONE);
        CHECK(data == 128);
    }

    SUBCASE("below the range")
    {
        const auto result = info_set_integer(nlohmann::json(-1), data, true, Range(0, 128));
        CHECK(to_error_code(result) == ParseErrorType::INVALID_VALUE);
        CHECK(data == SENTINEL);
    }

    SUBCASE("above the range")
    {
        const auto result = info_set_integer(nlohmann::json(129), data, true, Range(0, 128));
        CHECK(to_error_code(result) == ParseErrorType::INVALID_VALUE);
        CHECK(data == SENTINEL);
    }

    SUBCASE("range containing negative numbers")
    {
        const auto valid_result = info_set_integer(nlohmann::json(-99), data, true, Range(-99, 99));
        CHECK(to_error_code(valid_result) == ParseErrorType::NONE);
        CHECK(data == -99);

        const auto invalid_result = info_set_integer(nlohmann::json(-100), data, true, Range(-99, 99));
        CHECK(to_error_code(invalid_result) == ParseErrorType::INVALID_VALUE);
        CHECK(data == -99);
    }

    SUBCASE("enum value out of the range")
    {
        auto kind = TestKind::NONE;
        const auto invalid_result = info_set_integer(nlohmann::json(11), kind, true, Range(0, 10));
        CHECK(to_error_code(invalid_result) == ParseErrorType::INVALID_VALUE);
        CHECK(kind == TestKind::NONE);

        const auto valid_result = info_set_integer(nlohmann::json(10), kind, true, Range(0, 10));
        CHECK(to_error_code(valid_result) == ParseErrorType::NONE);
        CHECK(kind == TestKind::LAST);
    }

    SUBCASE("value which does not fit in the destination type")
    {
        // 範囲外の値が格納先の型で表現できない場合も範囲チェックで弾く。
        // 格納先の型へ変換してから範囲を見ると、uint8_t への 300 は 44 に切り詰められて
        // Range(0, 255) を通ってしまう (terrain の "power" がこの形で読まれている)。
        // 番兵値は切り詰め後の値 (44, 255) と重ならないものにする
        constexpr uint8_t uint8_sentinel = 123;
        auto uint8_data = uint8_sentinel;

        const auto above_result = info_set_integer(nlohmann::json(300), uint8_data, true, Range(0, 255));
        CHECK(to_error_code(above_result) == ParseErrorType::INVALID_VALUE);
        CHECK(uint8_data == uint8_sentinel);

        const auto below_result = info_set_integer(nlohmann::json(-1), uint8_data, true, Range(0, 255));
        CHECK(to_error_code(below_result) == ParseErrorType::INVALID_VALUE);
        CHECK(uint8_data == uint8_sentinel);

        const auto valid_result = info_set_integer(nlohmann::json(255), uint8_data, true, Range(0, 255));
        CHECK(to_error_code(valid_result) == ParseErrorType::NONE);
        CHECK(uint8_data == 255);
    }
}

TEST_CASE("info_set_integer checks signed int boundaries before conversion")
{
    constexpr auto minimum = std::numeric_limits<int>::min();
    constexpr auto maximum = std::numeric_limits<int>::max();
    auto data = SENTINEL;

    const auto minimum_result = info_set_integer(nlohmann::json(minimum), data, true, Range(minimum, maximum));
    CHECK(to_error_code(minimum_result) == ParseErrorType::NONE);
    CHECK(data == minimum);
    const auto maximum_result = info_set_integer(nlohmann::json(maximum), data, true, Range(minimum, maximum));
    CHECK(to_error_code(maximum_result) == ParseErrorType::NONE);
    CHECK(data == maximum);

    const auto below = static_cast<std::int64_t>(minimum) - 1;
    const auto above = static_cast<std::int64_t>(maximum) + 1;
    const auto below_result = info_set_integer(nlohmann::json(below), data, true, Range(minimum, maximum));
    CHECK(to_error_code(below_result) == ParseErrorType::INVALID_VALUE);
    CHECK(data == maximum);
    const auto above_result = info_set_integer(nlohmann::json(above), data, true, Range(minimum, maximum));
    CHECK(to_error_code(above_result) == ParseErrorType::INVALID_VALUE);
    CHECK(data == maximum);
}

TEST_CASE("info_set_integer checks unsigned JSON values and numeric types")
{
    auto data = SENTINEL;
    constexpr auto maximum = std::numeric_limits<int>::max();
    const auto unsigned_maximum = nlohmann::json(static_cast<std::uint64_t>(maximum));
    const auto unsigned_above = nlohmann::json(static_cast<std::uint64_t>(maximum) + 1);

    const auto maximum_result = info_set_integer(unsigned_maximum, data, true, Range(0, maximum));
    CHECK(to_error_code(maximum_result) == ParseErrorType::NONE);
    CHECK(data == maximum);
    const auto above_result = info_set_integer(unsigned_above, data, true, Range(0, maximum));
    CHECK(to_error_code(above_result) == ParseErrorType::INVALID_VALUE);
    CHECK(data == maximum);
    const auto type_result = info_set_integer(nlohmann::json(1.0), data, true, Range(0, maximum));
    CHECK(to_error_code(type_result) == ParseErrorType::INVALID_TYPE);
    CHECK(data == maximum);
}

TEST_CASE("info_set_integer checks large unsigned JSON range and storage")
{
    const auto largest = std::numeric_limits<std::uint64_t>::max();
    const auto json = nlohmann::json::parse("18446744073709551615");
    REQUIRE(json.is_number_unsigned());
    auto data = SENTINEL;

    const auto small_range_result = info_set_integer(json, data, true, Range(-1, 1));
    CHECK(to_error_code(small_range_result) == ParseErrorType::INVALID_VALUE);
    CHECK(data == SENTINEL);
    const auto int_range_result = info_set_integer(json, data, false, Range(std::numeric_limits<int>::min(), std::numeric_limits<int>::max()));
    CHECK(to_error_code(int_range_result) == ParseErrorType::INVALID_VALUE);
    CHECK(data == SENTINEL);

    std::uint64_t unsigned_data = 0;
    const auto unsigned_result = info_set_integer(json, unsigned_data, true);
    CHECK(to_error_code(unsigned_result) == ParseErrorType::NONE);
    CHECK(unsigned_data == largest);
}

TEST_CASE("info_set_string picks the string of the language of the build")
{
    nlohmann::json json;
    json[LANG_KEY] = "name-of-this-build";
    json[OTHER_LANG_KEY] = "name-of-the-other-build";

    std::string data;
    const auto result = info_set_string(json, data, true);
    CHECK(to_error_code(result) == ParseErrorType::NONE);
    CHECK(data == "name-of-this-build");
}

TEST_CASE("info_set_string ignores malformed values of the other language")
{
    const std::vector<nlohmann::json> ignored_values = {
        nullptr,
        true,
        100,
        nlohmann::json::array(),
        nlohmann::json::object(),
        std::string("\xff"),
        std::string("a\0b", 3),
    };
    for (const auto &ignored : ignored_values) {
        for (const auto is_required : { false, true }) {
            nlohmann::json json = { { LANG_KEY, "selected" }, { OTHER_LANG_KEY, ignored } };
            std::string data = "unchanged";
            CHECK(to_error_code(info_set_string(json, data, is_required)) == ParseErrorType::NONE);
            CHECK(data == "selected");

            json.erase(LANG_KEY);
            data = "unchanged";
            CHECK(to_error_code(info_set_string(json, data, is_required)) == (is_required ? ParseErrorType::TOO_FEW_ARGUMENTS : ParseErrorType::NONE));
            CHECK(data == "unchanged");
        }
    }
}

TEST_CASE("info_set_string stores an empty selected string")
{
    for (const auto is_required : { false, true }) {
        const nlohmann::json json = { { LANG_KEY, "" }, { OTHER_LANG_KEY, nullptr } };
        std::string data = "unchanged";
        CHECK(to_error_code(info_set_string(json, data, is_required)) == ParseErrorType::NONE);
        CHECK(data.empty());
    }
}

TEST_CASE("info_set_string preserves the language-specific embedded NUL behavior")
{
    const std::string value("a\0b", 3);
    const nlohmann::json json = { { LANG_KEY, value } };
    for (const auto is_required : { false, true }) {
        std::string data = "unchanged";
#ifdef JP
        // 埋め込みNULの扱いは変換層 (utf8_to_sys) 依存で、bakaba のシステム文字コード変換は
        // NUL で打ち切って成功するなど環境により結果が異なる。consolidation がこの既存挙動を
        // 変えないことを見るため、期待値は utf8_to_sys から求める (変換失敗は INVALID_VALUE)
        const auto converted = utf8_to_sys(value);
        CHECK(to_error_code(info_set_string(json, data, is_required)) == (converted ? ParseErrorType::NONE : ParseErrorType::INVALID_VALUE));
        CHECK(data == (converted ? *converted : "unchanged"));
#else
        CHECK(to_error_code(info_set_string(json, data, is_required)) == ParseErrorType::NONE);
        CHECK(data == value);
#endif
    }
}

TEST_CASE("info_set_string leaves malformed UTF-8 handling to the existing conversion")
{
    const std::string value("\xff");
    const nlohmann::json json = { { LANG_KEY, value } };
    for (const auto is_required : { false, true }) {
        std::string data = "unchanged";
#ifdef JP
        // Windows may replace invalid UTF-8, while iconv can reject it. Do not add a stricter policy here.
        const auto converted = utf8_to_sys(value);
        CHECK(to_error_code(info_set_string(json, data, is_required)) == (converted ? ParseErrorType::NONE : ParseErrorType::INVALID_VALUE));
        CHECK(data == (converted ? *converted : "unchanged"));
#else
        CHECK(to_error_code(info_set_string(json, data, is_required)) == ParseErrorType::NONE);
        CHECK(data == value);
#endif
    }
}

TEST_CASE("info_set_string rejects all non-string selected values without changing output")
{
    const std::vector<nlohmann::json> invalid_values = {
        nullptr,
        true,
        100,
        1.5,
        nlohmann::json::array(),
        nlohmann::json::object(),
    };
    for (const auto &value : invalid_values) {
        for (const auto is_required : { false, true }) {
            const nlohmann::json json = { { LANG_KEY, value }, { OTHER_LANG_KEY, "other" } };
            std::string data = "unchanged";
            CHECK(to_error_code(info_set_string(json, data, is_required)) == ParseErrorType::INVALID_TYPE);
            CHECK(data == "unchanged");
        }
    }
}

#ifdef JP
TEST_CASE("info_set_string converts the UTF-8 string into the system encoding")
{
    nlohmann::json json;
    json["ja"] = UTF8_TEST_STRING;

    std::string data;
    const auto result = info_set_string(json, data, true);
    REQUIRE(to_error_code(result) == ParseErrorType::NONE);

    // 日本語版のシステム文字コード(EUC-JP/Shift_JIS)はUTF-8と異なるので、
    // 変換されていれば元のバイト列とは一致しない
    CHECK(data != UTF8_TEST_STRING);

    // 変換結果の内容まで確かめる。期待するバイト列はEUC-JPとShift_JISで異なるため、
    // UTF-8へ戻して元の文字列と一致することを見る
    const auto restored = sys_to_utf8(data);
    REQUIRE(restored.has_value());
    CHECK(*restored == UTF8_TEST_STRING);
}
#endif

#if defined(JP) && defined(EUC) && (defined(__GLIBC__) || defined(_LIBICONV_VERSION))
TEST_CASE("info_set_string cannot convert a character missing from EUC-JP")
{
    // U+1F600 (GRINNING FACE)。EUC-JPに対応する文字がないため変換に失敗する。
    // Windows版(Shift_JIS)は MultiByteToWideChar/WideCharToMultiByte をエラー指定なしで
    // 呼ぶので置換文字になって成功してしまう。そのためEUC版限定のテストとする。
    // さらに、変換不能文字でiconvがエラーを返すのは glibc/GNU libiconv での挙動で、
    // musl のiconvは '*' に置換して成功扱いにする。そのため glibc (__GLIBC__) か
    // GNU libiconv (_LIBICONV_VERSION) のときに限定し、musl 系ではこのテストごと除外する
    nlohmann::json json;
    json["ja"] = "\xf0\x9f\x98\x80";

    std::string data = "unchanged";

    // 変換失敗は必須かどうかによらずエラーになり、格納先は書き換えない
    const auto required_result = info_set_string(json, data, true);
    CHECK(to_error_code(required_result) == ParseErrorType::INVALID_VALUE);

    const auto optional_result = info_set_string(json, data, false);
    CHECK(to_error_code(optional_result) == ParseErrorType::INVALID_VALUE);

    CHECK(data == "unchanged");

    // 変換に失敗しても後続の変換に影響しないことを確かめる。
    // utf8_to_euc の iconv_t は関数内staticで、変換失敗後にリセットされないため
    nlohmann::json valid_json;
    valid_json["ja"] = UTF8_TEST_STRING;

    const auto result_after_failure = info_set_string(valid_json, data, true);
    REQUIRE(to_error_code(result_after_failure) == ParseErrorType::NONE);
    const auto restored = sys_to_utf8(data);
    REQUIRE(restored.has_value());
    CHECK(*restored == UTF8_TEST_STRING);
}
#endif

TEST_CASE("info_set_string treats a missing string as an unwritten key")
{
    std::string data = "unchanged";

    SUBCASE("null JSON")
    {
        const nlohmann::json null_json;

        const auto required_result = info_set_string(null_json, data, true);
        CHECK(to_error_code(required_result) == ParseErrorType::TOO_FEW_ARGUMENTS);

        const auto optional_result = info_set_string(null_json, data, false);
        CHECK(to_error_code(optional_result) == ParseErrorType::NONE);
    }

    SUBCASE("object which has only the key of the other language")
    {
        nlohmann::json json;
        json[OTHER_LANG_KEY] = "name-of-the-other-build";

        const auto required_result = info_set_string(json, data, true);
        CHECK(to_error_code(required_result) == ParseErrorType::TOO_FEW_ARGUMENTS);

        const auto optional_result = info_set_string(json, data, false);
        CHECK(to_error_code(optional_result) == ParseErrorType::NONE);
    }

    CHECK(data == "unchanged");
}

TEST_CASE("info_set_string rejects a JSON of an invalid type")
{
    std::string data = "unchanged";

    SUBCASE("string written directly instead of an object")
    {
        // 言語別のオブジェクトではなく文字列が直接書かれている場合
        const auto required_result = info_set_string(nlohmann::json("name"), data, true);
        CHECK(to_error_code(required_result) == ParseErrorType::INVALID_TYPE);

        const auto optional_result = info_set_string(nlohmann::json("name"), data, false);
        CHECK(to_error_code(optional_result) == ParseErrorType::INVALID_TYPE);
    }

    SUBCASE("the value of the language key is not a string")
    {
        nlohmann::json json;
        json[LANG_KEY] = 100;

        const auto required_result = info_set_string(json, data, true);
        CHECK(to_error_code(required_result) == ParseErrorType::INVALID_TYPE);

        const auto optional_result = info_set_string(json, data, false);
        CHECK(to_error_code(optional_result) == ParseErrorType::INVALID_TYPE);
    }

    CHECK(data == "unchanged");
}

TEST_CASE("info_set_dice parses the dice string")
{
    Dice dice;

    SUBCASE("single digit")
    {
        const auto result = info_set_dice(nlohmann::json("3d5"), dice, true);
        CHECK(to_error_code(result) == ParseErrorType::NONE);
        CHECK(dice == Dice(3, 5));
    }

    SUBCASE("multiple digits")
    {
        const auto result = info_set_dice(nlohmann::json("10d100"), dice, true);
        CHECK(to_error_code(result) == ParseErrorType::NONE);
        CHECK(dice == Dice(10, 100));
    }
}

TEST_CASE("info_set_dice treats a null JSON as an unwritten key")
{
    Dice dice(1, 1);
    const nlohmann::json null_json;

    const auto required_result = info_set_dice(null_json, dice, true);
    CHECK(to_error_code(required_result) == ParseErrorType::TOO_FEW_ARGUMENTS);

    const auto optional_result = info_set_dice(null_json, dice, false);
    CHECK(to_error_code(optional_result) == ParseErrorType::NONE);

    CHECK(dice == Dice(1, 1));
}

TEST_CASE("info_set_dice rejects an invalid dice notation")
{
    Dice dice(1, 1);

    SUBCASE("non-string JSON is a type error")
    {
        const auto required_result = info_set_dice(nlohmann::json(3), dice, true);
        CHECK(to_error_code(required_result) == ParseErrorType::INVALID_TYPE);

        const auto optional_result = info_set_dice(nlohmann::json(3), dice, false);
        CHECK(to_error_code(optional_result) == ParseErrorType::INVALID_TYPE);
    }

    SUBCASE("malformed dice string")
    {
        // Dice::parse が投げる例外は捕捉され、必須かどうかによらずエラーになる
        for (const auto *const dice_str : { "3", "3d", "dice", "" }) {
            CAPTURE(dice_str);

            const auto required_result = info_set_dice(nlohmann::json(dice_str), dice, true);
            CHECK(to_error_code(required_result) == ParseErrorType::INVALID_VALUE);

            const auto optional_result = info_set_dice(nlohmann::json(dice_str), dice, false);
            CHECK(to_error_code(optional_result) == ParseErrorType::INVALID_VALUE);
        }
    }

    CHECK(dice == Dice(1, 1));
}

TEST_CASE("info_set_bool stores the value")
{
    auto data = false;

    const auto true_result = info_set_bool(nlohmann::json(true), data, true);
    CHECK(to_error_code(true_result) == ParseErrorType::NONE);
    CHECK(data);

    const auto false_result = info_set_bool(nlohmann::json(false), data, true);
    CHECK(to_error_code(false_result) == ParseErrorType::NONE);
    CHECK_FALSE(data);
}

TEST_CASE("info_set_bool preserves null semantics and rejects other non-boolean types")
{
    // 格納先を書き換えないことを確かめるため、既定値の false ではなく true から始める。
    // false で始めると、誤って false を書き込む実装でもテストが通ってしまう
    auto data = true;

    SUBCASE("null JSON")
    {
        const nlohmann::json null_json;

        const auto required_result = info_set_bool(null_json, data, true);
        CHECK(to_error_code(required_result) == ParseErrorType::TOO_FEW_ARGUMENTS);

        const auto optional_result = info_set_bool(null_json, data, false);
        CHECK(to_error_code(optional_result) == ParseErrorType::NONE);
    }

    SUBCASE("JSON of another type")
    {
        // 必須・任意のどちらでも型不一致を報告し、格納先は変更しない。
        const std::vector<nlohmann::json> invalid_values = {
            "true",
            "false",
            1,
            1.5,
            nlohmann::json::array(),
            nlohmann::json::object(),
        };
        for (const auto &json : invalid_values) {
            CAPTURE(json.type_name());

            CHECK(to_error_code(info_set_bool(json, data, true)) == ParseErrorType::INVALID_TYPE);
            CHECK(data);
            CHECK(to_error_code(info_set_bool(json, data, false)) == ParseErrorType::INVALID_TYPE);
            CHECK(data);
        }
    }

    CHECK(data);
}

TEST_CASE("info_read_symbol preserves baseitem validation errors")
{
    struct InvalidSymbol {
        const char *json;
        ParseErrorType error;
    };
    const InvalidSymbol cases[] = {
        { R"(null)", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"(42)", ParseErrorType::INVALID_TYPE },
        { R"([])", ParseErrorType::INVALID_TYPE },
        { R"(true)", ParseErrorType::INVALID_TYPE },
        { R"("bad")", ParseErrorType::INVALID_TYPE },
        { R"({})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"character":null,"color":"White"})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"character":42,"color":"White"})", ParseErrorType::INVALID_TYPE },
        { R"({"character":true,"color":"White"})", ParseErrorType::INVALID_TYPE },
        { R"({"character":[],"color":"White"})", ParseErrorType::INVALID_TYPE },
        { R"({"character":{},"color":"White"})", ParseErrorType::INVALID_TYPE },
        { R"({"color":null,"character":"!"})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"color":42,"character":"!"})", ParseErrorType::INVALID_TYPE },
        { R"({"color":true,"character":"!"})", ParseErrorType::INVALID_TYPE },
        { R"({"color":[],"character":"!"})", ParseErrorType::INVALID_TYPE },
        { R"({"color":{},"character":"!"})", ParseErrorType::INVALID_TYPE },
        { R"({"character":"!"})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"color":"White"})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"character":"","color":"White"})", ParseErrorType::INVALID_VALUE },
        { R"({"character":"!!","color":"White"})", ParseErrorType::INVALID_VALUE },
        { R"({"character":"\u3042","color":"White"})", ParseErrorType::INVALID_VALUE },
        { R"({"character":"!","color":"UNKNOWN"})", ParseErrorType::INVALID_FLAG },
        { R"({"character":"","color":"UNKNOWN"})", ParseErrorType::INVALID_VALUE },
        { R"({"character":"","color":42})", ParseErrorType::INVALID_VALUE },
        { R"({"character":""})", ParseErrorType::INVALID_VALUE },
    };
    for (const auto &invalid : cases) {
        CAPTURE(invalid.json);
        const auto input = nlohmann::json::parse(invalid.json);
        const auto saved = input;
        const auto result = info_read_symbol(input, SymbolReadPolicy::TYPE_AND_CHARACTER_FIRST);
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == invalid.error);
        CHECK(input == saved);
    }
}

TEST_CASE("info_read_symbol preserves monster validation errors")
{
    struct InvalidSymbol {
        const char *json;
        ParseErrorType error;
    };
    const InvalidSymbol cases[] = {
        { R"(null)", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"(42)", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"([])", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"(true)", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"("bad")", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"character":null,"color":"White"})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"character":42,"color":"White"})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"character":true,"color":"White"})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"character":[],"color":"White"})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"character":{},"color":"White"})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"color":null,"character":"!"})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"color":42,"character":"!"})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"color":true,"character":"!"})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"color":[],"character":"!"})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"color":{},"character":"!"})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"character":"!"})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"color":"White"})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"character":"","color":"White"})", ParseErrorType::INVALID_VALUE },
        { R"({"character":"!!","color":"White"})", ParseErrorType::INVALID_VALUE },
        { R"({"character":"\u3042","color":"White"})", ParseErrorType::INVALID_VALUE },
        { R"({"character":"!","color":"UNKNOWN"})", ParseErrorType::INVALID_FLAG },
        { R"({"character":"","color":"UNKNOWN"})", ParseErrorType::INVALID_FLAG },
        { R"({"character":"","color":42})", ParseErrorType::TOO_FEW_ARGUMENTS },
        { R"({"character":""})", ParseErrorType::TOO_FEW_ARGUMENTS },
    };
    for (const auto &invalid : cases) {
        CAPTURE(invalid.json);
        const auto input = nlohmann::json::parse(invalid.json);
        const auto saved = input;
        const auto result = info_read_symbol(input, SymbolReadPolicy::MONSTER_COLOR_FIRST);
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == invalid.error);
        CHECK(input == saved);
    }
}

TEST_CASE("info_read_symbol accepts every single byte and color token")
{
    for (const auto policy : { SymbolReadPolicy::TYPE_AND_CHARACTER_FIRST, SymbolReadPolicy::MONSTER_COLOR_FIRST }) {
        for (const auto &[name, color] : color_list) {
            CHECK(color <= 15);
            for (int byte = 0; byte <= 255; ++byte) {
                CAPTURE(name);
                CAPTURE(byte);
                nlohmann::json input;
                input["character"] = std::string(1, static_cast<char>(byte));
                input["color"] = std::string(name);
                const auto saved = input;
                const auto result = info_read_symbol(input, policy);
                REQUIRE(result.has_value());
                CHECK(result->character == static_cast<char>(byte));
                CHECK(result->color == color);
                CHECK(input == saved);
            }
        }
    }
}
