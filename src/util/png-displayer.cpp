/*!
 * @file png-displayer.cpp
 * @brief PNG画像表示機能の実装
 */

#include "util/png-displayer.h"

#ifdef WINDOWS
#include "main-win/main-win-utils.h"
#include <gdiplus.h>
#include <iostream>
#include <string>
#include <windows.h>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

using namespace Gdiplus;

// GDI+初期化状態
static ULONG_PTR g_gdiplusToken = 0;
static bool g_gdiplus_initialized = false;
static bool g_png_displayed = false;

/*!
 * @brief GDI+を初期化する
 */
static void ensure_gdiplus_initialized()
{
    if (!g_gdiplus_initialized) {
        GdiplusStartupInput gdiplusStartupInput;
        if (GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr) == Ok) {
            g_gdiplus_initialized = true;
        }
    }
}

/*!
 * @brief PNGファイルを画面中央に表示する
 * @param png_file_path 表示するPNGファイルのパス
 * @return 成功した場合true、失敗した場合false
 */
bool display_png_centered(const std::string &png_file_path)
{
    // メインウィンドウを取得
    HWND hwndMain = get_main_window_hwnd();
    if (!hwndMain) {
        // ウィンドウがまだ作成されていない場合
        return false;
    }

    // GDI+を初期化
    ensure_gdiplus_initialized();
    if (!g_gdiplus_initialized) {
        return false;
    }

    // PNG画像を読み込み
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, png_file_path.c_str(), -1, nullptr, 0);
    if (size_needed <= 0) {
        return false;
    }

    std::wstring wstr_path(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, png_file_path.c_str(), -1, &wstr_path[0], size_needed);

    Image image(wstr_path.c_str());
    if (image.GetLastStatus() != Ok) {
        return false;
    }

    // ウィンドウのクライアント領域を取得
    RECT rect;
    GetClientRect(hwndMain, &rect);
    int windowWidth = rect.right - rect.left;
    int windowHeight = rect.bottom - rect.top;

    // 画像サイズを取得
    int imageWidth = image.GetWidth();
    int imageHeight = image.GetHeight();

    // 中央に配置するための座標を計算
    int x = (windowWidth - imageWidth) / 2;
    int y = (windowHeight - imageHeight) / 2;

    // メインウィンドウのデバイスコンテキストを取得
    HDC hdc = GetDC(hwndMain);
    if (!hdc) {
        return false;
    }

    // Graphicsオブジェクトを作成して直接描画
    Graphics graphics(hdc);
    if (graphics.GetLastStatus() != Ok) {
        ReleaseDC(hwndMain, hdc);
        return false;
    }

    // 背景を黒で塗りつぶし
    SolidBrush blackBrush(Color(255, 0, 0, 0));
    graphics.FillRectangle(&blackBrush, 0, 0, windowWidth, windowHeight);

    // 画像を描画
    Status status = graphics.DrawImage(&image, x, y, imageWidth, imageHeight);

    // クリーンアップ
    ReleaseDC(hwndMain, hdc);

    if (status != Ok) {
        return false;
    }

    g_png_displayed = true;
    return true;
}

/*!
 * @brief 表示中のPNG画像を消去する
 */
void clear_png_display()
{
    if (!g_png_displayed) {
        return;
    }

    // メインウィンドウを取得
    HWND hwndMain = get_main_window_hwnd();
    if (!hwndMain) {
        return;
    }

    // 画面を再描画させる
    InvalidateRect(hwndMain, NULL, TRUE);
    UpdateWindow(hwndMain);

    g_png_displayed = false;
}

#endif // WINDOWS
