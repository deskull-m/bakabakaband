#pragma once

#include "system/angband.h"
#include "util/bit-flags-calculator.h"
#include <concepts>
#include <limits>
#include <map>
#include <string>
#include <string_view>
#include <tl/optional.hpp>
#include <unordered_map>
#include <utility>

//! ベースアイテム・アーティファクト・エゴ定義で共有する価格 (cost) の上限。
//! 数量や割引の計算で桁あふれしないよう PRICE 上限の 1/32 に抑える。
//! ヘッダのため windows.h の max マクロ展開を避けて括弧で囲む (MSVC C4003 対策)。
constexpr auto ITEM_COST_MAX = (std::numeric_limits<PRICE>::max)() / 32;

class BaseitemKey;

//! 種別 (tval) の上限。種別を添字にする色の表が 128 要素のため 127 とする。
constexpr int ITEM_TYPE_MAX = 127;
//! 副種別 (sval) の既定上限。
constexpr int ITEM_SUBTYPE_DEFAULT_MAX = 128;
//! 種別ごとの副種別 (sval) の上限を返す (武器熟練度・呪文・魔道具の表のサイズに依存)。定義は baseitem-reader.cpp。
int item_subtype_max(const BaseitemKey &key);

enum class RandomArtActType : short;
RandomArtActType grab_one_activation_flag(std::string_view what);

#ifndef JP
void append_english_text(std::string &text, std::string_view add);
#endif

/// @note clang-formatによるconceptの整形が安定していないので抑制しておく
// clang-format off
/*!
 * @brief 型Keyをキーとして持つような連想配列型のコンセプト
 * std::mapやstd::unordered_mapなどが該当する
 */
template <typename T, typename Key>
concept DictIndexedBy = requires(T t, Key k) {
    requires std::is_constructible_v<typename T::key_type, std::remove_cvref_t<Key>>;
    typename T::mapped_type;
    { t.find(k) } -> std::same_as<typename T::iterator>;
    { t.find(k)->second } -> std::convertible_to<typename T::mapped_type>;
    { t.end() } -> std::same_as<typename T::iterator>;
};
// clang-format on

/*!
 * @brief info文字列から定数を取得し、それを返す
 * @param dict 文字列辞書
 * @param what 文字列
 * @return 見つけたら定数を返す。見つからなければnulloptを返す
 */
template <typename Key, DictIndexedBy<Key> Dict>
tl::optional<typename Dict::mapped_type> info_get_const(const Dict &dict, Key &&what)
{
    if (auto it = dict.find(what); it != dict.end()) {
        return it->second;
    }
    return tl::nullopt;
}

/*!
 * @brief info文字列を定数に変換する
 * @param buf 格納変数
 * @param dict 定数文字列変換表
 * @param what 定数文字列
 * @return 見つけたらtrue
 */
template <typename Key, DictIndexedBy<Key> Dict>
bool info_grab_one_const(uint32_t &buf, const Dict &dict, Key &&what)
{
    auto val = info_get_const(dict, std::forward<Key>(what));
    if (val) {
        buf = static_cast<uint32_t>(*val);
        return true;
    }
    return false;
}

/*!
 * @brief infoパラメータに値をセットする
 * @param パラメータ変数
 * @val 値
 */
template <typename T>
void info_set_value(T &arg, const std::string &val, int base = 10)
{
    arg = static_cast<T>(std::stoi(val, nullptr, base));
}
