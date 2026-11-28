#pragma once

/*!
 * @file screen-state.h
 * @brief いま画面に何が出ているかの大分類 (3D 表示などで地図を見せてよいかの判定)
 * @details
 * 3D 表示は「地図を見ている間」だけ出し、メニューや全画面の文章は端末で見せたい。
 * そのために、ゲームが待っている入力 (io/input-context.h) と、
 * 画面を保存して上に重ねている深さ (AngbandWorld::character_icky_depth) から画面の状態を分類する。
 *
 * 判定が曖昧なときは端末を見せる側 (OVERLAY_MENU / FULLSCREEN_TEXT) に倒す。
 */

#include "io/input-context.h"
#include "util/point-2d.h"
#include <cstdint>
#include <tl/optional.hpp>

/*!
 * @brief 画面の状態
 * @details 値は外部 (Godot) へそのまま渡すため、追加は末尾に限る。
 */
enum class ScreenState : uint8_t {
    NO_GAME = 0, //!< フロアがまだ無い (タイトル・キャラクター作成・読み込み中)
    MAP = 1, //!< 地図を見ている (コマンド待ち・ゲームの進行中)
    MAP_WITH_PROMPT = 2, //!< 地図の上で 1 行のプロンプトに答えている (-more-・方向・y/n・数量など)
    TARGETING = 3, //!< ターゲットの選択・見るコマンド・場所の指定 (カーソルが地図の上にある)
    OVERLAY_MENU = 4, //!< 地図の上に一覧などを重ねている (持ち物の一覧・呪文の一覧など)
    FULLSCREEN_TEXT = 5, //!< 画面全体を使う表示 (店・知識・ヘルプ・オプション・キャラクター情報など)
};

/*!
 * @brief 画面の状態を決める材料
 */
struct ScreenStateInputs {
    bool has_dungeon = false; //!< フロアがある (AngbandWorld::character_dungeon)
    int icky_depth = 0; //!< 画面を保存して上に重ねている深さ (AngbandWorld::character_icky_depth)
    bool cleared_since_save = false; //!< いちばん上に重ねた画面を開いてから、画面全体を消したか
    tl::optional<InputContext> input_context; //!< 待っている入力 (分からなければ無し)
};

/*!
 * @brief 画面の状態の判定結果
 */
struct ScreenStateInfo {
    ScreenState state = ScreenState::NO_GAME;
    tl::optional<InputKind> input_kind; //!< 待っている入力の種類 (分からなければ無し)
    tl::optional<Pos2D> cursor_pos; //!< TARGETING のときのカーソルのフロアでの座標

    bool operator==(const ScreenStateInfo &) const = default;
};

ScreenStateInfo classify_screen_state(const ScreenStateInputs &inputs);

/// ゲームの現在の状態から ScreenStateInputs を作る
ScreenStateInputs collect_screen_state_inputs();
