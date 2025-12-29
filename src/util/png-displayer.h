#pragma once

#ifdef WINDOWS
#include <string>

/*!
 * @brief PNG画像を描画リストに登録する
 * @param png_file_path 表示するPNGファイルのパス
 * @return 成功した場合true、失敗した場合false
 */
bool register_png_image(const std::string &png_file_path);

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
