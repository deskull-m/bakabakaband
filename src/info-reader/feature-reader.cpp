#include "info-reader/feature-reader.h"
#include "floor/wild.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "info-reader/feature-info-tokens-table.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "main/angband-headers.h"
#include "room/door-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "term/gameterm.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

/*!
 * @brief テキストトークンを走査してフラグを一つ得る（地形情報向け） /
 * Grab one flag in an TerrainType from a textual string
 * @param f_ptr 地形情報を保管する先の構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return 見つけたらtrue
 */
static bool grab_one_feat_flag(TerrainType *f_ptr, std::string_view what)
{
    if (EnumClassFlagGroup<TerrainCharacteristics>::grab_one_flag(f_ptr->flags, f_info_flags, what)) {
        return true;
    }

    msg_format(_("未知の地形フラグ '%s'。", "Unknown feature flag '%s'."), what.data());
    return false;
}

/*!
 * @brief テキストトークンを走査してフラグ(ステート)を一つ得る（地形情報向け2） /
 * Grab an action in an TerrainType from a textual string
 * @param f_ptr 地形情報を保管する先の構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @param count ステートの保存先ID
 * @return 見つけたらtrue
 */
static bool grab_one_feat_action(TerrainType *f_ptr, std::string_view what, int count)
{
    if (auto it = f_info_flags.find(what); it != f_info_flags.end()) {
        f_ptr->state[count].action = it->second;
        return true;
    }

    msg_format(_("未知の地形アクション '%s'。", "Unknown feature action '%s'."), what.data());
    return false;
}

