#pragma once

#include <string>

class PlayerType;

/*!
 * @brief スコアをワールドスコアサーバーに送信する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 成功した場合true、失敗した場合false
 */
bool send_world_score(PlayerType *player_ptr);

/*!
 * @brief スコアサーバーのURLを取得
 * @return スコアサーバーのURL
 */
std::string get_score_server_url();

/*!
 * @brief スコアサーバーのURLを設定
 * @param url 設定するURL
 */
void set_score_server_url(const std::string &url);
