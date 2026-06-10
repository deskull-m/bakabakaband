#include "info-reader/terrain-reader.h"
#include "info-reader/feature-info-tokens-table.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "term/gameterm.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <algorithm>
#include <optional>
#include <string>
#include <string_view>

/*!
 * @brief テキストトークンを走査してフラグを一つ得る
 */
static bool grab_one_feat_flag(TerrainType &terrain, std::string_view what)
{
    if (EnumClassFlagGroup<TerrainCharacteristics>::grab_one_flag(terrain.flags, f_info_flags, what)) {
        return true;
    }

    msg_format(_("未知の地形フラグ '%s'。", "Unknown feature flag '%s'."), what.data());
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
static errr set_terrain_symbol(const nlohmann::json &symbol_obj, TerrainType &terrain)
{
    if (symbol_obj.is_null() || !symbol_obj.is_object()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto &ch_obj = symbol_obj["character"];
    const auto &color_obj = symbol_obj["color"];
    const auto &lit_obj = symbol_obj["lit"];

    if (!ch_obj.is_string() || !color_obj.is_string() || !lit_obj.is_boolean()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto ch_str = ch_obj.get<std::string>();
    if (ch_str.empty()) {
        return PARSE_ERROR_GENERIC;
    }

    const auto color_name = color_obj.get<std::string>();
    const auto it = color_list.find(color_name);
    if (it == color_list.end()) {
        msg_format(_("未知の色 '%s'。", "Unknown color '%s'."), color_name.c_str());
        return PARSE_ERROR_INVALID_FLAG;
    }
    if (it->second > 127) {
        return PARSE_ERROR_GENERIC;
    }

    const auto character = ch_str.front();
    const auto color = it->second;

    terrain.symbol_definitions[F_LIT_STANDARD] = { color, character };

    const bool lit = lit_obj.get<bool>();
    if (lit) {
        terrain.reset_lighting(false);
        return PARSE_ERROR_NONE;
    }

    for (int j = F_LIT_NS_BEGIN; j < F_LIT_MAX; j++) {
        terrain.symbol_definitions[j] = { color, character };
    }

    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Object から地形情報を 1 エントリ分読み取る
 * @param element 地形情報の JSON Object
 * @param head 使用しない
 * @return エラーコード
 */
errr parse_terrains_json_info(nlohmann::json &element, angband_header *)
{
    if (element.is_null() || !element.is_object()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    int id = 0;
    if (auto err = info_set_integer(element["id"], id, true, Range(0, 9999))) {
        return err;
    }

    const auto &key_obj = element["key"];
    if (!key_obj.is_string()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    const auto key = key_obj.get<std::string>();
    if (key.empty()) {
        return PARSE_ERROR_GENERIC;
    }

    auto &terrains = TerrainList::get_instance();
    if (id >= static_cast<int>(terrains.size())) {
        terrains.resize(id + 1);
    }

    error_idx = id;

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
    for (auto j = 0; j < MAX_FEAT_STATES; j++) {
        terrain.state[j].action = TerrainCharacteristics::MAX;
        terrain.state[j].result_tag.clear();
    }

    if (auto err = info_set_string(element["name"], terrain.name, true)) {
        return err;
    }

    if (auto err = set_terrain_symbol(element["symbol"], terrain)) {
        return err;
    }

    const auto &mimic_obj = element["mimic"];
    if (!mimic_obj.is_null()) {
        if (!mimic_obj.is_string()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
        terrain.mimic_tag = mimic_obj.get<std::string>();
    }

    const auto &prio_obj = element["map_priority"];
    if (!prio_obj.is_null()) {
        if (auto err = info_set_integer(prio_obj, terrain.priority, true, Range(-9999, 9999))) {
            return err;
        }
    }

    const auto &flags_obj = element["flags"];
    if (flags_obj.is_null() || !flags_obj.is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    std::optional<uint8_t> specific_type;
    for (const auto &f_obj : flags_obj) {
        if (!f_obj.is_string()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
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
            info_set_value(terrain.power, std::string(f.substr(sizeof("POWER_") - 1)));
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
            return PARSE_ERROR_INVALID_FLAG;
        }
    }
    if (specific_type && !terrain.set_specific_type(*specific_type)) {
        return PARSE_ERROR_INVALID_FLAG;
    }

    // bakabakaband 独自: 確率的ランダム地形変化 (R: ディレクティブ相当)
    const auto &random_change_obj = element["random_change"];
    if (!random_change_obj.is_null()) {
        if (!random_change_obj.is_object()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
        if (auto err = info_set_integer(random_change_obj["prob"], terrain.random_change_prob, true, Range(0, 99999))) {
            return err;
        }
        const auto &tag_obj = random_change_obj["tag"];
        if (!tag_obj.is_string()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
        terrain.random_change_tag = tag_obj.get<std::string>();
    }

    // 財宝地形 (HAS_GOLD) を掘った際に生成する財宝数 (省略時は従来通り 1 個)
    if (auto err = info_set_dice(element["DROP_GOLD"], terrain.gold_drop, false)) {
        return err;
    }

    const auto &inter_obj = element["interactions"];
    if (!inter_obj.is_null()) {
        if (!inter_obj.is_object()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        for (auto it = inter_obj.begin(); it != inter_obj.end(); ++it) {
            const auto action = it.key();
            const auto &value_obj = it.value();
            if (!value_obj.is_string()) {
                return PARSE_ERROR_TOO_FEW_ARGUMENTS;
            }
            const auto result_tag = value_obj.get<std::string>();
            if (result_tag.empty()) {
                return PARSE_ERROR_TOO_FEW_ARGUMENTS;
            }

            if (action == "DESTROYED") {
                terrain.destroyed_tag = result_tag;
                continue;
            }

            const auto state_it = std::find_if(std::begin(terrain.state), std::end(terrain.state),
                [](const auto &s) { return s.action == TerrainCharacteristics::MAX; });
            if (state_it == std::end(terrain.state)) {
                return PARSE_ERROR_GENERIC;
            }

            const auto idx_state = static_cast<int>(std::distance(std::begin(terrain.state), state_it));

            if (auto val = parse_terrain_action(action)) {
                terrain.state[idx_state].action = *val;
            } else {
                msg_format(_("未知の地形アクション '%s'。", "Unknown feature action '%s'."), action.data());
                return PARSE_ERROR_INVALID_FLAG;
            }
            terrain.state[idx_state].result_tag = result_tag;
        }
    }

    return PARSE_ERROR_NONE;
}
