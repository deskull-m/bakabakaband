#pragma once

#include "system/angband-exceptions.h"
#include "system/angband.h"
#include "target/target.h"
#include "util/point-2d.h"
#include <array>
#include <span>
#include <tl/optional.hpp>
#include <utility>

/*!
 * @brief 視界及び光源の過渡処理配列サイズ / Maximum size of the "temp" array
 * @details We must be as large as "VIEW_MAX" and "LITE_MAX" for proper functioning
 * of "update_view()" and "update_lite()".  We must also be as large as the
 * largest illuminatable room, but no room is larger than 800 grids.  We
 * must also be large enough to allow "good enough" use as a circular queue,
 * to calculate monster flow, but note that the flow code is "paranoid".
 */
#define TEMP_MAX 2298

/*!
 * @brief 方向クラス
 * @todo
 * メンバにstd::unique_ptrを持つTargetクラスを取り込んだことにより、
 * std::unique_ptrがconstexprに対応するのはC++23からであるため、
 * C++20ではこのクラスもconstexprに対応できなくなった。
 * 将来的にC++23を導入した際には再度constexprに対応させたい。
 * なお、DirectionにTargetを持たせずDirectionかTargetを持つ上位の
 * Aimのようなクラスを作るのが設計的には理想的ではあるが、
 * 影響範囲が大きく現状労力に見合わなそう。
 */
class Direction {
public:
    /*!
     * @brief 方向クラスのコンストラクタ
     *
     * 方向ID dir で指定した方向を示すクラスを生成する。
     *
     * 方向IDは5を中央とし以下のように定義される。
     *
     * 7 8 9
     *  \|/
     * 4-5-6
     *  /|\
     * 1 2 3
     *
     * 0: 無効な方向を意味する
     *
     * @param dir 方向ID
     */
    explicit Direction(int dir)
        : dir_(dir)
    {
        if ((dir < 0) || std::cmp_greater_equal(dir, DIR_TO_VEC.size())) {
            THROW_EXCEPTION(std::logic_error, "Invalid direction is specified!");
        }
    }

    /*!
     * @brief 無効な方向を示す方向クラスのインスタンスを生成する
     */
    static Direction none()
    {
        return Direction(0);
    }

    /*!
     * @brief 自身のマスを示す方向クラスのインスタンスを生成する
     */
    static Direction self()
    {
        return Direction(5);
    }

    /*!
     * @brief 特定のマスをターゲット中であることを示す方向クラスのインスタンスを生成する
     */
    static Direction targetting(const Target &target)
    {
        auto dir = Direction::self();
        dir.target = target;
        return dir;
    }

    /*!
     * @brief 引数に指定した、円周順に方向を示す値から方向クラスのインスタンスを生成する
     *
     * 引数 cdir は南を0とし反時計回りの順。
     *
     * 5 4 3
     *  \|/
     * 6-@-2
     *  /|\
     * 7 0 1
     *
     * @param cdir 円周順に方向を示す値
     */
    static Direction from_cdir(int cdir)
    {
        static const std::array<Direction, 8> DIRECTIONS_8_CCW = {
            { Direction(2), Direction(3), Direction(6), Direction(9), Direction(8), Direction(7), Direction(4), Direction(1) }
        };

        if ((cdir < 0) || std::cmp_greater_equal(cdir, DIRECTIONS_8_CCW.size())) {
            THROW_EXCEPTION(std::logic_error, "Invalid direction is specified!");
        }

        return DIRECTIONS_8_CCW[cdir];
    }

    static constexpr std::span<const Direction> directions();
    static constexpr std::span<const Direction> directions_8();
    static constexpr std::span<const Direction> directions_4();
    static constexpr std::span<const Direction> directions_diag4();
    static constexpr std::span<const Direction> directions_reverse();
    static constexpr std::span<const Direction> directions_8_reverse();

    /*!
     * @brief 引数で指定された方向IDが有効かどうかを返す
     *
     * @param dir 方向ID
     * @return 有効な方向IDならばtrue、そうでなければfalse
     */
    static constexpr bool is_valid_dir(int dir)
    {
        return (dir >= 0) && std::cmp_less(dir, DIR_TO_VEC.size());
    }

    /*!
     * @brief 方向を示すベクトルを取得する
     *
     * ベクトルのy, xの値はその方向を示すY成分/X成分があれば1もしくは-1、なければ0を返す。
     *
     * @return 方向を示すベクトル
     */
    constexpr Pos2DVec vec() const
    {
        return DIR_TO_VEC[this->dir_];
    }

    /*!
     * @brief 方向IDを取得する
     * @return 方向ID
     */
    constexpr int dir() const noexcept
    {
        return this->dir_;
    }

    /*!
     * @brief 円周順に方向を示す値を取得する
     * @return 円周順に方向を示す値。示す値がない(方向IDが0もしくは5)場合はtl::nulloptを返す
     */
    constexpr tl::optional<int> cdir() const
    {
        return DIR_TO_CDIR[this->dir_];
    }

    /*!
     * @brief 8方向いずれかの方向を指し示しているかどうかを返す
     * @return 方向IDが8方向いずれかの方向を示している場合はtrue、そうでない場合はfalseを返す。
     */
    constexpr bool has_direction() const noexcept
    {
        return this->dir_ != 0 && this->dir_ != 5;
    }

    /*!
     * @brief 方向が軸方向(上下左右)のいずれかを示しているかどうかを返す
     * @return 上下左右ならばtrue、そうでなければfalse
     */
    constexpr bool is_axial() const noexcept
    {
        return this->dir_ == 2 || this->dir_ == 4 || this->dir_ == 6 || this->dir_ == 8;
    }

