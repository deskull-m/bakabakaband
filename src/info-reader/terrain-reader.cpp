#include "info-reader/terrain-reader.h"
#include "info-reader/feature-info-tokens-table.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/symbol-reader-util.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "view/display-messages.h"
#include <algorithm>
#include <cstdint>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>

namespace {
// 縮小マップの byte 変換で折り返さず、同順位の +1 後も自動拾い印の 127 未満にする。
constexpr int MAX_TERRAIN_MAP_PRIORITY = 125;
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る
 */
static bool grab_one_feat_flag(TerrainType &terrain, std::string_view what)
{
    if (EnumClassFlagGroup<TerrainCharacteristics>::grab_one_flag(terrain.flags, f_info_flags, what)) {
        return true;
    }

    msg_print(_("未知の地形フラグ '{}'。", "Unknown feature flag '{}'."), what);
    return false;
}

/*!
 * @brief テキストトークンから地形アクション (TerrainCharacteristics) を取得する
 */
static std::optional<TerrainCharacteristics> parse_terrain_action(std::string_view what)
{
    if (auto it = f_info_flags.find(what); it != f_info_flags.end()) {
        return it->second;
    }

    return std::nullopt;
}

/*!
 * @brief JSON Objectから地形シンボルをセットする
 */
static ParseErrorType set_terrain_symbol(const nlohmann::json &symbol_obj, TerrainType &terrain)
{
    if (symbol_obj.is_null() || !symbol_obj.is_object()) {
        return ParseErrorType::TOO_FEW_ARGUMENTS;
    }

    const auto &ch_obj = symbol_obj["character"];
    const auto &color_obj = symbol_obj["color"];
    const auto &lit_obj = symbol_obj["lit"];

    if (!ch_obj.is_string() || !color_obj.is_string() || !lit_obj.is_boolean()) {
        return ParseErrorType::TOO_FEW_ARGUMENTS;
    }

    const auto symbol = info_read_symbol(symbol_obj);
    if (!symbol) {
        if (symbol.error() == ParseErrorType::INVALID_FLAG) {
            msg_print(_("未知の色 '{}'。", "Unknown color '{}'."), color_obj.get<std::string>());
        }
        return symbol.error();
    }

    terrain.symbol_definitions[F_LIT_STANDARD] = *symbol;

    const bool lit = lit_obj.get<bool>();
    if (lit) {
        terrain.reset_lighting(false);
        return ParseErrorType::NONE;
    }

    for (int j = F_LIT_NS_BEGIN; j < F_LIT_MAX; j++) {
        terrain.symbol_definitions[j] = *symbol;
    }

    return ParseErrorType::NONE;
}

/*!
 * @brief 生成時の確率的地形変化 (generation.changes) を読み取る (上流 #5393 相当)
 * @param generation_obj generation を表す JSON 値
 * @param terrain 地形情報への参照
 * @return エラーコード
 */
static ParseErrorType set_terrain_generation(const nlohmann::json &generation_obj, TerrainType &terrain)
{
    if (generation_obj.is_null()) {
        return ParseErrorType::NONE;
    }
    if (!generation_obj.is_object()) {
        return ParseErrorType::TOO_FEW_ARGUMENTS;
    }

    const auto &changes_obj = generation_obj["changes"];
    if (!changes_obj.is_array()) {
        return ParseErrorType::TOO_FEW_ARGUMENTS;
    }

    auto probability_sum = 0;
    for (const auto &change_obj : changes_obj) {
        if (!change_obj.is_object()) {
            return ParseErrorType::TOO_FEW_ARGUMENTS;
        }

        const auto &terrain_obj = change_obj["terrain"];
        if (!terrain_obj.is_string()) {
            return ParseErrorType::TOO_FEW_ARGUMENTS;
        }

        auto change = TerrainGenerationChange{};
        change.result_tag = terrain_obj.get<std::string>();
        if (change.result_tag.empty()) {
            return ParseErrorType::TOO_FEW_ARGUMENTS;
        }

        if (const auto err = info_set_integer(change_obj["probability"], change.probability, true, Range(1, 100)); !err) {
            return err.error();
        }

        probability_sum += change.probability;
        if (probability_sum > 100) {
            return ParseErrorType::INVALID_VALUE;
        }

        terrain.generation_changes.push_back(change);
    }

    return ParseErrorType::NONE;
}

/*!
 * @brief JSON Object から地形情報を 1 エントリ分読み取る
 * @param element 地形情報の JSON Object
 * @return エラーコード
 */
ParseErrorType parse_terrains_json_info(nlohmann::json &element, TerrainReaderSession &session)
{
    if (element.is_null() || !element.is_object()) {
        return ParseErrorType::TOO_FEW_ARGUMENTS;
    }

    int id = 0;
    if (const auto err = info_set_integer(element["id"], id, true, Range(0, 9999)); !err) {
        return err.error();
    }

    const auto &key_obj = element["key"];
    if (!key_obj.is_string()) {
        return ParseErrorType::TOO_FEW_ARGUMENTS;
    }
    const auto key = key_obj.get<std::string>();
    if (key.empty()) {
        return ParseErrorType::GENERIC;
    }

    // 同じ ID・タグを 2 つ書くと、エラーにならず上書き・取り違えになるので弾く
    if (session.ids.contains(id)) {
        msg_print(_("地形ID '{}' が重複しています。", "Duplicate terrain ID '{}'."), id);
        return ParseErrorType::NON_SEQUENTIAL_RECORDS;
    }
    if (session.keys.contains(key)) {
        msg_print(_("地形タグ '{}' が重複しています。", "Duplicate terrain tag '{}'."), key);
        return ParseErrorType::INVALID_VALUE;
    }

    auto &terrains = TerrainList::get_instance();
    if (id >= static_cast<int>(terrains.size())) {
        terrains.resize(id + 1);
    }

    const auto s = static_cast<short>(id);
    auto &terrain = terrains.get_terrain(s);

    terrain.idx = s;
    terrain.tag = key;

    const auto tag_it = terrain_tags.find(key);
    if (tag_it != terrain_tags.end()) {
        terrain.tag_enum = tag_it->second;
    }

    terrain.mimic = s;
    terrain.destroyed = s;
    terrain.gold_drop = Dice{};
    terrain.generation_changes.clear();
    terrain.door_power = 0;
    terrain.trap_power = 0;
    terrain.tunnel_power = 0;
    for (auto j = 0; j < MAX_FEAT_STATES; j++) {
        terrain.state[j].action = TerrainCharacteristics::MAX;
        terrain.state[j].result_tag.clear();
    }

    if (const auto err = info_set_string(element["name"], terrain.name, true); !err) {
        return err.error();
    }

    if (const auto err = set_terrain_symbol(element["symbol"], terrain); err != ParseErrorType::NONE) {
        return err;
    }

    const auto &mimic_obj = element["mimic"];
    if (!mimic_obj.is_null()) {
        if (!mimic_obj.is_string()) {
            return ParseErrorType::TOO_FEW_ARGUMENTS;
        }
        terrain.mimic_tag = mimic_obj.get<std::string>();
    }

    const auto &prio_obj = element["map_priority"];
    if (!prio_obj.is_null()) {
        if (const auto err = info_set_integer(prio_obj, terrain.priority, true, Range(0, MAX_TERRAIN_MAP_PRIORITY)); !err) {
            return err.error();
        }
    }

    const auto &flags_obj = element["flags"];
    if (flags_obj.is_null()) {
        return ParseErrorType::TOO_FEW_ARGUMENTS;
    }
    // フラグ配列の型の誤りは、リーダー間で揃えて INVALID_TYPE を返す
    if (!flags_obj.is_array()) {
        return ParseErrorType::INVALID_TYPE;
    }

    std::optional<uint8_t> specific_type;
    std::optional<uint8_t> terrain_power;
    for (const auto &f_obj : flags_obj) {
        if (!f_obj.is_string()) {
            return ParseErrorType::INVALID_TYPE;
        }

        const auto f = f_obj.get<std::string>();
        if (f.empty()) {
            continue;
        }

        if (f.starts_with("SUBTYPE_")) {
            uint8_t parsed_specific_type{};
            info_set_value(parsed_specific_type, f.substr(sizeof("SUBTYPE_") - 1));
            specific_type = parsed_specific_type;
            continue;
        }
        if (f.starts_with("POWER_")) {
            uint8_t parsed_power{};
            info_set_value(parsed_power, std::string(f.substr(sizeof("POWER_") - 1)));
            terrain_power = parsed_power;
            continue;
        }
        // bakabakaband 独自フラグプレフィックス
        if (f.starts_with("HYGIENE_")) {
            info_set_value(terrain.hygiene, std::string(f.substr(sizeof("HYGIENE_") - 1)));
            continue;
        }
        if (f.starts_with("C-PRIORITY_")) {
            info_set_value(terrain.change_priority, std::string(f.substr(sizeof("C-PRIORITY_") - 1)));
            continue;
        }

        if (!grab_one_feat_flag(terrain, f)) {
            return ParseErrorType::INVALID_FLAG;
        }
    }
    if (specific_type && !terrain.set_specific_type(*specific_type)) {
        return ParseErrorType::INVALID_FLAG;
    }

    // 単一の POWER 値を、地形が持つ特性フラグに応じて用途別 power へ割り当てる。
    if (terrain_power) {
        if (terrain.flags.has(TerrainCharacteristics::DOOR)) {
            terrain.door_power = *terrain_power;
        }
        if (terrain.flags.has(TerrainCharacteristics::TRAP)) {
            terrain.trap_power = *terrain_power;
        }
        if (terrain.flags.has(TerrainCharacteristics::TUNNEL)) {
            terrain.tunnel_power = *terrain_power;
        }
    }

    // bakabakaband 独自: 確率的ランダム地形変化 (R: ディレクティブ相当)
    const auto &random_change_obj = element["random_change"];
    if (!random_change_obj.is_null()) {
        if (!random_change_obj.is_object()) {
            return ParseErrorType::TOO_FEW_ARGUMENTS;
        }
        if (const auto err = info_set_integer(random_change_obj["prob"], terrain.random_change_prob, true, Range(0, 99999)); !err) {
            return err.error();
        }
        const auto &tag_obj = random_change_obj["tag"];
        if (!tag_obj.is_string()) {
            return ParseErrorType::TOO_FEW_ARGUMENTS;
        }
        terrain.random_change_tag = tag_obj.get<std::string>();
    }

    // 財宝地形 (HAS_GOLD) を掘った際に生成する財宝数 (省略時は従来通り 1 個)
    const auto &drop_gold_obj = element["DROP_GOLD"];
    if (const auto err = info_set_dice(drop_gold_obj, terrain.gold_drop, false); !err) {
        return err.error();
    }
    // ダイスを解析できても個数・面数が 0 以下なら黙って捨てず不正として弾く (上流 hengband#6036 相当)
    if (!drop_gold_obj.is_null() && !terrain.gold_drop.is_valid()) {
        msg_print(_("金貨ドロップのダイスが不正。", "Invalid gold drop dice."));
        return ParseErrorType::INVALID_VALUE;
    }

    // 生成時の確率的地形変化 (上流 #5393 相当)
    if (const auto err = set_terrain_generation(element["generation"], terrain); err != ParseErrorType::NONE) {
        return err;
    }

    const auto &inter_obj = element["interactions"];
    if (!inter_obj.is_null()) {
        if (!inter_obj.is_object()) {
            return ParseErrorType::TOO_FEW_ARGUMENTS;
        }

        for (auto it = inter_obj.begin(); it != inter_obj.end(); ++it) {
            const auto action = it.key();
            const auto &value_obj = it.value();
            if (!value_obj.is_string()) {
                return ParseErrorType::TOO_FEW_ARGUMENTS;
            }
            const auto result_tag = value_obj.get<std::string>();
            if (result_tag.empty()) {
                return ParseErrorType::TOO_FEW_ARGUMENTS;
            }

            if (action == "DESTROYED") {
                terrain.destroyed_tag = result_tag;
                continue;
            }

            const auto state_it = std::find_if(std::begin(terrain.state), std::end(terrain.state),
                [](const auto &s) { return s.action == TerrainCharacteristics::MAX; });
            if (state_it == std::end(terrain.state)) {
                return ParseErrorType::GENERIC;
            }

            const auto idx_state = static_cast<int>(std::distance(std::begin(terrain.state), state_it));

            if (auto val = parse_terrain_action(action)) {
                terrain.state[idx_state].action = *val;
            } else {
                msg_print(_("未知の地形アクション '{}'。", "Unknown feature action '{}'."), action);
                return ParseErrorType::INVALID_FLAG;
            }
            terrain.state[idx_state].result_tag = result_tag;
        }
    }

    session.ids.insert(id);
    session.keys.insert(key);
    return ParseErrorType::NONE;
}
