#include "info-reader/dungeon-reader.h"
#include "grid/feature.h"
#include "info-reader/dungeon-info-tokens-table.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/race-info-tokens-table.h"
#include "io/tokenizer.h"
#include "main/angband-headers.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/monster-race-info.h"
#include "system/terrain-type-definition.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <span>

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(ダンジョン用) /
 * Grab one flag for a dungeon type from a textual string
 * @param d_ptr 保管先のダンジョン構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return 見つけたらtrue
 */
static bool grab_one_dungeon_flag(DungeonDefinition *d_ptr, std::string_view what)
{
    if (EnumClassFlagGroup<DungeonFeatureType>::grab_one_flag(d_ptr->flags, dungeon_flags, what)) {
        return true;
    }

    msg_format(_("未知のダンジョン・フラグ '%s'。", "Unknown dungeon type flag '%s'."), what.data());
    return false;
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(モンスターのダンジョン出現条件用1) /
 * Grab one (basic) flag in a MonraceDefinition from a textual string
 * @param d_ptr 保管先のダンジョン構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return 見つけたらtrue
 */
static bool grab_one_basic_monster_flag(DungeonDefinition *d_ptr, std::string_view what)
{
    if (EnumClassFlagGroup<MonsterFeedType>::grab_one_flag(d_ptr->mon_meat_feed_flags, r_info_meat_feed, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterResistanceType>::grab_one_flag(d_ptr->mon_resistance_flags, r_info_flagsr, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterBehaviorType>::grab_one_flag(d_ptr->mon_behavior_flags, r_info_behavior_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterVisualType>::grab_one_flag(d_ptr->mon_visual_flags, r_info_visual_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterKindType>::grab_one_flag(d_ptr->mon_kind_flags, r_info_kind_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterDropType>::grab_one_flag(d_ptr->mon_drop_flags, r_info_drop_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterWildernessType>::grab_one_flag(d_ptr->mon_wilderness_flags, r_info_wilderness_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterFeatureType>::grab_one_flag(d_ptr->mon_feature_flags, r_info_feature_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterPopulationType>::grab_one_flag(d_ptr->mon_population_flags, r_info_population_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterSpeakType>::grab_one_flag(d_ptr->mon_speak_flags, r_info_speak_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterBrightnessType>::grab_one_flag(d_ptr->mon_brightness_flags, r_info_brightness_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterSpecialType>::grab_one_flag(d_ptr->mon_special_flags, r_info_special_flags, what)) {
        return true;
    }
    if (EnumClassFlagGroup<MonsterMiscType>::grab_one_flag(d_ptr->mon_misc_flags, r_info_misc_flags, what)) {
        return true;
    }

    return false;
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(モンスターのダンジョン出現条件用2) /
 * Grab one (spell) flag in a MonraceDefinition from a textual string
 * @param d_ptr 保管先のダンジョン構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return 見つけたらtrue
 */
static bool grab_one_spell_monster_flag(DungeonDefinition *d_ptr, std::string_view what)
{
    if (EnumClassFlagGroup<MonsterAbilityType>::grab_one_flag(d_ptr->mon_ability_flags, r_info_ability_flags, what)) {
        return true;
    }

    msg_format(_("未知のモンスター・フラグ '%s'。", "Unknown monster flag '%s'."), what.data());
    return false;
}

/*!
 * @brief ダンジョン情報(DungeonsDefinition)のパース関数 /
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_dungeons_info(std::string_view buf, angband_header *)
{
    static DungeonDefinition *d_ptr = nullptr;
    const auto &tokens = str_split(buf, ':', false);
    const auto &terrains = TerrainList::get_instance();

    if (tokens[0] == "N") {
        // N:index:name_ja
        if (tokens.size() < 3 || tokens[1].size() == 0) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        auto i = std::stoi(tokens[1]);
        if (i < error_idx) {
            return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
        }
        if (i >= static_cast<int>(dungeons_info.size())) {
            dungeons_info.resize(i + 1);
        }

        error_idx = i;
        d_ptr = &dungeons_info[i];
        d_ptr->idx = static_cast<DUNGEON_IDX>(i);
#ifdef JP
        d_ptr->name = tokens[2];
#endif
    } else if (!d_ptr) {
        return PARSE_ERROR_MISSING_RECORD_HEADER;
    } else if (tokens[0] == "E") {
        // E:name_en
#ifndef JP
        if (tokens.size() < 2 || tokens[1].size() == 0) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
        d_ptr->name = tokens[1];
#endif
    } else if (tokens[0] == "D") {
        // D:text_ja
        // D:$text_en
        if (tokens.size() < 2 || buf.length() < 3) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
#ifdef JP
        if (buf[2] == '$') {
            return PARSE_ERROR_NONE;
        }
        d_ptr->text.append(buf.substr(2));
#else
        if (buf[2] != '$') {
            return PARSE_ERROR_NONE;
        }
        if (buf.length() == 3) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
        append_english_text(d_ptr->text, buf.substr(3));
#endif
    } else if (tokens[0] == "W") {
        // W:min_level:max_level:(1):mode:(2):(3):(4):(5):prob_pit:prob_nest
        // (1)minimum player level (unused)
        // (2)minimum level of allocating monster
        // (3)maximum probability of level boost of allocation monster
        // (4)maximum probability of dropping good objects
        // (5)maximum probability of dropping great objects
        if (tokens.size() < 11) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        info_set_value(d_ptr->mindepth, tokens[1]);
        info_set_value(d_ptr->maxdepth, tokens[2]);
        info_set_value(d_ptr->min_plev, tokens[3]);
        info_set_value(d_ptr->mode, tokens[4]);
        info_set_value(d_ptr->min_m_alloc_level, tokens[5]);
        info_set_value(d_ptr->max_m_alloc_chance, tokens[6]);
        info_set_value(d_ptr->obj_good, tokens[7]);
        info_set_value(d_ptr->obj_great, tokens[8]);
        info_set_value(d_ptr->pit, tokens[9], 16);
        info_set_value(d_ptr->nest, tokens[10], 16);
    } else if (tokens[0] == "P") {
        // P:wild_y:wild_x
        if (tokens.size() < 3) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        info_set_value(d_ptr->dy, tokens[1]);
        info_set_value(d_ptr->dx, tokens[2]);
    } else if (tokens[0] == "L") {
        // L:floor_1:prob_1:floor_2:prob_2:floor_3:prob_3:tunnel_prob
        if (tokens.size() < DUNGEON_FEAT_PROB_NUM * 2 + 2) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        for (size_t i = 0; i < DUNGEON_FEAT_PROB_NUM; i++) {
            auto feat_idx = i * 2 + 1;
            auto per_idx = feat_idx + 1;
            try {
                d_ptr->floor[i].feat = terrains.get_terrain_id_by_tag(tokens[feat_idx]);
            } catch (const std::exception &) {
                return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
            }

            info_set_value(d_ptr->floor[i].percent, tokens[per_idx]);
        }

        auto tunnel_idx = DUNGEON_FEAT_PROB_NUM * 2 + 1;
        info_set_value(d_ptr->tunnel_percent, tokens[tunnel_idx]);
    } else if (tokens[0] == "A") {
        // A:wall_1:prob_1:wall_2:prob_2:wall_3:prob_3:outer_wall:inner_wall:stream_1:stream_2
        if (tokens.size() < DUNGEON_FEAT_PROB_NUM * 2 + 5) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        for (int i = 0; i < DUNGEON_FEAT_PROB_NUM; i++) {
            auto feat_idx = i * 2 + 1;
            auto prob_idx = feat_idx + 1;
            try {
                d_ptr->fill[i].feat = terrains.get_terrain_id_by_tag(tokens[feat_idx]);
            } catch (const std::exception &) {
                return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
            }

            info_set_value(d_ptr->fill[i].percent, tokens[prob_idx]);
        }

        try {
            const std::span tags(tokens.begin() + DUNGEON_FEAT_PROB_NUM * 2 + 1, 4);
            d_ptr->outer_wall = terrains.get_terrain_id_by_tag(tags[0]);
            d_ptr->inner_wall = terrains.get_terrain_id_by_tag(tags[1]);
            d_ptr->stream1 = terrains.get_terrain_id_by_tag(tags[2]);
            d_ptr->stream2 = terrains.get_terrain_id_by_tag(tags[3]);
        } catch (const std::exception &) {
            return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
        }
    } else if (tokens[0] == "F") {
        // F:flags
        if (tokens.size() < 2) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto &flags = str_split(tokens[1], '|', true);
        for (const auto &f : flags) {
            if (f.size() == 0) {
                continue;
            }

            const auto &f_tokens = str_split(f, '_');
            if (f_tokens.size() == 3) {
                if (f_tokens[0] == "FINAL" && f_tokens[1] == "ARTIFACT") {
                    info_set_value(d_ptr->final_artifact, f_tokens[2]);
                    continue;
                }
                if (f_tokens[0] == "FINAL" && f_tokens[1] == "OBJECT") {
                    info_set_value(d_ptr->final_object, f_tokens[2]);
                    continue;
                }
                if (f_tokens[0] == "FINAL" && f_tokens[1] == "GUARDIAN") {
                    info_set_value(d_ptr->final_guardian, f_tokens[2]);
                    continue;
                }
                if (f_tokens[0] == "MONSTER" && f_tokens[1] == "DIV") {
                    info_set_value(d_ptr->special_div, f_tokens[2]);
                    continue;
                }
                if (f_tokens[0] == "MONSTER" && f_tokens[1] == "RATE") {
                    info_set_value(d_ptr->monster_rate, f_tokens[2]);
                    continue;
                }
                if (f_tokens[0] == "TRAP" && f_tokens[1] == "RATE") {
                    info_set_value(d_ptr->trap_rate, f_tokens[2]);
                    continue;
                }
            }

            if (f_tokens.size() == 5 && f_tokens[0] == "FIXED" && f_tokens[1] == "ROOM") {
                int depth, id, percentage;
                info_set_value(depth, f_tokens[2]);
                info_set_value(id, f_tokens[3]);
                info_set_value(percentage, f_tokens[4]);
                d_ptr->fixed_room_list.push_back(std::make_tuple(depth, id, percentage));
                continue;
            }

            if (f_tokens.size() == 2 && f_tokens[0] == "ALLIANCE") {
                for (auto a : alliance_list) {
                    if (a.second->tag == f_tokens[1]) {
                        d_ptr->alliance_idx = static_cast<AllianceType>(a.second->id);
                    }
                }
                continue;
            }

            if (!grab_one_dungeon_flag(d_ptr, f)) {
                return PARSE_ERROR_INVALID_FLAG;
            }
        }
    } else if (tokens[0] == "M") {
        // M:monsterflags
        if (tokens[1] == "X") {
            if (tokens.size() < 3) {
                return PARSE_ERROR_TOO_FEW_ARGUMENTS;
            }
            uint32_t sex;
            if (!info_grab_one_const(sex, r_info_sex, tokens[2])) {
                return PARSE_ERROR_INVALID_FLAG;
            }
            d_ptr->mon_sex = static_cast<MonsterSex>(sex);
            return 0;
        }
        if (tokens.size() < 2) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto &flags = str_split(tokens[1], '|', true);
        for (const auto &f : flags) {
            if (f.size() == 0) {
                continue;
            }

            const auto &m_tokens = str_split(f, '_');
            if (m_tokens[0] == "R" && m_tokens[1] == "CHAR") {
                d_ptr->r_chars.insert(d_ptr->r_chars.end(), m_tokens[2].begin(), m_tokens[2].end());
                continue;
            }

            if (grab_one_basic_monster_flag(d_ptr, f)) {
                continue;
            }

            uint32_t sex;
            if (!info_grab_one_const(sex, r_info_sex, f)) {
                return PARSE_ERROR_INVALID_FLAG;
            }
        }
    } else if (tokens[0] == "S") {
        // S: flags
        if (tokens.size() < 2) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto &flags = str_split(tokens[1], '|', true);
        for (const auto &f : flags) {
            if (f.size() == 0) {
                continue;
            }

            const auto &s_tokens = str_split(f, '_');
            if (s_tokens.size() == 3 && s_tokens[1] == "IN") {
                if (s_tokens[0] != "1") {
                    return PARSE_ERROR_GENERIC;
                }
                continue; //!< MonsterRaceDefinitions.txtからのコピペ対策
            }

            if (!grab_one_spell_monster_flag(d_ptr, f)) {
                return PARSE_ERROR_INVALID_FLAG;
            }
        }
    } else if (tokens[0] == "R") {
        int r_type, r_rate;
        if (tokens.size() < 2) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
        info_set_value(r_type, tokens[1]);
        info_set_value(r_rate, tokens[2]);
        d_ptr->room_rate[i2enum<RoomType>(r_type)] = r_rate;
    } else {
        return PARSE_ERROR_UNDEFINED_DIRECTIVE;
    }

    return 0;
}

/*!
 * @brief ダンジョン情報の読み込みが終わった後の設定を行う
 */
void retouch_dungeons_info()
{
    for (auto &dungeon : dungeons_info) {
        if (dungeon.is_dungeon() && dungeon.has_guardian()) {
            auto &monrace = dungeon.get_guardian();
            monrace.misc_flags.set(MonsterMiscType::GUARDIAN);
        }
    }
}
