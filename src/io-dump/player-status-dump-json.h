#pragma once

#include <string>

class PlayerType;

/*!
 * @brief プレイヤーのステータス情報をJSON形式で出力する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return JSON文字列
 */
std::string dump_player_status_json(PlayerType *player_ptr);

/*!
 * @brief プレイヤーのステータス情報をJSON形式でファイルに出力する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff ファイルポインタ
 */
void dump_player_status_json_to_file(PlayerType *player_ptr, FILE *fff);
