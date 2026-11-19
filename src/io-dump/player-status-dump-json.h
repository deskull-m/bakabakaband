#pragma once

#include <nlohmann/json_fwd.hpp>
#include <string>

class CreatureEntity;

/*!
 * @brief クリーチャー（プレイヤー・モンスター）のステータス情報を JSON オブジェクトとして構築する
 * @param creature クリーチャーへの参照
 * @return JSON オブジェクト (format: bakabakaband-creature-status)
 */
nlohmann::json make_creature_status_json(CreatureEntity &creature);

/*!
 * @brief クリーチャー（プレイヤー・モンスター）のステータス情報を JSON 形式で出力する
 * @param creature クリーチャーへの参照
 * @return JSON 文字列
 */
std::string dump_player_status_json(CreatureEntity &creature);

/*!
 * @brief クリーチャー（プレイヤー・モンスター）のステータス情報を JSON 形式でファイルに出力する
 * @param creature クリーチャーへの参照
 * @param fff ファイルポインタ
 */
void dump_player_status_json_to_file(CreatureEntity &creature, FILE *fff);
