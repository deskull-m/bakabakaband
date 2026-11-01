/*!
 * @brief 定義ファイル読込の共通ユーティリティのテスト
 *
 * 定義ファイル中の文字列を定数へ変換する info_get_const / info_grab_one_const、
 * 数値文字列を格納する info_set_value、英語版のフレーバーテキストを連結する
 * append_english_text を検証する。
 * JSON から値を取り出す info_set_* 群は test-json-reader-util.cpp で扱う。
 *
 * 同じヘッダで宣言されている grab_one_activation_flag 自体は対象外とした。
 * 未知のトークンに対する挙動が、非数値なら std::stoi が例外を送出し、"0" や負数なら
 * msg_format (ターミナルが必要) を呼ぶという状態で、現状を仕様として固定したくないため。
 * ただし、その grab_one_activation_flag を安全に包んで値域・型を検証する read_activation は
 * 例外もターミナル依存も持たないため、本ファイルで検証する。
 */

#include "info-reader/info-reader-util.h"

#include "artifact/random-art-effects.h"
#include "info-reader/activation-reader.h"
#include "info-reader/parse-error-types.h"
#include "object-enchant/activation-info-table.h"
#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

namespace {

//! 変換対象の定数。実際の定義ファイルと同じく、値が連番でない場合も含める
enum class TestFlag {
    FIRST = 1,
    SECOND = 2,
    LARGE = 0x40000000,
};

//! 実際のトークン辞書 (f_info_flags など) と同じ形の辞書
const std::unordered_map<std::string_view, TestFlag> TEST_FLAGS = {
    { "FIRST", TestFlag::FIRST },
    { "SECOND", TestFlag::SECOND },
    { "LARGE", TestFlag::LARGE },
};

}

TEST_CASE("info_get_const returns the constant of the token")
{
    CHECK(info_get_const(TEST_FLAGS, "FIRST") == TestFlag::FIRST);
    CHECK(info_get_const(TEST_FLAGS, "SECOND") == TestFlag::SECOND);
}

TEST_CASE("info_get_const returns nullopt for an unknown token")
{
    CHECK_FALSE(info_get_const(TEST_FLAGS, "UNKNOWN").has_value());

    // 大文字小文字は区別され、空文字列も見つからない扱いになる
    CHECK_FALSE(info_get_const(TEST_FLAGS, "first").has_value());
    CHECK_FALSE(info_get_const(TEST_FLAGS, "").has_value());
}

TEST_CASE("info_get_const accepts any dictionary type indexed by the key")
{
    // std::string をキーとする std::map でも同じように引ける
    const std::map<std::string, int> dict = { { "ONE", 1 }, { "TWO", 2 } };

    CHECK(info_get_const(dict, "ONE") == 1);
    CHECK(info_get_const(dict, std::string("TWO")) == 2);
    CHECK_FALSE(info_get_const(dict, "THREE").has_value());
}

TEST_CASE("info_grab_one_const stores the constant only when the token is found")
{
    constexpr uint32_t initial_value = 0xdeadbeef;
    auto buf = initial_value;

    SUBCASE("known token")
    {
        CHECK(info_grab_one_const(buf, TEST_FLAGS, "SECOND"));
        CHECK(buf == static_cast<uint32_t>(TestFlag::SECOND));
    }

    SUBCASE("value which uses the upper bits")
    {
        CHECK(info_grab_one_const(buf, TEST_FLAGS, "LARGE"));
        CHECK(buf == static_cast<uint32_t>(TestFlag::LARGE));
    }

    SUBCASE("unknown token leaves the buffer as it is")
    {
        CHECK_FALSE(info_grab_one_const(buf, TEST_FLAGS, "UNKNOWN"));
        CHECK(buf == initial_value);
    }
}

