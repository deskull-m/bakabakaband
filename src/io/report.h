#pragma once

#include <nlohmann/json_fwd.hpp>
#include <string>
#include <string_view>

/*!
 * @brief 保存している画面ダンプをセーブデータ用の文字列にする
 * @return 画面の JSON 文字列。保存していなければ空文字列
 * @details スコア送信を後回しにしたセーブデータに画面を残すために使う。WORLD_SCORE 無効でも空文字列を返す。
 */
std::string serialize_screen_dump();

/*!
 * @brief セーブデータの文字列から画面ダンプを復元する
 * @param text serialize_screen_dump() が返した文字列。空や解析できないもの (旧形式の HTML など) は未保存扱い
 */
void restore_screen_dump(std::string_view text);

#ifdef WORLD_SCORE

class CreatureEntity;
bool report_score(CreatureEntity &creature);

/*!
 * @brief 画面の内容を色付きテキストの JSON として取得する
 * @param creature クリーチャーへの参照
 * @return bakabakaband-screen 形式の JSON オブジェクト。大きすぎる場合は null
 */
nlohmann::json make_screen_dump(CreatureEntity &creature);

/*!
 * @brief 死亡時などの画面をスコア送信用に保存する
 * @param creature クリーチャーへの参照
 */
void capture_screen_dump(CreatureEntity &creature);

/*!
 * @brief capture_screen_dump() で保存した画面を返す。未保存なら null
 */
const nlohmann::json &get_screen_dump();
#endif
