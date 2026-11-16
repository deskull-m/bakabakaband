#pragma once
/*!
 * @file godot-character-sheet.h
 * @brief ステータス画面のダンプ (C コマンドの画面、キャラクタダンプの先頭と同じ内容) の Godot 側へのブリッジ
 *
 * ゲームスレッドが画面の更新 (TERM_XTRA_FRESH) で、ゲームターンが進んだときだけ画面外の端末に描いて作り直し、
 * 中身が変わったときだけ版を進めて公開する。作るのは character_sheet_set_enabled(true) の間だけ。
 */

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class PlayerType;

/// 端末の 1 文字 (全角は 2 桁を占める)
struct GodotSheetCell {
    int col{ 0 };
    int width{ 1 }; ///< 占める桁数 (1 か 2)
    std::string text; ///< UTF-8
    uint8_t color{ 0 }; ///< 端末の色の番号
};

struct GodotSheetLine {
    std::string text; ///< 行全体 (UTF-8)
    std::vector<GodotSheetCell> cells; ///< 空白以外の文字
};

struct GodotCharacterSheet {
    uint64_t version{ 0 };
    std::shared_ptr<const std::vector<GodotSheetLine>> lines;
};

void character_sheet_set_enabled(bool enabled);

/// ゲームスレッドから呼ぶ: 有効でゲームターンが進んでいれば作り直し、変わっていれば公開する
void character_sheet_push(PlayerType *player_ptr);

/// 次の push で必ず作り直す (表示を開いたときなど)
void character_sheet_request_refresh();

uint64_t character_sheet_get_version();
GodotCharacterSheet character_sheet_get();
