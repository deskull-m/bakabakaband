/*!
 * @brief ゲームデータ初期化1 / Initialization (part 1) -BEN-
 * @date 2014/01/28
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * 2014 Deskull rearranged comment for Doxygen
 */

#include "info-reader/fixed-map-parser.h"
#include "dungeon/quest.h"
#include "floor/fixed-map-generator.h"
#include "game-option/birth-options.h"
#include "game-option/runtime-arguments.h"
#include "info-reader/general-parser.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/quest-reader.h"
#include "io/condition-expression.h"
#include "io/files-util.h"
#include "io/pref-file-expressor.h"
#include "locale/character-encoding.h"
#include "main/init-error-messages-table.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "player/player-realm.h"
#include "system/angband-exceptions.h"
#include "system/angband-system.h"
#include "system/creature-entity.h"
#include "system/dungeon/quest-definition.h"
#include "system/dungeon/quest-fixed-map.h"
#include "system/floor/floor-info.h"
#include "system/gamevalue.h"
#include "util/angband-files.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "world/world-collapsion.h"
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <limits>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

static char tmp[8];
static concptr variant = "ZANGBAND";

/*!
 * @brief 街の凡例セルの special 値が int16_t の範囲に収まる整数かを判定する
 * @param value JSON 値
 * @return 範囲内の整数なら true
 */
static bool is_valid_town_special(const nlohmann::json &value)
{
    if (!value.is_number_integer()) {
        return false;
    }

    constexpr auto minimum = std::numeric_limits<int16_t>::min();
    constexpr auto maximum = std::numeric_limits<int16_t>::max();
    if (value.is_number_unsigned()) {
        return value.get<uint64_t>() <= static_cast<uint64_t>(maximum);
    }

    const auto special = value.get<int64_t>();
    return special >= minimum && special <= maximum;
}

/*!
 * @brief 街の地形凡例 (TownPreferences.jsonc) を読み込み、共通の凡例テーブルに反映する
 * @return エラーコード
 * @details 個別の街定義はこの凡例を上書きできる。INIT_ONLY_BUILDINGS 時は読み込まない。
 */
static parse_error_type load_town_preferences()
{
    if (init_flags & INIT_ONLY_BUILDINGS) {
        return PARSE_ERROR_NONE;
    }

    std::ifstream ifs(path_build(ANGBAND_DIR_EDIT, TOWN_PREFERENCES));
    if (!ifs) {
        return PARSE_ERROR_GENERIC;
    }

    try {
        const auto data = nlohmann::json::parse(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>(), nullptr, true, true, true);
        if (!data.is_object() || !data.contains("version") || !data["version"].is_number_integer() || data["version"] != 1 ||
            !data.contains("legend") || !data["legend"].is_object() || data["legend"].empty()) {
            return PARSE_ERROR_INVALID_TYPE;
        }

        std::vector<std::pair<unsigned char, dungeon_grid>> legend;
        for (const auto &[symbol, cell_data] : data["legend"].items()) {
            if (symbol.size() != 1 || symbol.front() < '!' || symbol.front() > '~' || !cell_data.is_object() ||
                !cell_data.contains("terrain") || !cell_data["terrain"].is_string() || !cell_data.contains("caveInfo") || !cell_data["caveInfo"].is_array()) {
                return PARSE_ERROR_INVALID_TYPE;
            }
            for (const auto &flag : cell_data["caveInfo"]) {
                if (!flag.is_string()) {
                    return PARSE_ERROR_INVALID_TYPE;
                }
            }
            if (cell_data.contains("special") && !is_valid_town_special(cell_data["special"])) {
                return PARSE_ERROR_INVALID_VALUE;
            }

            QuestLegendCell cell;
            if (const auto err = parse_quest_legend_cell(cell_data, cell); err != PARSE_ERROR_NONE) {
                return err;
            }
            legend.emplace_back(static_cast<unsigned char>(symbol.front()), cell.grid);
        }

        for (const auto &[symbol, grid] : legend) {
            letter[symbol] = grid;
        }
        return PARSE_ERROR_NONE;
    } catch (const nlohmann::json::exception &) {
        return PARSE_ERROR_INVALID_VALUE;
    }
}

