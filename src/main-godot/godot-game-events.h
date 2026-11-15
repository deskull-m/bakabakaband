#pragma once
/*!
 * @file godot-game-events.h
 * @brief 表示用のゲームイベント (view/view-event.h) の Godot 側へのブリッジ (#9295)
 *
 * ゲームスレッドで起きたイベントをキューに溜め、Godot メインスレッドが毎フレームまとめて取り出す。
 * - 有効にしている間だけ溜める (既定は無効)
 * - キューが溢れたら古いものから捨てる (演出は後追いなので、取りこぼしてもゲームには影響しない)
 * - 移動 (MOVE / TELEPORT / APPEAR / DISAPPEAR) は godot-world-snapshot.cpp が
 *   スナップショットの差から作って game_events_push() で入れる
 */

#include "view/view-event.h"

#include <vector>

/// ゲームスレッドの開始時に呼ぶ: イベントの受け手を登録する
void game_events_install_sink();

void game_events_set_enabled(bool enabled);
bool game_events_is_enabled();

/// イベントをキューに入れる (有効なときだけ)
void game_events_push(const ViewEvent &event);

/// 溜まったイベントをすべて取り出す (Godot メインスレッドから呼ぶ)
std::vector<ViewEvent> game_events_drain();
