/*!
 * @file png-displayer.cpp
 * @brief PNG画像表示機能の実装
 */

#include "util/png-displayer.h"

#ifdef WINDOWS
#include "main-win/main-win-utils.h"
#include <gdiplus.h>
#include <iostream>
#include <map>
#include <string>
#include <windows.h>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

using namespace Gdiplus;

// 画像情報構造体
struct PngImageInfo {
    std::wstring path;
    int x;
    int y;
    int width;
    int height;
};

// GDI+初期化状態
static ULONG_PTR g_gdiplusToken = 0;
static bool g_gdiplus_initialized = false;

// 登録されている画像のマップ（ファイルパス -> 画像情報）
static std::map<std::string, PngImageInfo> g_registered_images;

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
 * @brief PNG画像を描画リストに登録する
 * @param png_file_path 表示するPNGファイルのパス
 * @return 成功した場合true、失敗した場合false
 */
bool register_png_image(const std::string &png_file_path)
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

    // 画像の読み込みテスト
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

    // 画像情報を登録
    PngImageInfo info;
    info.path = wstr_path;
    info.x = x;
    info.y = y;
    info.width = imageWidth;
    info.height = imageHeight;

    g_registered_images[png_file_path] = info;

    return true;
}

/*!
 * @brief PNG画像を描画リストから削除する
 * @param png_file_path 削除するPNGファイルのパス
 */
void unregister_png_image(const std::string &png_file_path)
{
    auto it = g_registered_images.find(png_file_path);
    if (it != g_registered_images.end()) {
        g_registered_images.erase(it);
    }
}

/*!
 * @brief 登録されている全てのPNG画像を再描画する
 * @details termの表示前に呼び出すことで、登録された画像を画面に表示する
 */
void redraw_png_images()
{
    if (g_registered_images.empty()) {
        return;
    }

    // メインウィンドウを取得
    HWND hwndMain = get_main_window_hwnd();
    if (!hwndMain) {
        return;
    }

    // GDI+が初期化されていない場合は何もしない
    if (!g_gdiplus_initialized) {
        return;
    }

    // メインウィンドウのデバイスコンテキストを取得
    HDC hdc = GetDC(hwndMain);
    if (!hdc) {
        return;
    }

    // Graphicsオブジェクトを作成
    Graphics graphics(hdc);
    if (graphics.GetLastStatus() != Ok) {
        ReleaseDC(hwndMain, hdc);
        return;
    }

    // 登録されている全ての画像を描画
    for (const auto &pair : g_registered_images) {
        const PngImageInfo &info = pair.second;

        // 画像を読み込み
        Image image(info.path.c_str());
        if (image.GetLastStatus() != Ok) {
            continue;
        }

        // 画像を描画
        graphics.DrawImage(&image, info.x, info.y, info.width, info.height);
    }

    // クリーンアップ
    ReleaseDC(hwndMain, hdc);
}

#endif // WINDOWS
