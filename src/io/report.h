#pragma once

#include <nlohmann/json_fwd.hpp>

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
