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
#include "io/files-util.h"
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
 * @brief 固定マップ (クエスト＆街＆広域マップ)生成時の分岐処理
 * Helper function for "parse_fixed_map()"
 * @param creature クリーチャーへの参照
 * @param sp
 * @param fp
 * @return エラーコード
 */
static std::string parse_fixed_map_expression(CreatureEntity &creature, char **sp, char *fp)
{
    constexpr char b1 = '[';
    constexpr char b2 = ']';

    char f = ' ';

    char *s = (*sp);

    while (iswspace(*s)) {
        s++;
    }

    char *b = s;
    std::string v = "?o?o?";
    if (*s == b1) {
        std::string t;
        s++;
        t = parse_fixed_map_expression(creature, &s, &f);
        if (t.empty()) {
            /* Nothing */
        } else if (t == "IOR") {
            v = "0";
            while (*s && (f != b2)) {
                t = parse_fixed_map_expression(creature, &s, &f);
                if (!t.empty() && t != "0") {
                    v = "1";
                }
            }
        } else if (t == "AND") {
            v = "1";
            while (*s && (f != b2)) {
                t = parse_fixed_map_expression(creature, &s, &f);
                if (!t.empty() && t == "0") {
                    v = "0";
                }
            }
        } else if (t == "NOT") {
            v = "1";
            while (*s && (f != b2)) {
                t = parse_fixed_map_expression(creature, &s, &f);
                if (!t.empty() && t == "1") {
                    v = "0";
                }
            }
        } else if (t == "EQU") {
            v = "0";
            if (*s && (f != b2)) {
                t = parse_fixed_map_expression(creature, &s, &f);
            }

            while (*s && (f != b2)) {
                auto p = parse_fixed_map_expression(creature, &s, &f);
                if (t == p) {
                    v = "1";
                }
            }
        } else if (t == "LEQ") {
            v = "1";
            if (*s && (f != b2)) {
                t = parse_fixed_map_expression(creature, &s, &f);
            }

            while (*s && (f != b2)) {
                auto p = parse_fixed_map_expression(creature, &s, &f);
                if (!p.empty() && atoi(t.data()) > atoi(p.data())) {
                    v = "0";
                }
            }
        } else if (t == "GEQ") {
            v = "1";
            if (*s && (f != b2)) {
                t = parse_fixed_map_expression(creature, &s, &f);
            }

            while (*s && (f != b2)) {
                auto p = parse_fixed_map_expression(creature, &s, &f);
                if (!p.empty() && atoi(t.data()) < atoi(p.data())) {
                    v = "0";
                }
            }
        } else {
            while (*s && (f != b2)) {
                t = parse_fixed_map_expression(creature, &s, &f);
            }
        }

        if (f != b2) {
            v = "?x?x?";
        }
        if ((f = *s) != '\0') {
            *s++ = '\0';
        }

        (*fp) = f;
        (*sp) = s;
        return v;
    }

#ifdef JP
    while (iskanji(*s) || (isprint(*s) && !angband_strchr(" []", *s))) {
        if (iskanji(*s)) {
            s++;
        }
        s++;
    }
#else
    while (isprint(*s) && !angband_strchr(" []", *s)) {
        ++s;
    }
#endif
    if ((f = *s) != '\0') {
        *s++ = '\0';
    }

    if (*b != '$') {
        v = b;
        (*fp) = f;
        (*sp) = s;
        return v;
    }

    if (streq(b + 1, "SYS")) {
        v = ANGBAND_SYS;
    } else if (streq(b + 1, "GRAF")) {
        v = ANGBAND_GRAF;
    } else if (streq(b + 1, "MONOCHROME")) {
        if (arg_monochrome) {
            v = "ON";
        } else {
            v = "OFF";
        }
    } else if (streq(b + 1, "RACE")) {
        v = creature.get_race_info()->title.en_string();
    } else if (streq(b + 1, "CLASS")) {
        v = (*creature.get_class_info()).title.en_string();
    } else if (streq(b + 1, "REALM1")) {
        v = PlayerRealm(creature).realm1().get_name().en_string();
    } else if (streq(b + 1, "REALM2")) {
        v = PlayerRealm(creature).realm2().get_name().en_string();
    } else if (streq(b + 1, "PLAYER")) {
        char tmp_player_name[64]{};
        const char *pn = creature.name.c_str();
        char *tpn = tmp_player_name;
        for (; *pn; pn++, tpn++) {
#ifdef JP
            if (iskanji(*pn)) {
                *(tpn++) = *(pn++);
                *tpn = *pn;
                continue;
            }
#endif
            *tpn = angband_strchr(" []", *pn) ? '_' : *pn;
        }

        *tpn = '\0';
        v = tmp_player_name;
    } else if (streq(b + 1, "TOWN")) {
        v = std::to_string(creature.get_town_num());
    } else if (streq(b + 1, "LEVEL")) {
        v = std::to_string(creature.get_level());
    } else if (streq(b + 1, "QUEST_NUMBER")) {
        v = std::to_string(enum2i(creature.get_floor()->quest_number));
    } else if (streq(b + 1, "LEAVING_QUEST")) {
        v = std::to_string(enum2i(leaving_quest));
    } else if (prefix(b + 1, "QUEST_TYPE")) {
        const auto &quests = QuestList::get_instance();
        v = std::to_string(enum2i(quests.get_quest(i2enum<QuestId>(atoi(b + 11))).type));
    } else if (prefix(b + 1, "QUEST")) {
        const auto &quests = QuestList::get_instance();
        v = std::to_string(enum2i(quests.get_quest(i2enum<QuestId>(atoi(b + 6))).status));
    } else if (prefix(b + 1, "RANDOM")) {
        const auto &system = AngbandSystem::get_instance();
        v = std::to_string((static_cast<int>(system.get_seed_town()) % std::stoi(b + 7)));
    } else if (streq(b + 1, "VARIANT")) {
        v = variant;
    } else if (streq(b + 1, "WILDERNESS")) {
        if (vanilla_town) {
            sprintf(tmp, "NONE");
        } else if (lite_town) {
            sprintf(tmp, "LITE");
        } else {
            sprintf(tmp, "NORMAL");
        }
        v = tmp;
    } else if (streq(b + 1, "WORLD_COLLAPSE")) {
        v = std::to_string(wc_ptr->collapse_degree);
    } else if (streq(b + 1, "IRONMAN_DOWNWARD")) {
        v = (ironman_downward ? "1" : "0");
    }

    (*fp) = f;
    (*sp) = s;
    return v;
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
            char f;
            auto *s = line_str->data() + 2;
            auto v = parse_fixed_map_expression(creature, &s, &f);
            bypass = v == "0";
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
