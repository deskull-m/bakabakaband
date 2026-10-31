#pragma once

#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "system/angband.h"
#include "util/type-concepts.h"
#include <concepts>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include <tl/optional.hpp>
#include <type_traits>
#include <utility>

class Dice;

using Range = std::pair<int, int>;

tl::expected<void, ParseErrorType> info_set_string(const nlohmann::json &json, std::string &data, bool is_required);
tl::expected<void, ParseErrorType> info_set_dice(const nlohmann::json &json, Dice &dice, bool is_required);
tl::expected<void, ParseErrorType> info_set_bool(const nlohmann::json &json, bool &bool_value, bool is_required);
const nlohmann::json &get_json_value(const nlohmann::json &json, std::string_view key);
tl::expected<void, ParseErrorType> info_validate_json_array(const nlohmann::json &root, std::string_view key, bool allow_empty = true);

/*!
 * @brief JSON Objectから整数値もしくはenum値を取得する

 * 引数で与えられたJSON Objectから整数値もしくはenum値を取得し、 data に格納する。
 *
 * @param json 整数値が格納されたJSON Object
 * @param data 値を格納する変数への参照
 * @param is_required 必須かどうか。
 * 必須でJSON Objectがnullの場合はエラーを返す。必須でない場合は何もせずに終了する。
 * @param range 取得した値の範囲を指定する。範囲外の値が取得された場合はエラーを返す。
 * range の指定にかかわらず格納先の型 (enum は基底型) の表現範囲も検証する。
 * @return 成功時は空の値、失敗時は解析エラーコード（ParseErrorType::TOO_FEW_ARGUMENTS、ParseErrorType::INVALID_TYPE、
 * ParseErrorType::INVALID_VALUEのいずれか。条件は上記のとおり）
 */
template <IntegralOrEnum T>
tl::expected<void, ParseErrorType> info_set_integer(const nlohmann::json &json, T &data, bool is_required, tl::optional<Range> range = tl::nullopt)
{
    if (json.is_null()) {
        if (is_required) {
            return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
        }
        return {};
    }
    if (!json.is_number_integer()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }

    // 範囲チェックは格納先の型へ変換する前に行う。変換してから比べると、
    // 格納先の型で表現できない値が切り詰められて範囲内に収まり、チェックをすり抜ける
    // (例: uint8_t への 300 は 44 になり Range(0, 255) を通ってしまう)
    const auto fits_destination = [](auto value) {
        if constexpr (std::is_enum_v<T>) {
            return std::in_range<std::underlying_type_t<T>>(value);
        } else {
            return std::in_range<T>(value);
        }
    };
    if (json.is_number_unsigned()) {
        const auto value = json.get<nlohmann::json::number_unsigned_t>();
        if (!fits_destination(value) || (range && (std::cmp_less(value, range->first) || std::cmp_greater(value, range->second)))) {
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }

        data = static_cast<T>(value);
        return {};
    }

    const auto value = json.get<nlohmann::json::number_integer_t>();
    if (!fits_destination(value) || (range && (value < range->first || value > range->second))) {
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }

    data = static_cast<T>(value);
    return {};
}

/*!
 * @brief JSON のトークン (文字列) を辞書または検索関数で値へ変換する
 * @param json トークン文字列。null は必須値の欠落として扱う
 * @param table 辞書 (info_get_const で引ける連想配列)、または文字列を受け取り tl::optional を返す検索関数
 * @return 対応する値。欠落は TOO_FEW_ARGUMENTS、型違い (非文字列) は INVALID_TYPE、未知の名前は INVALID_FLAG
 * @details 定義リーダーごとに重複していた「null→欠落 / 非文字列→型エラー / 辞書引き→未知名エラー」の
 *          判定を集約する (上流 hengband#6043 相当)。
 */
template <typename Table>
auto info_read_token(const nlohmann::json &json, const Table &table)
{
    const auto lookup = [&table](const std::string &name) {
        if constexpr (std::is_invocable_v<const Table &, const std::string &>) {
            return table(name);
        } else {
            return info_get_const(table, name);
        }
    };
    using Value = typename decltype(lookup(std::declval<const std::string &>()))::value_type;
    using Result = tl::expected<Value, ParseErrorType>;
    if (json.is_null()) {
        return Result(tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS));
    }
    if (!json.is_string()) {
        return Result(tl::make_unexpected(ParseErrorType::INVALID_TYPE));
    }
    const auto value = lookup(json.get_ref<const std::string &>());
    if (!value) {
        return Result(tl::make_unexpected(ParseErrorType::INVALID_FLAG));
    }
    return Result(*value);
}