/*!
 * @brief 街定義リスト (TownDefinitionList.jsonc) から現在の街・荒野モードに対応する街マップファイル名を得る
 * @param creature クリーチャーへの参照 (現在の街番号を得るために使用)
 * @param map_file 選択された街マップファイル名の格納先 (該当街が無い場合は空のまま)
 * @return エラーコード
 * @details towns キーの各街は単一のマップファイル名、または荒野モード別
 * ({ lite / normal / none }) のオブジェクトで指定する。vanilla_town なら none、
 * lite_town なら lite、それ以外は normal を選ぶ。
 */
static parse_error_type load_town_definition_file(CreatureEntity &creature, std::string &map_file)
{
    std::ifstream ifs(path_build(ANGBAND_DIR_EDIT, TOWN_DEFINITION_LIST));
    if (!ifs) {
        return PARSE_ERROR_GENERIC;
    }

    try {
        const auto data = nlohmann::json::parse(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>(), nullptr, true, true, true);
        if (!data.is_object() || !data.contains("version") || !data["version"].is_number_integer() || data["version"] != 1 ||
            !data.contains("towns") || !data["towns"].is_object() || data["towns"].empty()) {
            return PARSE_ERROR_INVALID_TYPE;
        }

        const auto &towns = data["towns"];
        const auto town = towns.find(std::to_string(creature.get_town_num()));
        if (town == towns.end()) {
            return PARSE_ERROR_NONE;
        }

        const nlohmann::json *selected = &*town;
        if (selected->is_object()) {
            const auto *mode = vanilla_town ? "none" : (lite_town ? "lite" : "normal");
            const auto file = selected->find(mode);
            if (file == selected->end()) {
                return PARSE_ERROR_INVALID_TYPE;
            }
            selected = &*file;
        }

        if (!selected->is_string()) {
            return PARSE_ERROR_INVALID_TYPE;
        }
        map_file = selected->get<std::string>();
        if (!map_file.starts_with("towns/") || !map_file.ends_with(".txt") || map_file.find("..") != std::string::npos ||
            map_file.find('\\') != std::string::npos) {
            return PARSE_ERROR_INVALID_VALUE;
        }
        return PARSE_ERROR_NONE;
    } catch (const nlohmann::json::exception &) {
        return PARSE_ERROR_INVALID_VALUE;
    }
}

/*!
 * @brief 町のマップの条件式の変数の値を返す
 * @param creature クリーチャーへの参照
 * @param name 先頭の「$」を除いた変数名
 * @return 変数の値。知らない変数なら tl::nullopt
 */
static tl::optional<std::string> resolve_town_map_variable(CreatureEntity &creature, std::string_view name)
{
    if (name == "TOWN") {
        return std::to_string(creature.get_town_num());
    }
    if (name == "LEVEL") {
        return std::to_string(creature.get_level());
    }
    if (name == "QUEST_NUMBER") {
        return std::to_string(enum2i(creature.get_floor()->quest_number));
    }
    if (name == "LEAVING_QUEST") {
        return std::to_string(enum2i(leaving_quest));
    }
    // 接頭辞の後ろの番号のクエストを返す。不正な番号・未定義のクエストは nullptr
    const auto get_quest_after = [name](std::string_view prefix) -> const QuestType * {
        const auto number = parse_condition_expression_integer(name.substr(prefix.length()));
        if (!number || !std::in_range<std::underlying_type_t<QuestId>>(*number)) {
            return nullptr;
        }
        const auto id = i2enum<QuestId>(*number);
        const auto &quests = QuestList::get_instance();
        return quests.contains(id) ? &quests.get_quest(id) : nullptr;
    };
    constexpr std::string_view quest_type_prefix = "QUEST_TYPE";
    if (name.starts_with(quest_type_prefix)) {
        const auto *quest = get_quest_after(quest_type_prefix);
        if (quest == nullptr) {
            return tl::nullopt;
        }
        return std::to_string(enum2i(quest->type));
    }
    constexpr std::string_view quest_prefix = "QUEST";
    if (name.starts_with(quest_prefix)) {
        const auto *quest = get_quest_after(quest_prefix);
        if (quest == nullptr) {
            return tl::nullopt;
        }
        return std::to_string(enum2i(quest->status));
    }
    constexpr std::string_view random_prefix = "RANDOM";
    if (name.starts_with(random_prefix)) {
        const auto divisor = parse_condition_expression_integer(name.substr(random_prefix.length()));
        if (!divisor || (*divisor <= 0)) {
            return tl::nullopt;
        }
        const auto &system = AngbandSystem::get_instance();
        // 符号付きへ変換すると seed の最上位ビットが立つ場合に剰余が負になるため、符号なしのまま剰余を取る
        return std::to_string(system.get_seed_town() % static_cast<uint32_t>(*divisor));
    }
    if (name == "VARIANT") {
        return variant;
    }
    if (name == "WILDERNESS") {
        if (vanilla_town) {
            return "NONE";
        }
        if (lite_town) {
            return "LITE";
        }
        return "NORMAL";
    }
    if (name == "WORLD_COLLAPSE") {
        return std::to_string(wc_ptr->collapse_degree);
    }
    if (name == "IRONMAN_DOWNWARD") {
        return ironman_downward ? "1" : "0";
    }

    return resolve_common_expression_variable(creature, name);
}