/*!
 * @brief 地形情報(TerrainDefinitions)のパース関数
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_terrains_info(std::string_view buf, angband_header *)
{
    const auto &tokens = str_split(buf, ':', false, 10);
    auto &terrains = TerrainList::get_instance();

    // N:index:tag
    if (tokens[0] == "N") {
        if (tokens.size() < 3) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        if (tokens[1].size() == 0 || tokens[2].size() == 0) {
            return PARSE_ERROR_GENERIC;
        }

        const auto i = std::stoi(tokens[1]);
        if (i < error_idx) {
            return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
        }

        if (i >= static_cast<int>(TerrainList::get_instance().size())) {
            terrains.resize(i + 1);
        }

        error_idx = i;
        const auto s = static_cast<short>(i);
        auto &terrain = terrains.get_terrain(s);
        terrain.idx = s;
        const auto &tag = tokens[2];
        terrain.tag = tag;

        //!< @todo 後でif文は消す.
        static const auto tag_begin = terrain_tags.begin();
        static const auto tag_end = terrain_tags.end();
        const auto tag_enum = std::find_if(tag_begin, tag_end, [&tag](const auto &x) { return x.first == tag; });
        if (tag_enum != tag_end) {
            terrain.tag_enum = tag_enum->second;
        }

        terrain.mimic = s;
        terrain.destroyed = s;
        for (auto j = 0; j < MAX_FEAT_STATES; j++) {
            terrain.state[j].action = TerrainCharacteristics::MAX;
        }

        return PARSE_ERROR_NONE;
    }

    if (terrains.empty()) {
        return PARSE_ERROR_MISSING_RECORD_HEADER;
    }

    // J:name_ja, E:name_en
    if (tokens[0] == _("J", "E")) {
        if (tokens.size() < 2 || tokens[1].size() == 0) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        auto &terrain = *terrains.rbegin();
        terrain.name = tokens[1];
        return PARSE_ERROR_NONE;
    }

    // pass
    if (tokens[0] == _("E", "J")) {
        return PARSE_ERROR_NONE;
    }

    // M:mimic_tag
    if (tokens[0] == "M") {
        if (tokens.size() < 2 || tokens[1].size() == 0) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        auto &terrain = *terrains.rbegin();
        terrain.mimic_tag = tokens[1];
        return PARSE_ERROR_NONE;
    }

    // G:symbol:color:lite:lite_symbol:lite_color:dark_symbol:dark_color
    if (tokens[0] == "G") {
        if (tokens.size() < 3) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        size_t n;
        char character;
        if (tokens[1].size() == 1) {
            character = tokens[1][0];
            n = 2;
        } else if (tokens[1].size() == 0 && tokens[2].size() == 0) {
            if (tokens.size() < 4) {
                return PARSE_ERROR_TOO_FEW_ARGUMENTS;
            }

            character = ':';
            n = 3;
        } else {
            return PARSE_ERROR_GENERIC;
        }

        const auto color = color_char_to_attr(tokens[n++][0]);
        if (color > 127) {
            return PARSE_ERROR_GENERIC;
        }

        auto &terrain = *terrains.rbegin();
        terrain.symbol_definitions[F_LIT_STANDARD] = { color, character };
        if (tokens.size() == n) {
            for (int j = F_LIT_NS_BEGIN; j < F_LIT_MAX; j++) {
                terrain.symbol_definitions[j] = { color, character };
            }

            return PARSE_ERROR_NONE;
        }

        if (tokens[n++] == "LIT") {
            terrain.reset_lighting(false);
            for (auto j = F_LIT_NS_BEGIN; j < F_LIT_MAX; j++) {
                auto c_idx = n + (j - F_LIT_NS_BEGIN) * 2;
                auto a_idx = c_idx + 1;
                if (tokens.size() <= (size_t)a_idx) {
                    continue;
                }

                if (tokens[c_idx].size() != 1 || tokens[a_idx].size() != 1) {
                    continue;
                }

                terrain.symbol_definitions[j].character = tokens[c_idx][0];
                if (tokens[a_idx] == "*") {
                    continue;
                }

                if (tokens[a_idx] == "-") {
                    terrain.symbol_definitions[j].color = color;
                    continue;
                }

                terrain.symbol_definitions[j].color = color_char_to_attr(tokens[a_idx][0]);
                if (terrain.symbol_definitions[j].color > 127) {
                    return PARSE_ERROR_GENERIC;
                }
            }

            return PARSE_ERROR_NONE;
        }

        return PARSE_ERROR_GENERIC;
    }

    // F:flags
    if (tokens[0] == "F") {
        if (tokens.size() < 2 || tokens[1].size() == 0) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto &flags = str_split(tokens[1], '|', true, 10);
        for (const auto &f : flags) {
            if (f.size() == 0) {
                continue;
            }

            const auto &f_tokens = str_split(f, '_', false, 2);
            auto &terrain = *terrains.rbegin();
            if (f_tokens.size() == 2) {
                if (f_tokens[0] == "SUBTYPE") {
                    info_set_value(terrain.subtype, f_tokens[1]);
                    continue;
                }

                if (f_tokens[0] == "POWER") {
                    info_set_value(terrain.power, f_tokens[1]);
                    continue;
                } else if (f_tokens[0] == "HYGIENE") {
                    info_set_value(terrain.hygiene, f_tokens[1]);
                    continue;
                } else if (f_tokens[0] == "C-PRIORITY") {
                    info_set_value(terrain.change_priority, f_tokens[1]);
                    continue;
                }
            }

            if (!grab_one_feat_flag(&terrain, f)) {
                return PARSE_ERROR_INVALID_FLAG;
            }
        }

        return PARSE_ERROR_NONE;
    }

    // W:priority
    if (tokens[0] == "W") {
        if (tokens.size() < 2 || tokens[1].size() == 0) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        auto &terrain = *terrains.rbegin();
        info_set_value(terrain.priority, tokens[1]);
        return PARSE_ERROR_NONE;
    }

    // K:state:feat
    if (tokens[0] == "K") {
        if (tokens.size() < 3) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        if (tokens[1].size() == 0 || tokens[2].size() == 0) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        auto &terrain = *terrains.rbegin();
        auto i = 0;
        for (; i < MAX_FEAT_STATES; i++) {
            if (terrain.state[i].action == TerrainCharacteristics::MAX) {
                break;
            }
        }

        if (i == MAX_FEAT_STATES) {
            return PARSE_ERROR_GENERIC;
        }

        if (tokens[1] == "DESTROYED") {
            terrain.destroyed_tag = tokens[2];
            return PARSE_ERROR_NONE;
        }

        if (!grab_one_feat_action(&terrain, tokens[1], i)) {
            return PARSE_ERROR_INVALID_FLAG;
        }

        terrain.state[i].result_tag = tokens[2];
        return PARSE_ERROR_NONE;
    }

    return PARSE_ERROR_UNDEFINED_DIRECTIVE;
}

/*!
 * @brief 地形の汎用定義をタグを通じて取得する /
 * Initialize feature variables
 */
