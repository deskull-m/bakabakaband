#pragma once

#include <nlohmann/json_fwd.hpp>
#include <string>

class CreatureEntity;

/*!
 * @brief スコアサーバへ送信する JSON (bakabakaband-score-submission 形式) を組み立てる
 * @param creature クリーチャーへの参照
 * @param character_dump キャラクターダンプ本文 (システム文字コード)。空なら添付しない
 * @param screen_dump スクリーンショット HTML (システム文字コード)。空なら添付しない
 * @return JSON オブジェクト。構造は schema/ScoreSubmission.schema.json を参照
 */
nlohmann::json make_score_submission(CreatureEntity &creature, const std::string &character_dump, const std::string &screen_dump);

/*!
 * @brief 送信 ID (UUID v4) を生成する
 * @details サーバはこの値で再送の重複登録を抑止する。同じ送信内容の再送には同じ値を使うこと。
 */
std::string generate_submission_id();
