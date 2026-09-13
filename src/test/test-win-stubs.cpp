/*!
 * @file test-win-stubs.cpp
 * @brief テスト実行ファイル用の Windows 依存シンボルのスタブ
 * @details BakabakabandCore.lib の png-displayer.cpp は WINDOWS(非 Godot) ビルドで
 * get_main_window_hwnd() を参照するが、その実体はテスト実行ファイルがリンクしない
 * main-win.cpp にある。テストは PNG 描画を行わないため、ここで nullptr を返す
 * no-op スタブを提供してリンクエラー (LNK2019) を回避する。
 * png-displayer.cpp の参照条件 (WINDOWS かつ 非 USE_GODOT) と同一ガードで定義する。
 */
#if defined(WINDOWS) && !defined(USE_GODOT)
#include "main-win/main-win-utils.h"

HWND get_main_window_hwnd(void)
{
    return nullptr;
}
#endif
