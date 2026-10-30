#pragma once

#include "info-reader/parse-error-types.h"
#include "system/angband.h"
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <unordered_set>

/*!
 * @brief 1 回の地形定義の読み込み (init_terrains_info) ごとに保持する、登録済みの ID・タグ
 * @details 同じ ID・タグを 2 つ書いたときに、エラーにせず上書き・取り違えになるのを防ぐための重複検出用。
 */
struct TerrainReaderSession {
    std::unordered_set<int> ids;
    std::unordered_set<std::string> keys;
};

ParseErrorType parse_terrains_json_info(nlohmann::json &element, TerrainReaderSession &session);
