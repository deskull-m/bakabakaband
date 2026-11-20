#pragma once

#include "system/angband.h"
#include <string_view>
#include <tl/optional.hpp>

class term_win;

/** 画面情報保存スタックからの読み出しオプション */
enum class ScreenLoadOptType {
    ONE, //!< スタックの先頭のデータを1つ読み出す
    ALL, //!< スタックからすべてのデータを読み出す(画面は最後に読み出したデータになる)
};

void move_cursor(int row, int col);
void flush(void);
void screen_save();
void screen_load(ScreenLoadOptType opt = ScreenLoadOptType::ONE);
void c_put_str(TERM_COLOR attr, std::string_view sv, TERM_LEN row, TERM_LEN col);
void put_str(std::string_view sv, TERM_LEN row, TERM_LEN col);
void c_prt(TERM_COLOR attr, std::string_view sv, TERM_LEN row, TERM_LEN col);
void prt(std::string_view sv, TERM_LEN row, TERM_LEN col);
void c_roff(TERM_COLOR attr, std::string_view str);
void roff(std::string_view str);
void clear_from(int row);

/// メイン画面の全体が消された (term_clear() / clear_from(0) から呼ぶ)
void note_main_screen_cleared();

/// いちばん上に重ねた画面 (screen_save()) を開いてから、メイン画面の全体を消したか
bool is_main_screen_cleared_since_save();

/*!
 * @brief 端末の矩形 (桁・行)
 */
struct ScreenRegion {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;

    bool operator==(const ScreenRegion &) const = default;
};

/*!
 * @brief 地図の上に重ねた画面の範囲を求められるよう、最初の screen_save() の時点のメイン画面を覚えるか
 * @details 3D 表示などで、一覧 (持ち物・装備など) を地図の上の窓に写すときに有効にする。既定は無効で、
 * 無効の間は何も覚えない。表示の判定にだけ使い、ゲームの進行には使わない
 */
void set_main_screen_overlay_tracking(bool enabled);

/*!
 * @brief いま地図の上に重ねている部分の範囲 (最初の screen_save() の時点のメイン画面と違うセルを囲む矩形)
 * @details 1 行目 (メッセージの行) は除く。重ねていない・覚えていない・違うセルが無いときは無し
 */
tl::optional<ScreenRegion> calc_main_screen_overlay_region();

/*!
 * @brief 2 つの画面で違うセルを囲む矩形 (first_row より上の行は見ない)。大きさが違う・違うセルが無いときは無し
 */
tl::optional<ScreenRegion> calc_screen_overlay_region(const term_win &base, const term_win &current, int first_row);
