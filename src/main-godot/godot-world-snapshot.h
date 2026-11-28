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
 * - 差分更新 (#9293): view/view-observer.h の通知で変わったかもしれないグリッドを集め、
 *   次の push ではそのグリッドだけを求め直す。各グリッドは「最後に変わった版」を持つので、
 *   利用者は途中の版を読み飛ばしても「ある版以降に変わったグリッド」を取りこぼさない
 */

#include <cstdint>
#include <memory>
#include <vector>

struct ObservableWorld;
class PlayerType;

/// 受け渡し用のスナップショット
struct GodotWorldSnapshot {
    uint64_t version{ 0 }; ///< push のたびに増える (変化検出用)
    /// フロア全体を作り直した版 (階の移動・大きさの変化など)。これより前の版からの差分は作れない
    uint64_t floor_version{ 0 };
    std::shared_ptr<const ObservableWorld> world;
    /// グリッドごとの最後に変わった版 (world->grids と同じ並び)
    std::shared_ptr<const std::vector<uint64_t>> grid_versions;
};

/// ゲームスレッドの開始時に呼ぶ: 地図表示の観測者を登録する
void world_snapshot_install_observer();

/// スナップショットを作るかどうかを切り替える (どのスレッドからでも呼べる)
void world_snapshot_set_enabled(bool enabled);

/// スナップショットを作る設定か
bool world_snapshot_is_enabled();

/// 検証モード: 差分で作った結果をフロア全体から作った結果と突き合わせ、食い違いを報告する
void world_snapshot_set_verify(bool verify);

/// 検証モードで見つかった食い違いのグリッド数の累計
uint64_t world_snapshot_get_verify_mismatches();

/// ゲームスレッドから呼ぶ: 有効ならフロアの観測結果を作って差し替える
void world_snapshot_push(PlayerType *player_ptr);

/// 最新のスナップショットの版を返す (中身をコピーしないので毎フレーム呼んでよい)
uint64_t world_snapshot_get_version();

/// 最新のスナップショットを返す
GodotWorldSnapshot world_snapshot_get();
