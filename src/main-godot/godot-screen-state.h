#pragma once
/*!
 * @file godot-screen-state.h
 * @brief 画面の状態 (view/screen-state.h) の Godot 側へのブリッジ (#9294)
 *
 * ゲームスレッドが入力待ちに入るとき (TERM_XTRA_EVENT) と画面の更新 (TERM_XTRA_FRESH) で
 * 画面の状態を求めて公開し、Godot メインスレッドが screen_state_get() で受け取る。
 */

#include "view/screen-state.h"

#include <cstdint>
#include <string>

/// Godot 側へ渡す画面の状態
struct GodotScreenState {
    uint64_t version{ 0 }; ///< 変わるたびに増える
    ScreenStateInfo info;
    int icky_depth{ 0 };
    std::string prompt; ///< 待っている入力のプロンプト (UTF-8)
};

/// ゲームスレッドから呼ぶ: 画面の状態を求め直し、変わっていれば公開する
void screen_state_update();

/// 公開している画面の状態の版 (毎フレーム呼んでよい)
uint64_t screen_state_get_version();

/// 公開している画面の状態
GodotScreenState screen_state_get();