TEST_CASE("info_set_value converts the string into a number")
{
    SUBCASE("decimal")
    {
        int value = 0;
        info_set_value(value, "100");
        CHECK(value == 100);

        info_set_value(value, "-100");
        CHECK(value == -100);
    }

    SUBCASE("hexadecimal")
    {
        int value = 0;
        info_set_value(value, "ff", 16);
        CHECK(value == 255);
    }

    SUBCASE("value which fits is converted into the type of the destination")
    {
        // 定義ファイル中の値は std::stoi で int として読んでから格納先の型へ変換される。
        // 格納先の型に収まらない値 (uint8_t への 300 など) は static_cast で黙って折り返されるが、
        // それは実装側の課題であり、現状を仕様として固定しないためここでは扱わない
        short short_value = 0;
        info_set_value(short_value, "30000");
        CHECK(short_value == 30000);

        char char_value = 0;
        info_set_value(char_value, "65");
        CHECK(char_value == 'A');
    }

    SUBCASE("leading spaces are skipped")
    {
        int value = 0;
        info_set_value(value, "  42");
        CHECK(value == 42);
    }
}

TEST_CASE("info_set_value throws an exception for a string which is not a number")
{
    // std::stoi の例外はそのまま呼び出し元へ伝わる (info_set_value は捕捉しない)
    int value = 0;
    CHECK_THROWS_AS(info_set_value(value, "abc"), std::invalid_argument);
    CHECK_THROWS_AS(info_set_value(value, ""), std::invalid_argument);
    CHECK_THROWS_AS(info_set_value(value, "99999999999999999999"), std::out_of_range);
    CHECK(value == 0);
}

TEST_CASE("read_activation treats absent or null as no activation")
{
    // 項目が省略された場合は get_json_value が null を返すため、null は「発動なし」として NONE を返す
    CHECK(read_activation(nlohmann::json()) == RandomArtActType::NONE);
    CHECK(read_activation(nlohmann::json(nullptr)) == RandomArtActType::NONE);
}

TEST_CASE("read_activation rejects a non-string value with INVALID_TYPE")
{
    for (const auto &value : { nlohmann::json(1), nlohmann::json(true), nlohmann::json::array(), nlohmann::json::object() }) {
        const auto result = read_activation(value);
        REQUIRE_FALSE(result);
        CHECK(result.error() == ParseErrorType::INVALID_TYPE);
    }
}

TEST_CASE("read_activation rejects an unknown name with INVALID_FLAG")
{
    // bakaba の grab_one_activation_flag は未知名でもメッセージを出さず NONE を返すため、
    // read_activation はターミナル非依存で INVALID_FLAG を返す
    const auto result = read_activation(nlohmann::json("UNKNOWN_ACTIVATION"));
    REQUIRE_FALSE(result);
    CHECK(result.error() == ParseErrorType::INVALID_FLAG);
}

TEST_CASE("read_activation accepts every registered activation name")
{
    for (const auto &activation : activation_info) {
        CAPTURE(activation.flag);
        REQUIRE(activation.index > RandomArtActType::NONE);
        CHECK(read_activation(nlohmann::json(activation.flag)) == activation.index);
    }
}

#ifndef JP
TEST_CASE("append_english_text joins the texts with a space")
{
    std::string text;

    SUBCASE("the first text is appended as it is")
    {
        append_english_text(text, "The first sentence");
        CHECK(text == "The first sentence");
    }

    SUBCASE("a single space is put between the texts")
    {
        append_english_text(text, "The first line");
        append_english_text(text, "the second line");
        CHECK(text == "The first line the second line");
    }

    SUBCASE("two spaces are put after the end of a sentence")
    {
        for (const auto *const eos : { "It ends here.", "It ends here!", "It ends here?" }) {
            CAPTURE(eos);
            std::string sentences;
            append_english_text(sentences, eos);
            append_english_text(sentences, "The next sentence");
            CHECK(sentences == std::string(eos) + "  The next sentence");
        }
    }

    SUBCASE("both ends of the appended text are trimmed")
    {
        append_english_text(text, "  The first line \t");
        append_english_text(text, "\t the second line  ");
        CHECK(text == "The first line the second line");
    }
}

TEST_CASE("append_english_text ignores an empty text")
{
    std::string text;

    // 空文字列や空白のみの行を連結しても、末尾に空白が残ってはいけない
    append_english_text(text, "");
    CHECK(text.empty());

    append_english_text(text, "   \t ");
    CHECK(text.empty());

    append_english_text(text, "The first line");
    append_english_text(text, "");
    append_english_text(text, "   ");
    CHECK(text == "The first line");
}
#endif
