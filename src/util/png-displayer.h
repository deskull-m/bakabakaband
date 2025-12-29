#pragma once

#ifdef WINDOWS
#include <string>

/*!
 * @brief PNGファイルを画面中央に表示する
 * @param png_file_path 表示するPNGファイルのパス
 * @return 成功した場合true、失敗した場合false
 */
bool display_png_centered(const std::string &png_file_path);

/*!
 * @brief 表示中のPNG画像を消去する
 */
void clear_png_display();

#endif // WINDOWS
