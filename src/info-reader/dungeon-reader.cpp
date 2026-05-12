#include "info-reader/dungeon-reader.h"
#include "info-reader/dungeon-info-tokens-table.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/race-info-tokens-table.h"
#include "io/tokenizer.h"
#include "main/angband-headers.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "util/dice.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <span>

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(ダンジョン用)
 * @param dungeon ダンジョンへの参照
 * @param what 参照元の文字列
 * @return 見つけたらtrue
 */
static bool grab_one_dungeon_flag(DungeonDefinition &dungeon, std::string_view what)
{
    if (EnumClassFlagGroup<DungeonFeatureType>::grab_one_flag(dungeon.flags, dungeon_flags, what)) {
        return true;
    }

    msg_format(_("未知のダンジョン・フラグ '%s'。", "Unknown dungeon type flag '%s'."), what.data());
    return false;
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(モンスターのダンジョン出現条件用1)
 * @param dungeon ダンジョンへの参照
 * @param what 参照元の文字列
 * @return 見つけたらtrue
 */
static bool grab_one_basic_monster_flag(DungeonDefinition &dungeon, std::string_view what)
{
    if (EnumClassFlagGroup<MonsterFeedType>::grab_one_flag(dungeon.mon_meat_feed_flags, r_info_meat_feed, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterResistanceType>::grab_one_flag(dungeon.mon_resistance_flags, r_info_flagsr, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterBehaviorType>::grab_one_flag(dungeon.mon_behavior_flags, r_info_behavior_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterVisualType>::grab_one_flag(dungeon.mon_visual_flags, r_info_visual_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterKindType>::grab_one_flag(dungeon.mon_kind_flags, r_info_kind_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterDropType>::grab_one_flag(dungeon.mon_drop_flags, r_info_drop_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterWildernessType>::grab_one_flag(dungeon.mon_wilderness_flags, r_info_wilderness_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterFeatureType>::grab_one_flag(dungeon.mon_feature_flags, r_info_feature_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterPopulationType>::grab_one_flag(dungeon.mon_population_flags, r_info_population_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterSpeakType>::grab_one_flag(dungeon.mon_speak_flags, r_info_speak_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterBrightnessType>::grab_one_flag(dungeon.mon_brightness_flags, r_info_brightness_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterSpecialType>::grab_one_flag(dungeon.mon_special_flags, r_info_special_flags, what)) {
        return true;
    }
    if (EnumClassFlagGroup<MonsterMiscType>::grab_one_flag(dungeon.mon_misc_flags, r_info_misc_flags, what)) {
        return true;
    }

    return false;
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(モンスターのダンジョン出現条件用2)
 * @param dungeon ダンジョンへの参照
 * @param what 参照元の文字列
 * @return 見つけたらtrue
 */
static bool grab_one_spell_monster_flag(DungeonDefinition &dungeon, std::string_view what)
{
    if (EnumClassFlagGroup<MonsterAbilityType>::grab_one_flag(dungeon.mon_ability_flags, r_info_ability_flags, what)) {
        return true;
    }

    msg_format(_("未知のモンスター・フラグ '%s'。", "Unknown monster flag '%s'."), what.data());
    return false;
}

static tl::optional<ProbabilityTable<short>> parse_terrain_probability(std::span<const std::string> tokens)
{
    const auto &terrains = TerrainList::get_instance();
    ProbabilityTable<short> prob_table;

    for (auto i = 0; std::cmp_less(i + 1, tokens.size()); i += 2) {
        try {
            const auto terrain_id = terrains.get_terrain_id(tokens[i]);
            const auto prob = static_cast<short>(std::stoi(tokens[i + 1]));
            prob_table.entry_item(terrain_id, prob);
        } catch (const std::exception &) {
            return tl::nullopt;
        }
    }

    return prob_table;
}

/*!
 * @brief ダンジョン情報(DungeonsDefinition)のパース関数 /
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_dungeons_info(std::string_view buf, angband_header *)
{
    const auto &tokens = str_split(buf, ':', false);
    const auto &terrains = TerrainList::get_instance();

    // N:index:name_ja
    auto &dungeons = DungeonList::get_instance();
    if (tokens[0] == "N") {
        if (tokens.size() < 3 || tokens[1].size() == 0) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto i = std::stoi(tokens[1]);
        if (i < error_idx) {
            return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
        }

        error_idx = i;
        DungeonDefinition dungeon;
#ifdef JP
        dungeon.name = tokens[2];
#endif
        dungeons.emplace(i2enum<DungeonId>(i), std::move(dungeon));
        return PARSE_ERROR_NONE;
    }

    if (dungeons.empty()) {
        return PARSE_ERROR_MISSING_RECORD_HEADER;
    }

    // E:name_en
    auto &[dungeon_id, dungeon] = *dungeons.rbegin();
    if (tokens[0] == "E") {
#ifndef JP
        if (tokens.size() < 2 || tokens[1].size() == 0) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        dungeon->name = tokens[1];
#endif
        return PARSE_ERROR_NONE;
    }

    // T:tag
    if (tokens[0] == "T") {
        if (tokens.size() < 2 || tokens[1].size() == 0) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        dungeon->tag = tokens[1];
        return PARSE_ERROR_NONE;
    }

    // D:text_ja
    // D:$text_en
    if (tokens[0] == "D") {
        if (tokens.size() < 2 || buf.length() < 3) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
#ifdef JP
        if (buf[2] == '$') {
            return PARSE_ERROR_NONE;
        }

        dungeon->text.append(buf.substr(2));
#else
        if (buf[2] != '$') {
            return PARSE_ERROR_NONE;
        }

        if (buf.length() == 3) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
        append_english_text(dungeon->text, buf.substr(3));
#endif
        return PARSE_ERROR_NONE;
    }

    // W:min_level:max_level:(1):mode:(2):(3):(4):(5):prob_pit:prob_nest
    // (1)minimum player level (unused)
    // (2)minimum level of allocating monster
    // (3)maximum probability of level boost of allocation monster
    // (4)maximum probability of dropping good objects
    // (5)maximum probability of dropping great objects
    if (tokens[0] == "W") {
        if (tokens.size() < 11) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        info_set_value(dungeon->mindepth, tokens[1]);
        info_set_value(dungeon->maxdepth, tokens[2]);
        info_set_value(dungeon->min_plev, tokens[3]);
        info_set_value(dungeon->mode, tokens[4]);
        info_set_value(dungeon->min_m_alloc_level, tokens[5]);
        info_set_value(dungeon->max_m_alloc_chance, tokens[6]);
        info_set_value(dungeon->obj_good, tokens[7]);
        info_set_value(dungeon->obj_great, tokens[8]);
        info_set_value(dungeon->pit, tokens[9], 16);
        info_set_value(dungeon->nest, tokens[10], 16);
        return PARSE_ERROR_NONE;
    }

    // P:wild_y:wild_x
    if (tokens[0] == "P") {
        if (tokens.size() < 3) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto wild_y = std::stoi(tokens[1]);
        const auto wild_x = std::stoi(tokens[2]);
        dungeon->initialize_position({ wild_y, wild_x });
        return PARSE_ERROR_NONE;
    }

    // L:floor_1:prob_1:floor_2:prob_2:floor_3:prob_3:tunnel_prob
    constexpr auto terrain_probability_num = 3;
    if (tokens[0] == "L") {
        if (tokens.size() < terrain_probability_num * 2 + 2) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        auto prob_table = parse_terrain_probability(std::span(tokens).subspan(1, terrain_probability_num * 2));
        if (!prob_table) {
            return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
        }
        dungeon->prob_table_floor = std::move(*prob_table);

        auto tunnel_idx = terrain_probability_num * 2 + 1;
        info_set_value(dungeon->tunnel_percent, tokens[tunnel_idx]);
        return PARSE_ERROR_NONE;
    }

    // A:wall_1:prob_1:wall_2:prob_2:wall_3:prob_3:outer_wall:inner_wall:stream_1:stream_2
    if (tokens[0] == "A") {
        if (tokens.size() < terrain_probability_num * 2 + 5) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        auto prob_table = parse_terrain_probability(std::span(tokens).subspan(1, terrain_probability_num * 2));
        if (!prob_table) {
            return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
        }
        dungeon->prob_table_wall = std::move(*prob_table);

        try {
            const auto tags = std::span(tokens).subspan(terrain_probability_num * 2 + 1, 4);
            dungeon->outer_wall = terrains.get_terrain_id(tags[0]);
            dungeon->inner_wall = terrains.get_terrain_id(tags[1]);
            dungeon->stream1 = terrains.get_terrain_id(tags[2]);
            dungeon->stream2 = terrains.get_terrain_id(tags[3]);
            return PARSE_ERROR_NONE;
        } catch (const std::exception &) {
            return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
        }
    }

    // F:flags
    if (tokens[0] == "F") {
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
                    info_set_value(dungeon->final_artifact, f_tokens[2]);
                    continue;
                }
                if (f_tokens[0] == "FINAL" && f_tokens[1] == "OBJECT") {
                    info_set_value(dungeon->final_object, f_tokens[2]);
                    continue;
                }
                if (f_tokens[0] == "FINAL" && f_tokens[1] == "GUARDIAN") {
                    info_set_value(dungeon->final_guardian, f_tokens[2]);
                    continue;
                }
                if (f_tokens[0] == "MONSTER" && f_tokens[1] == "DIV") {
                    info_set_value(dungeon->special_div, f_tokens[2]);
                    continue;
                }
                if (f_tokens[0] == "MONSTER" && f_tokens[1] == "RATE") {
                    info_set_value(dungeon->monster_rate, f_tokens[2]);
                    continue;
                }
                if (f_tokens[0] == "TRAP" && f_tokens[1] == "RATE") {
                    info_set_value(dungeon->trap_rate, f_tokens[2]);
                    continue;
                }
            }

            if (f_tokens.size() == 5 && f_tokens[0] == "FIXED" && f_tokens[1] == "ROOM") {
                int depth, id, percentage;
                info_set_value(depth, f_tokens[2]);
                info_set_value(id, f_tokens[3]);
                info_set_value(percentage, f_tokens[4]);
                dungeon->fixed_room_list.push_back(std::make_tuple(depth, id, percentage));
                continue;
            }

            if (f_tokens.size() == 2 && f_tokens[0] == "ALLIANCE") {
                for (auto a : alliance_list) {
                    if (a.second->tag == f_tokens[1]) {
                        dungeon->alliance_idx = static_cast<AllianceType>(a.second->id);
                    }
                }
                continue;
            }

            if (!grab_one_dungeon_flag(*dungeon, f)) {
                return PARSE_ERROR_INVALID_FLAG;
            }
        }

        return PARSE_ERROR_NONE;
    }

    // M:Monster flags
    if (tokens[0] == "M") {
        if (tokens[1] == "X") {
            if (tokens.size() < 3) {
                return PARSE_ERROR_TOO_FEW_ARGUMENTS;
            }

            uint32_t sex;
            if (!info_grab_one_const(sex, r_info_sex, tokens[2])) {
                return PARSE_ERROR_INVALID_FLAG;
            }

            dungeon->mon_sex = static_cast<MonsterSex>(sex);
            return 0;
        }

        if (tokens.size() < 2) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto &flags = str_split(tokens[1], '|', true);
        for (const auto &f : flags) {
            if (f.empty()) {
                continue;
            }

            const auto &m_tokens = str_split(f, '_');
            if (m_tokens[0] == "R" && m_tokens[1] == "CHAR") {
                dungeon->r_chars.insert(dungeon->r_chars.end(), m_tokens[2].begin(), m_tokens[2].end());
                continue;
            }

            uint32_t sex;
            if (info_grab_one_const(sex, r_info_sex, f)) {
                continue;
            }

            if (!grab_one_basic_monster_flag(*dungeon, f)) {
                return PARSE_ERROR_INVALID_FLAG;
            }
        }

        return PARSE_ERROR_NONE;
    }

    // S: flags
    if (tokens[0] == "S") {
        if (tokens.size() < 2) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto &flags = str_split(tokens[1], '|', true);
        for (const auto &f : flags) {
            if (f.empty()) {
                continue;
            }

            const auto &s_tokens = str_split(f, '_');
            if (s_tokens.size() == 3 && s_tokens[1] == "IN") {
                if (s_tokens[0] != "1") {
                    return PARSE_ERROR_GENERIC;
                }

                continue; //!< @details MonsterRaceDefinitions.jsonc からのコピペ対策
            }

            if (!grab_one_spell_monster_flag(*dungeon, f)) {
                return PARSE_ERROR_INVALID_FLAG;
            }
        }
        return PARSE_ERROR_NONE;
    }

    if (tokens[0] == "R") {
        int r_type, r_rate;
        if (tokens.size() < 2) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
        info_set_value(r_type, tokens[1]);
        info_set_value(r_rate, tokens[2]);
        dungeon->room_rate[i2enum<RoomType>(r_type)] = r_rate;
        return PARSE_ERROR_NONE;
    }

    // K:floor:probability:dice_num:dice_sides:item_id - 特定階層でのダイスベースアイテム生成指定
    if (tokens[0] == "K") {
        if (tokens.size() < 6) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
        int floor_level, probability, dice_num, dice_sides, item_id;
        info_set_value(floor_level, tokens[1]);
        info_set_value(probability, tokens[2]);
        info_set_value(dice_num, tokens[3]);
        info_set_value(dice_sides, tokens[4]);
        info_set_value(item_id, tokens[5]);

        FloorItemGenerationRule rule;
        rule.probability = probability;
        rule.dice = Dice(dice_num, dice_sides);
        rule.item_id = item_id;

        dungeon->specific_item_generation_map[floor_level] = rule;
        return PARSE_ERROR_NONE;
    }

    // Z:floor:vault_id - 特定階層で生成するVault指定
    if (tokens[0] == "Z") {
        if (tokens.size() < 3) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
        int floor_level, vault_id;
        info_set_value(floor_level, tokens[1]);
        info_set_value(vault_id, tokens[2]);
        dungeon->specific_vault_map[floor_level] = i2enum<VaultTypeId>(vault_id);
        return PARSE_ERROR_NONE;
    }

    return PARSE_ERROR_UNDEFINED_DIRECTIVE;
}

/*!
 * @brief ダンジョン情報(JSON Object)のパース関数 (フェーズ A-2 Dungeon JSON 化)
 * @param dungeon_data JSON ダンジョンエントリ
 * @param head ヘッダ構造体
 * @return エラーコード
 * @details
 * 構造化 JSON (version 2) を読み、各フィールドから .txt 形式のトークン文字列を
 * 合成して既存の token-based parse_dungeons_info() に流す。これにより:
 *  - JSON は人間可読・schema 検証可能な構造化データとして保持
 *  - パーサ本体は token-based の既存実装をそのまま再利用
 *
 * 後方互換: version 1 の `lines` 配列形式 (旧 wrapped-line) も受け付ける。
 */
static errr emit_token(const std::string &token, angband_header *head)
{
    if (token.empty()) {
        return PARSE_ERROR_NONE;
    }
    return parse_dungeons_info(token, head);
}

errr parse_dungeons_info_json(nlohmann::json &dungeon_data, angband_header *head)
{
    // version 1 (wrapped-line) 互換パス
    if (dungeon_data.contains("lines") && dungeon_data["lines"].is_array()) {
        for (const auto &line : dungeon_data["lines"]) {
            if (!line.is_string()) {
                continue;
            }
            const auto buf = line.get<std::string>();
            if (buf.empty()) {
                continue;
            }
            const auto err = parse_dungeons_info(buf, head);
            if (err != PARSE_ERROR_NONE) {
                return err;
            }
        }
        return PARSE_ERROR_NONE;
    }

    // version 2 (structured) パス
    if (!dungeon_data.contains("id")) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    const auto id = dungeon_data["id"].get<int>();

    // N: id:name_ja (必須)
    std::string name_ja;
    if (dungeon_data.contains("name") && dungeon_data["name"].contains("ja")) {
        name_ja = dungeon_data["name"]["ja"].get<std::string>();
    }
    if (auto err = emit_token("N:" + std::to_string(id) + ":" + name_ja, head); err != PARSE_ERROR_NONE) {
        return err;
    }

    // E: name_en
    if (dungeon_data.contains("name") && dungeon_data["name"].contains("en")) {
        const auto en = dungeon_data["name"]["en"].get<std::string>();
        if (!en.empty()) {
            if (auto err = emit_token("E:" + en, head); err != PARSE_ERROR_NONE) {
                return err;
            }
        }
    }

    // T: tag
    if (dungeon_data.contains("tag")) {
        const auto tag = dungeon_data["tag"].get<std::string>();
        if (!tag.empty()) {
            if (auto err = emit_token("T:" + tag, head); err != PARSE_ERROR_NONE) {
                return err;
            }
        }
    }

    // D: text (ja and/or en)
    if (dungeon_data.contains("description")) {
        const auto &desc = dungeon_data["description"];
        if (desc.contains("ja")) {
            const auto ja = desc["ja"].get<std::string>();
            if (!ja.empty()) {
                if (auto err = emit_token("D:" + ja, head); err != PARSE_ERROR_NONE) {
                    return err;
                }
            }
        }
        if (desc.contains("en")) {
            const auto en = desc["en"].get<std::string>();
            if (!en.empty()) {
                if (auto err = emit_token("D:$" + en, head); err != PARSE_ERROR_NONE) {
                    return err;
                }
            }
        }
    }

    // P: y:x
    if (dungeon_data.contains("position")) {
        const auto &pos = dungeon_data["position"];
        const auto y = pos.value("y", 0);
        const auto x = pos.value("x", 0);
        if (auto err = emit_token("P:" + std::to_string(y) + ":" + std::to_string(x), head); err != PARSE_ERROR_NONE) {
            return err;
        }
    }

    // W: min_depth:max_depth:min_player_level:flags_mode:min_alloc:max_alloc_chance:obj_good:obj_great:pit:nest
    if (dungeon_data.contains("generation")) {
        const auto &g = dungeon_data["generation"];
        std::string buf = "W:";
        buf += std::to_string(g.value("min_depth", 0)) + ":";
        buf += std::to_string(g.value("max_depth", 0)) + ":";
        buf += std::to_string(g.value("min_player_level", 0)) + ":";
        buf += std::to_string(g.value("flags_mode", 0)) + ":";
        buf += std::to_string(g.value("min_alloc", 0)) + ":";
        buf += std::to_string(g.value("max_alloc_chance", 0)) + ":";
        buf += std::to_string(g.value("obj_good", 0)) + ":";
        buf += std::to_string(g.value("obj_great", 0)) + ":";
        buf += g.value("pit", std::string("0x0000")) + ":";
        buf += g.value("nest", std::string("0x0000"));
        if (auto err = emit_token(buf, head); err != PARSE_ERROR_NONE) {
            return err;
        }
    }

    // L: floor.tiles[3] + tunnel_rate
    if (dungeon_data.contains("floor")) {
        const auto &f = dungeon_data["floor"];
        std::string buf = "L:";
        const auto &tiles = f["tiles"];
        for (size_t i = 0; i < 3; i++) {
            buf += tiles[i].value("terrain", std::string("FLOOR")) + ":";
            buf += std::to_string(tiles[i].value("rate", 0)) + ":";
        }
        buf += std::to_string(f.value("tunnel_rate", 0));
        if (auto err = emit_token(buf, head); err != PARSE_ERROR_NONE) {
            return err;
        }
    }

    // A: wall.tiles[3] + outer + inner + stream1 + stream2
    if (dungeon_data.contains("wall")) {
        const auto &w = dungeon_data["wall"];
        std::string buf = "A:";
        const auto &tiles = w["tiles"];
        for (size_t i = 0; i < 3; i++) {
            buf += tiles[i].value("terrain", std::string("GRANITE")) + ":";
            buf += std::to_string(tiles[i].value("rate", 0)) + ":";
        }
        buf += w.value("outer", std::string("GRANITE")) + ":";
        buf += w.value("inner", std::string("GRANITE")) + ":";
        buf += w.value("stream1", std::string("MAGMA_VEIN")) + ":";
        buf += w.value("stream2", std::string("QUARTZ_VEIN"));
        if (auto err = emit_token(buf, head); err != PARSE_ERROR_NONE) {
            return err;
        }
    }

    // F: 各フラグ + 専用変換 (MONSTER_RATE/TRAP_RATE/MONSTER_DIV/FINAL_*/ALLIANCE_*/FIXED_ROOM)
    {
        std::vector<std::string> f_tokens;
        if (dungeon_data.contains("flags")) {
            for (const auto &flag : dungeon_data["flags"]) {
                f_tokens.push_back(flag.get<std::string>());
            }
        }
        if (dungeon_data.contains("monster_rate")) {
            f_tokens.push_back("MONSTER_RATE_" + std::to_string(dungeon_data["monster_rate"].get<int>()));
        }
        if (dungeon_data.contains("trap_rate")) {
            f_tokens.push_back("TRAP_RATE_" + std::to_string(dungeon_data["trap_rate"].get<int>()));
        }
        if (dungeon_data.contains("monster_div")) {
            f_tokens.push_back("MONSTER_DIV_" + std::to_string(dungeon_data["monster_div"].get<int>()));
        }
        if (dungeon_data.contains("final_guardian")) {
            f_tokens.push_back("FINAL_GUARDIAN_" + std::to_string(dungeon_data["final_guardian"].get<int>()));
        }
        if (dungeon_data.contains("final_object")) {
            f_tokens.push_back("FINAL_OBJECT_" + std::to_string(dungeon_data["final_object"].get<int>()));
        }
        if (dungeon_data.contains("final_artifact")) {
            f_tokens.push_back("FINAL_ARTIFACT_" + std::to_string(dungeon_data["final_artifact"].get<int>()));
        }
        if (dungeon_data.contains("alliance")) {
            f_tokens.push_back("ALLIANCE_" + dungeon_data["alliance"].get<std::string>());
        }
        if (dungeon_data.contains("fixed_rooms")) {
            for (const auto &fr : dungeon_data["fixed_rooms"]) {
                std::string flag = "FIXED_ROOM_" + std::to_string(fr.value("depth", 0)) + "_";
                flag += std::to_string(fr.value("id", 0)) + "_";
                flag += std::to_string(fr.value("percentage", 0));
                f_tokens.push_back(flag);
            }
        }
        // emit "F:" 1 行ごとに 1 フラグ (既存パーサが " | " 区切りも単独も両方扱える)
        for (const auto &flag : f_tokens) {
            if (auto err = emit_token("F:" + flag, head); err != PARSE_ERROR_NONE) {
                return err;
            }
        }
    }

    // M: monster_flags
    if (dungeon_data.contains("monster_flags")) {
        for (const auto &flag : dungeon_data["monster_flags"]) {
            if (auto err = emit_token("M:" + flag.get<std::string>(), head); err != PARSE_ERROR_NONE) {
                return err;
            }
        }
    }

    // S: monster_spells
    if (dungeon_data.contains("monster_spells")) {
        for (const auto &flag : dungeon_data["monster_spells"]) {
            if (auto err = emit_token("S:" + flag.get<std::string>(), head); err != PARSE_ERROR_NONE) {
                return err;
            }
        }
    }

    // K: specific_items
    if (dungeon_data.contains("specific_items")) {
        for (const auto &item : dungeon_data["specific_items"]) {
            std::string buf = "K:";
            buf += std::to_string(item.value("floor", 0)) + ":";
            buf += std::to_string(item.value("probability", 0)) + ":";
            buf += std::to_string(item.value("dice_num", 0)) + ":";
            buf += std::to_string(item.value("dice_sides", 0)) + ":";
            buf += std::to_string(item.value("item_id", 0));
            if (auto err = emit_token(buf, head); err != PARSE_ERROR_NONE) {
                return err;
            }
        }
    }

    // Z: specific_vaults
    if (dungeon_data.contains("specific_vaults")) {
        for (const auto &v : dungeon_data["specific_vaults"]) {
            std::string buf = "Z:";
            buf += std::to_string(v.value("floor", 0)) + ":";
            buf += std::to_string(v.value("vault_id", 0));
            if (auto err = emit_token(buf, head); err != PARSE_ERROR_NONE) {
                return err;
            }
        }
    }

    // R: room_rates
    if (dungeon_data.contains("room_rates")) {
        for (const auto &r : dungeon_data["room_rates"]) {
            std::string buf = "R:";
            buf += std::to_string(r.value("type", 0)) + ":";
            buf += std::to_string(r.value("rate", 0));
            if (auto err = emit_token(buf, head); err != PARSE_ERROR_NONE) {
                return err;
            }
        }
    }

    return PARSE_ERROR_NONE;
}