    /*!
     * @brief 方向が斜め方向かどうかを返す
     * @return 斜め方向ならばtrue、そうでなければfalse
     */
    constexpr bool is_diagonal() const noexcept
    {
        return this->dir_ != 5 && (this->dir_ & 0x01);
    }

    /*!
     * @brief 特定のマスかモンスターをターゲット中であるかどうかを返す
     */
    constexpr bool is_targetting() const noexcept
    {
        return this->dir_ == 5 && this->target.has_value();
    }

    /*!
     * @brief 特定のマスかモンスターをターゲット中であり、かつターゲットが有効かどうかを返す
     */
    constexpr bool is_target_okay() const
    {
        return this->is_targetting() && this->target->is_okay();
    }

    /*!
     * @brief ターゲットの座標を返す
     *
     * 特定のマスかモンスターをターゲット中の場合はその座標を返す。ただしターゲットが有効でない場合は基準座標を返す。
     * 単に方向を指定している場合は引数で指定した基準座標からその方向へlengthマス分進んだ座標を返す。
     *
     * @param pos (方向の場合の)基準座標
     * @param length (方向の場合の)基準座標からの距離。デフォルトは1(隣接マス)
     * @return ターゲットの座標
     */
    Pos2D get_target_position(const Pos2D &pos, int length = 1) const
    {
        if (this->is_targetting()) {
            return this->target->get_position().value_or(pos);
        }

        return pos + this->vec() * length;
    }

    /*!
     * @brief 方向を45度単位で回転させた方向クラスのインスタンスを生成する
     *
     * すなわち、 cdir をdeltaだけ増減させた(0↔7でつながる)方向を示すインスタンスを返す。
     *
     * @param delta 回転させる回数。正の値で反時計回り、負の値で時計回りに回転する。
     * @return 回転させた方向を示す方向クラスのインスタンス
     */
    Direction rotated_45degree(int delta) const
    {
        const auto cdir = this->cdir();
        if (!cdir) {
            return *this;
        }

        const auto new_cdir = (*cdir + delta) & 0x7;
        return Direction::from_cdir(new_cdir);
    }

    /*!
     * @brief インスタンスが有効な方向を示しているかどうかを返す
     * @return 有効な方向を示している場合はtrue、そうでない場合はfalse
     */
    constexpr explicit operator bool() const noexcept
    {
        return this->dir_ != 0;
    }

private:
    /// 方向IDに対応するベクトルの定義
    static constexpr std::array<Pos2DVec, 10> DIR_TO_VEC = {
        { { 0, 0 }, { 1, -1 }, { 1, 0 }, { 1, 1 }, { 0, -1 }, { 0, 0 }, { 0, 1 }, { -1, -1 }, { -1, 0 }, { -1, 1 } }
    };

    /// 方向IDに対応する円周順に方向を示す値の定義
    static constexpr std::array<tl::optional<int>, 10> DIR_TO_CDIR = { { tl::nullopt, 7, 0, 1, 6, tl::nullopt, 2, 5, 4, 3 } };

    int dir_; //<! 方向ID
    tl::optional<Target> target; //<! ターゲット
};

constexpr bool operator==(const Direction &dir1, const Direction &dir2) noexcept
{
    return dir1.dir() == dir2.dir() && dir1.is_targetting() == dir2.is_targetting();
}

/* 以降の定義は直接使用しないようdetail名前空間に入れておく*/
namespace detail {
/// 下・上・右・左・右下・左下・右上・左上・中央の順にDirectionクラスのインスタンスを保持する配列
inline const std::array<Direction, 9> DIRECTIONS = {
    { Direction(2), Direction(8), Direction(6), Direction(4), Direction(3), Direction(1), Direction(9), Direction(7), Direction(5) }
};

/// DIRECTIONSの要素を逆順にした配列
inline const std::array<Direction, 9> REVERSE_DIRECTIONS = {
    { Direction(5), Direction(7), Direction(9), Direction(1), Direction(3), Direction(4), Direction(6), Direction(8), Direction(2) }
};
}

/*!
 * @brief 下・上・右・左・右下・左下・右上・左上・中央 の順にDirectionクラスのオブジェクトを参照する配列を返す
 */
constexpr std::span<const Direction> Direction::directions()
{
    return detail::DIRECTIONS;
}
/*!
 * @brief 下・上・右・左・右下・左下・右上・左上 の順にDirectionクラスのオブジェクトを参照する配列を返す
 */
constexpr std::span<const Direction> Direction::directions_8()
{
    return Direction::directions().first(8);
}
/*!
 * @brief 下・上・右・左 の順にDirectionクラスのオブジェクトを参照する配列を返す
 */
constexpr std::span<const Direction> Direction::directions_4()
{
    return Direction::directions().first(4);
}
/*!
 * @brief 右下・左下・右上・左上 の順にDirectionクラスのオブジェクトを参照する配列を返す
 */
constexpr std::span<const Direction> Direction::directions_diag4()
{
    return Direction::directions().subspan(4, 4);
}

/*!
 * @brief Directions::directions() の逆順の配列を返す
 */
constexpr std::span<const Direction> Direction::directions_reverse()
{
    return detail::REVERSE_DIRECTIONS;
}

/*!
 * @brief Directions::directions_8() の逆順の配列を返す
 * @note 斜め方向を先に処理したい時に使用する
 */
constexpr std::span<const Direction> Direction::directions_8_reverse()
{
    return Direction::directions_reverse().subspan(1, 8);
}

class PlayerType;
Pos2D mmove2(const Pos2D &pos_orig, const Pos2D &pos1, const Pos2D &pos2);
bool player_can_see_bold(PlayerType *player_ptr, POSITION y, POSITION x);

class MonsterEntity;
bool is_seen(PlayerType *player_ptr, const MonsterEntity &monster);
