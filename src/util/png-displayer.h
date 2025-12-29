#pragma once

#ifdef WINDOWS
#include <string>

/*!
 * @brief 画像の縦方向配置
 */
enum class PngVAlign {
    TOP, //!< 上揃え
    CENTER, //!< 中央揃え
    BOTTOM //!< 下揃え
};

/*!
 * @brief 画像の横方向配置
 */
enum class PngHAlign {
    LEFT, //!< 左揃え
    CENTER, //!< 中央揃え
    RIGHT //!< 右揃え
};

/*!
 * @brief PNG画像を描画リストに登録する
 * @param png_file_path 表示するPNGファイルのパス
 * @param v_align 縦方向の配置（デフォルト：中央）
 * @param h_align 横方向の配置（デフォルト：中央）
 * @param x_offset 横方向のオフセット（ピクセル、デフォルト：0）
 * @param y_offset 縦方向のオフセット（ピクセル、デフォルト：0）
 * @return 成功した場合true、失敗した場合false
 */
bool register_png_image(const std::string &png_file_path,
    PngVAlign v_align = PngVAlign::CENTER,
    PngHAlign h_align = PngHAlign::CENTER,
    int x_offset = 0,
    int y_offset = 0);

/*!
 * @brief PNG画像を描画リストから削除する
 * @param png_file_path 削除するPNGファイルのパス
 */
void unregister_png_image(const std::string &png_file_path);

/*!
 * @brief 登録されている全てのPNG画像を再描画する
 * @details termの表示前に呼び出すことで、登録された画像を画面に表示する
 */
void redraw_png_images();

#endif // WINDOWS
