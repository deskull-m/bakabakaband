#pragma once
/*!
 * @file godot-world-snapshot.h
 * @brief 観測可能ワールドモデル (view/observable-world.h) の Godot 側へのブリッジ
 *
 * ゲームスレッドが world_snapshot_push() でフロアの観測結果を作り、
 * Godot メインスレッドが world_snapshot_get() で最新のものを受け取る。
 *
 * - 作るのは world_snapshot_set_enabled(true) の間だけ (既定は無効)。
 *   3D 表示などの利用者がいないときにフロア全体を走査するコストを掛けない
 * - スナップショットは不変オブジェクトとして shared_ptr で受け渡すので、
 *   Godot 側はロック無しで読める
 */

#include <cstdint>
#include <memory>

struct ObservableWorld;
class PlayerType;

/// 受け渡し用のスナップショット
struct GodotWorldSnapshot {
    uint64_t version{ 0 }; ///< push のたびに増える (変化検出用)
    std::shared_ptr<const ObservableWorld> world;
};

/// スナップショットを作るかどうかを切り替える (どのスレッドからでも呼べる)
void world_snapshot_set_enabled(bool enabled);

/// スナップショットを作る設定か
bool world_snapshot_is_enabled();

/// ゲームスレッドから呼ぶ: 有効ならフロアの観測結果を作って差し替える
void world_snapshot_push(PlayerType *player_ptr);

/// 最新のスナップショットの版を返す (中身をコピーしないので毎フレーム呼んでよい)
uint64_t world_snapshot_get_version();

/// 最新のスナップショットを返す
GodotWorldSnapshot world_snapshot_get();
