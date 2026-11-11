/*!
 * @brief 16 方位の方角名を扱うユーティリティ
 */

#include "util/compass-direction.h"
#include "locale/language-switcher.h"
#include <array>
#include <cmath>

namespace {
/*!
 * @brief 16 方位の名称 (北を 0 として時計回り)
 */
const std::array<std::string_view, 16> COMPASS_DIRECTION_NAMES = {
    _("北", "north"),
    _("北北東", "north-northeast"),
    _("北東", "northeast"),
    _("東北東", "east-northeast"),
    _("東", "east"),
    _("東南東", "east-southeast"),
    _("南東", "southeast"),
    _("南南東", "south-southeast"),
    _("南", "south"),
    _("南南西", "south-southwest"),
    _("南西", "southwest"),
    _("西南西", "west-southwest"),
    _("西", "west"),
    _("西北西", "west-northwest"),
    _("北西", "northwest"),
    _("北北西", "north-northwest"),
};
}

/*!
 * @brief 相対座標を 16 方位の添字に変換する
 * @param vec 相対座標 (フロア座標系なので y は南が正、x は東が正)
 * @return 北を 0 として時計回りの添字 (0〜15)
 * @details 22.5 度刻みで最も近い方位へ丸める。vec が原点の場合は 0 (北) を返すが、
 *          方角としては意味を持たないので呼出側で弾くこと。
 *          表示用の文字列を決めるだけでゲーム状態や乱数列には影響しないため、
 *          整数演算に落とさず atan2 をそのまま使っている。
 */
int calc_compass_direction_index(const Pos2DVec &vec)
{
    constexpr auto directions = static_cast<int>(std::tuple_size_v<decltype(COMPASS_DIRECTION_NAMES)>);

    // 1 ラジアンあたりのセクタ数。円周率は <numbers> が CI の全コンパイラで揃っている
    // 保証が無いためリテラルで置く。
    constexpr auto sectors_per_radian = directions / (2.0 * 3.14159265358979323846);

    // フロア座標は y が南へ増えるので、北成分は -vec.y。
    // atan2(東成分, 北成分) で北を 0 とした時計回りの角度になる。
    const auto radian = std::atan2(static_cast<double>(vec.x), static_cast<double>(-vec.y));
    const auto sector = static_cast<int>(std::lround(radian * sectors_per_radian));
    return ((sector % directions) + directions) % directions;
}

/*!
 * @brief 相対座標を 16 方位の名称に変換する
 * @param vec 相対座標 (フロア座標系なので y は南が正、x は東が正)
 * @return 16 方位の名称
 */
std::string_view describe_compass_direction(const Pos2DVec &vec)
{
    return COMPASS_DIRECTION_NAMES[calc_compass_direction_index(vec)];
}