void init_feat_variables()
{
    auto &terrains = TerrainList::get_instance();
    for (const auto &tag : terrain_tags) {
        terrains.emplace_tag(tag.first);
    }

    feat_door[DOOR_DOOR].open = terrains.get_terrain_id_by_tag("OPEN_DOOR");
    feat_door[DOOR_DOOR].broken = terrains.get_terrain_id_by_tag("BROKEN_DOOR");
    feat_door[DOOR_DOOR].closed = terrains.get_terrain_id_by_tag("CLOSED_DOOR");

    /* Locked doors */
    for (auto i = 1; i < MAX_LJ_DOORS; i++) {
        feat_door[DOOR_DOOR].locked[i - 1] = terrains.get_terrain_id_by_tag(format("LOCKED_DOOR_%d", i));
    }
    feat_door[DOOR_DOOR].num_locked = MAX_LJ_DOORS - 1;

    /* Jammed doors */
    for (auto i = 0; i < MAX_LJ_DOORS; i++) {
        feat_door[DOOR_DOOR].jammed[i] = terrains.get_terrain_id_by_tag(format("JAMMED_DOOR_%d", i));
    }
    feat_door[DOOR_DOOR].num_jammed = MAX_LJ_DOORS;

    /* Glass doors */
    feat_door[DOOR_GLASS_DOOR].open = terrains.get_terrain_id_by_tag("OPEN_GLASS_DOOR");
    feat_door[DOOR_GLASS_DOOR].broken = terrains.get_terrain_id_by_tag("BROKEN_GLASS_DOOR");
    feat_door[DOOR_GLASS_DOOR].closed = terrains.get_terrain_id_by_tag("CLOSED_GLASS_DOOR");

    /* Locked glass doors */
    for (auto i = 1; i < MAX_LJ_DOORS; i++) {
        feat_door[DOOR_GLASS_DOOR].locked[i - 1] = terrains.get_terrain_id_by_tag(format("LOCKED_GLASS_DOOR_%d", i));
    }
    feat_door[DOOR_GLASS_DOOR].num_locked = MAX_LJ_DOORS - 1;

    /* Jammed glass doors */
    for (auto i = 0; i < MAX_LJ_DOORS; i++) {
        feat_door[DOOR_GLASS_DOOR].jammed[i] = terrains.get_terrain_id_by_tag(format("JAMMED_GLASS_DOOR_%d", i));
    }
    feat_door[DOOR_GLASS_DOOR].num_jammed = MAX_LJ_DOORS;

    /* Curtains */
    feat_door[DOOR_CURTAIN].open = terrains.get_terrain_id_by_tag("OPEN_CURTAIN");
    feat_door[DOOR_CURTAIN].broken = feat_door[DOOR_CURTAIN].open;
    feat_door[DOOR_CURTAIN].closed = terrains.get_terrain_id_by_tag("CLOSED_CURTAIN");
    feat_door[DOOR_CURTAIN].locked[0] = feat_door[DOOR_CURTAIN].closed;
    feat_door[DOOR_CURTAIN].num_locked = 1;
    feat_door[DOOR_CURTAIN].jammed[0] = feat_door[DOOR_CURTAIN].closed;
    feat_door[DOOR_CURTAIN].num_jammed = 1;

    init_wilderness_terrains();
}
