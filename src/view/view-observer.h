#pragma once

/*!
 * @file view-observer.h
 * @brief 地図表示の変化を外部 (3D 表示など) へ知らせる観測点
 * @details
 * 端末版の地図は、グリッドの見え方が変わるたびに lite_spot() で、
 * 地図全体が変わるたびに print_map() で描き直される。
 * 同じ 2 か所で観測者へ知らせることで、観測者は変わったグリッドだけを求め直せる。
 *
 * - 観測者は 1 つだけ登録できる。未登録 (既定) なら通知は null 判定 1 回で終わる
 * - 通知はゲームスレッドから呼ばれる。観測者はゲームの状態を変えてはならない
 * - 乱数を消費しない (端末版との乱数列の一致を崩さない)
 */

#include "util/point-2d.h"

class ViewObserver {
public:
    virtual ~ViewObserver() = default;

    /// グリッドの見え方が変わったかもしれない (lite_spot())
    virtual void on_grid_changed(const Pos2D &pos) = 0;

    /// 地図全体の見え方が変わったかもしれない (print_map()。階の移動・画面の再描画など)
    virtual void on_all_grids_changed() = 0;
};

/// 観測者を登録する (nullptr で解除)
void set_view_observer(ViewObserver *observer);

void notify_view_grid_changed(const Pos2D &pos);
void notify_view_all_grids_changed();
