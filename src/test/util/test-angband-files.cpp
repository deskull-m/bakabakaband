/*!
 * @brief path_build() のテスト
 *
 * Windows 日本語版ではファイル名が Shift_JIS のため、std::filesystem::path へ
 * 渡す前に CP932 → UTF-16 へ変換する。相対パスだけでなく、'\\' 始まりの
 * 早期 return 経路でも同じ変換が必要なことを検証する。
 *
 * 2バイト文字のテストデータは必ず16進エスケープで書くこと。
 *
 * angband_fgets() は、'\0' を含む行をその '\0' で切った1行として返すことを検証する。
 */

#include "util/angband-files.h"
#include "util/finalizer.h"
#include <cstdio>
#include <doctest/doctest.h>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#if defined(_WIN32) && defined(JP) && defined(SJIS)
namespace {
template <typename... Args>
std::string cat(const Args &...args)
{
    std::string result;
    (result.append(args), ...);
    return result;
}

constexpr std::string_view KANJI_NI = "\x93\xfa"; //!< 日
constexpr std::string_view KANJI_HON = "\x96\x7b"; //!< 本
constexpr std::string_view DAME_SO = "\x83\x5c"; //!< ソ (後半バイトが 0x5c)
constexpr std::string_view UTF8_LOOKALIKE = "\xe0\xa0\x81\xe3\x82\x81"; //!< 燿√ａ

}

TEST_CASE("path_build appends a Shift_JIS file name as UTF-16 on Windows")
{
    const auto file = cat(KANJI_NI, KANJI_HON);
    const auto built = path_build(std::filesystem::path(L"pref"), file);
    CHECK(built.filename().wstring() == L"\u65e5\u672c");
}

TEST_CASE("path_build converts a root-relative Shift_JIS path on the early-return path")
{
    const auto file = cat("\\", KANJI_NI, KANJI_HON, "\\file.prf");
    const auto built = path_build(std::filesystem::path(L"ignored"), file);
    CHECK(built.wstring() == L"\\\u65e5\u672c\\file.prf");
}

TEST_CASE("path_build converts a Shift_JIS path whose second byte is 0x5c")
{
    const auto file = cat(DAME_SO, ".prf");
    const auto built = path_build(std::filesystem::path(L"pref"), file);
    CHECK(built.filename().wstring() == L"\u30bd.prf");
}

TEST_CASE("path_build converts a Shift_JIS path that is also valid UTF-8")
{
    const auto built = path_build(std::filesystem::path(L"pref"), std::string(UTF8_LOOKALIKE));
    CHECK(built.filename().wstring() == L"\u71ff\u221a\uff41");
}

TEST_CASE("path_build converts a Shift_JIS path when the directory argument is empty")
{
    const auto file = cat(KANJI_NI, KANJI_HON, ".prf");
    const auto built = path_build({}, file);
    CHECK(built.wstring() == L"\u65e5\u672c.prf");
}

#endif

using namespace std::literals;

namespace {
/*!
 * @brief \u5185\u5bb9\u3092\u4e00\u6642\u30d5\u30a1\u30a4\u30eb\u306b\u66f8\u304d\u3001angband_fgets() \u3067\u30d5\u30a1\u30a4\u30eb\u306e\u7d42\u7aef\u307e\u3067\u8aad\u3093\u3060\u884c\u3092\u8fd4\u3059
 * @param content \u30d5\u30a1\u30a4\u30eb\u306e\u5185\u5bb9
 * @return \u8aad\u3093\u3060\u884c\u306e\u4e26\u3073
 */
std::vector<std::string> read_all_lines(std::string_view content)
{
    auto *fp = std::tmpfile();
    REQUIRE(fp != nullptr);
    const auto close_file = util::make_finalizer([fp] { std::fclose(fp); });
    REQUIRE(std::fwrite(content.data(), 1, content.size(), fp) == content.size());
    std::rewind(fp);

    std::vector<std::string> lines;
    for (auto line = angband_fgets(fp); line; line = angband_fgets(fp)) {
        lines.push_back(std::move(*line));
    }

    return lines;
}
}

TEST_CASE("angband_fgets returns an empty line for a line starting with NUL")
{
    CHECK(read_all_lines("\0abc\nxyz\n"sv) == std::vector<std::string>{ "", "xyz" });

    // \u6539\u884c\u306e\u7121\u3044\u6700\u5f8c\u306e\u884c\u3067\u3082\u3001\u8aad\u307f\u53d6\u3063\u305f\u884c\u3068\u3057\u3066\u7a7a\u306e\u884c\u3092\u8fd4\u3059
    CHECK(read_all_lines("\0abc"sv) == std::vector<std::string>{ "" });
}

TEST_CASE("angband_fgets returns an empty line for a long line starting with NUL")
{
    // \u8aad\u307f\u53d6\u308a\u306e\u30d0\u30c3\u30d5\u30a1\u306e\u5927\u304d\u3055\u306b\u95a2\u308f\u3089\u305a\u3001\u884c\u5168\u4f53\u30921\u884c\u3068\u3057\u3066\u6271\u3046
    std::string content(1, '\0');
    content.append(1023, 'f').append("\nxyz\n");
    CHECK(read_all_lines(content) == std::vector<std::string>{ "", "xyz" });
}

TEST_CASE("angband_fgets cuts a line at NUL without joining it with the next line")
{
    CHECK(read_all_lines("ab\0cd\nxyz\n"sv) == std::vector<std::string>{ "ab", "xyz" });
}

TEST_CASE("angband_fgets keeps empty lines and the last line without a newline")
{
    CHECK(read_all_lines("abc\n\nxyz") == std::vector<std::string>{ "abc", "", "xyz" });
}