/*!
 * @brief 固定マップ (クエスト＆街＆広域マップ)をq_info、t_info、w_infoから読み込んでパースする
 * @param creature クリーチャーへの参照
 * @param name ファイル名
 * @param ymin 詳細不明
 * @param xmin 詳細不明
 * @param ymax 詳細不明
 * @param xmax 詳細不明
 * @return エラーコード
 */
parse_error_type parse_fixed_map(CreatureEntity &creature, std::string_view name, int ymin, int xmin, int ymax, int xmax)
{
    if (name == TOWN_DEFINITION_LIST) {
        if (const auto err = load_town_preferences(); err != PARSE_ERROR_NONE) {
            const auto oops = (((err > 0) && (err < PARSE_ERROR_MAX)) ? err_str[err] : "unknown");
            msg_print(format("Error %d (%s) loading '%s'.", enum2i(err), oops, TOWN_PREFERENCES));
            msg_erase();
            return err;
        }

        std::string map_file;
        if (const auto err = load_town_definition_file(creature, map_file); err != PARSE_ERROR_NONE) {
            const auto oops = (((err > 0) && (err < PARSE_ERROR_MAX)) ? err_str[err] : "unknown");
            msg_print(format("Error %d (%s) loading '%s'.", enum2i(err), oops, TOWN_DEFINITION_LIST));
            msg_erase();
            return err;
        }
        if (map_file.empty()) {
            return PARSE_ERROR_NONE;
        }
        const auto err = parse_fixed_map(creature, map_file, ymin, xmin, ymax, xmax);
        if (err != PARSE_ERROR_NONE) {
            const auto oops = (((err > 0) && (err < PARSE_ERROR_MAX)) ? err_str[err] : "unknown");
            msg_print(format("Error %d (%s) loading '%s'.", enum2i(err), oops, map_file.data()));
            msg_erase();
        }
        return err;
    }

    const auto path = path_build(ANGBAND_DIR_EDIT, name);
    auto *fp = angband_fopen(path, FileOpenMode::READ);
    if (fp == nullptr) {
        return PARSE_ERROR_GENERIC;
    }

    int num = -1;
    parse_error_type err = PARSE_ERROR_NONE;
    bool bypass = false;
    int x = xmin;
    int y = ymin;
    qtwg_type tmp_qg;
    qtwg_type *qg_ptr = initialize_quest_generator_type(&tmp_qg, ymin, xmin, ymax, xmax, &y, &x);
    while (true) {
        auto line_str = angband_fgets(fp);
        if (!line_str) {
            break;
        }
        num++;
        if (line_str->empty() || iswspace(line_str->front()) || line_str->starts_with(('#'))) {
            continue;
        }

        if (line_str->starts_with("?:")) {
            const auto resolve = [&creature](std::string_view name) { return resolve_town_map_variable(creature, name); };
            bypass = evaluate_condition_expression(std::string_view(*line_str).substr(2), resolve) == "0";
            continue;
        }

        if (bypass) {
            continue;
        }

        qg_ptr->buf = line_str->data();
        err = generate_fixed_map_floor(creature, qg_ptr, parse_fixed_map);
        if (err != PARSE_ERROR_NONE) {
            concptr oops = (((err > 0) && (err < PARSE_ERROR_MAX)) ? err_str[err] : "unknown");
            msg_format("Error %d (%s) at line %d of '%s'.", err, oops, num, name.data());
            msg_format(_("'%s'を解析中。", "Parsing '%s'."), line_str->data());
            msg_erase();
            break;
        }
    }

    angband_fclose(fp);
    return err;
}
