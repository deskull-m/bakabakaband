#include "info-reader/dungeon-reader.h"
#include "artifact/fixed-art-types.h"
#include "info-reader/dungeon-info-tokens-table.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/race-info-tokens-table.h"
#include "io/tokenizer.h"
#include "locale/japanese.h"
#include "main/angband-headers.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "util/dice.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <exception>
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
        info_set_value(dungeon->min_monster_count_on_floor, tokens[5]);
        info_set_value(dungeon->additional_monster_spawn_chance, tokens[6]);
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
 * @brief ダンジョン情報 JSON v2 直接フィル用ヘルパ群
 *
 * 旧 v2 パスはトークン文字列を合成して legacy `parse_dungeons_info()` に流す
 * 二段階方式だったが、JSON フィールドを直接 DungeonDefinition に書き込む形に
 * リファクタした (提案 38)。これにより:
 *  - 上流 (hengband) の direct-fill 方式と構造が一致し、ダンジョン定義系
 *    マージ (#8381 / #8382 / #8385 / #8387 / #8390 等) の差分吸収が容易に
 *  - トークン文字列合成コスト (string concat / split / re-parse) を削減
 *  - JSON エラーの位置特定が直接的になる
 *
 * legacy token parser `parse_dungeons_info()` は v1 (lines 配列) 互換用に
 * 引き続き残置。
 */

/*!
 * @brief JSON name オブジェクトから言語別文字列を取得
 * @param name_obj name オブジェクト (ja / en キーを含む)
 * @return JP ビルドでは ja を SJIS 変換した結果、それ以外は en (なければ空文字列)。
 *         変換失敗時は tl::nullopt。
 */
static tl::optional<std::string> read_localized_name(const nlohmann::json &name_obj)
{
#ifdef JP
    if (!name_obj.contains("ja")) {
        return std::string{};
    }
    const auto ja = name_obj["ja"].get<std::string>();
    if (ja.empty()) {
        return std::string{};
    }
    auto sys = utf8_to_sys(ja);
    if (!sys) {
        return tl::nullopt;
    }
    return std::move(*sys);
#else
    if (!name_obj.contains("en")) {
        return std::string{};
    }
    return name_obj["en"].get<std::string>();
#endif
}

/*!
 * @brief JSON description フィールドから言語別 description テキストを連結追加
 * @param dungeon 対象 DungeonDefinition
 * @param desc_obj description オブジェクト (ja / en キーを含む)
 * @return エラーコード
 */
static errr set_dungeon_description(DungeonDefinition &dungeon, const nlohmann::json &desc_obj)
{
#ifdef JP
    if (!desc_obj.contains("ja")) {
        return PARSE_ERROR_NONE;
    }
    const auto ja = desc_obj["ja"].get<std::string>();
    if (ja.empty()) {
        return PARSE_ERROR_NONE;
    }
    auto sys = utf8_to_sys(ja);
    if (!sys) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    dungeon.text.append(*sys);
#else
    if (!desc_obj.contains("en")) {
        return PARSE_ERROR_NONE;
    }
    const auto en = desc_obj["en"].get<std::string>();
    if (en.empty()) {
        return PARSE_ERROR_NONE;
    }
    append_english_text(dungeon.text, en);
#endif
    return PARSE_ERROR_NONE;
}

/*!
 * @brief generation オブジェクトを DungeonDefinition の生成パラメータに直接フィル
 */
static errr set_dungeon_generation(DungeonDefinition &dungeon, const nlohmann::json &gen_obj)
{
    if (!gen_obj.is_object()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    try {
        dungeon.mindepth = static_cast<DEPTH>(gen_obj.value("min_depth", 0));
        dungeon.maxdepth = static_cast<DEPTH>(gen_obj.value("max_depth", 0));
        dungeon.min_plev = static_cast<PLAYER_LEVEL>(gen_obj.value("min_player_level", 0));

        // flags_mode: 文字列 ("NONE" / "AND" / "NAND" / "OR" / "NOR")。
        // 後方互換: 整数 0-4 を受けたら従来通り cast (旧定義データのフォールバック)
        if (gen_obj.contains("flags_mode")) {
            const auto &mode_node = gen_obj["flags_mode"];
            if (mode_node.is_string()) {
                const auto mode_str = mode_node.get<std::string>();
                const auto it = dungeon_modes.find(mode_str);
                if (it == dungeon_modes.end()) {
                    return PARSE_ERROR_INVALID_FLAG;
                }
                dungeon.mode = it->second;
            } else {
                dungeon.mode = static_cast<DungeonMode>(mode_node.get<int>());
            }
        }

        dungeon.min_monster_count_on_floor = gen_obj.value("min_count", gen_obj.value("min_alloc", 0));

        // extra_spawn_probability: 大きいほど追加生成されやすい (1 〜 1,000,000)
        // 内部値 additional_monster_spawn_chance は 1,000,000 / extra_spawn_probability で
        // 従来の「1/X 形式」と等価に保つ。
        if (gen_obj.contains("extra_spawn_probability")) {
            constexpr auto conversion_rate = 1000000;
            const auto extra_spawn_probability = std::max(1, gen_obj["extra_spawn_probability"].get<int>());
            dungeon.additional_monster_spawn_chance = conversion_rate / extra_spawn_probability;
        } else if (gen_obj.contains("max_alloc_chance")) {
            // 後方互換: 旧定義の値をそのまま使う
            dungeon.additional_monster_spawn_chance = gen_obj.value("max_alloc_chance", 0);
        }

        dungeon.obj_good = gen_obj.value("obj_good", 0);
        dungeon.obj_great = gen_obj.value("obj_great", 0);
        const auto pit = gen_obj.value("pit", std::string("0x0000"));
        const auto nest = gen_obj.value("nest", std::string("0x0000"));
        dungeon.pit = static_cast<BIT_FLAGS16>(std::stoul(pit, nullptr, 16));
        dungeon.nest = static_cast<BIT_FLAGS16>(std::stoul(nest, nullptr, 16));
    } catch (const std::exception &) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    return PARSE_ERROR_NONE;
}

/*!
 * @brief tiles 配列 (terrain + rate のペア) を ProbabilityTable に変換
 */
static tl::optional<ProbabilityTable<short>> build_probability_table_from_tiles(const nlohmann::json &tiles, std::string_view default_terrain)
{
    const auto &terrains = TerrainList::get_instance();
    ProbabilityTable<short> prob_table;
    for (size_t i = 0; i < TERRAIN_PROBABILITY_NUM; ++i) {
        const auto terrain_tag = (i < tiles.size()) ? tiles[i].value("terrain", std::string(default_terrain)) : std::string(default_terrain);
        const auto rate = (i < tiles.size()) ? static_cast<short>(tiles[i].value("rate", 0)) : static_cast<short>(0);
        try {
            const auto terrain_id = terrains.get_terrain_id(terrain_tag);
            prob_table.entry_item(terrain_id, rate);
        } catch (const std::exception &) {
            return tl::nullopt;
        }
    }
    return prob_table;
}

/*!
 * @brief floor オブジェクトを DungeonDefinition の床関連フィールドに直接フィル
 */
static errr set_dungeon_floor(DungeonDefinition &dungeon, const nlohmann::json &floor_obj)
{
    if (!floor_obj.is_object()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    if (!floor_obj.contains("tiles")) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    auto prob_table = build_probability_table_from_tiles(floor_obj["tiles"], "FLOOR");
    if (!prob_table) {
        return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
    }
    dungeon.prob_table_floor = std::move(*prob_table);
    dungeon.tunnel_percent = floor_obj.value("tunnel_rate", 0);
    return PARSE_ERROR_NONE;
}

/*!
 * @brief wall オブジェクトを DungeonDefinition の壁関連フィールドに直接フィル
 */
static errr set_dungeon_wall(DungeonDefinition &dungeon, const nlohmann::json &wall_obj)
{
    if (!wall_obj.is_object()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    if (!wall_obj.contains("tiles")) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    auto prob_table = build_probability_table_from_tiles(wall_obj["tiles"], "GRANITE");
    if (!prob_table) {
        return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
    }
    dungeon.prob_table_wall = std::move(*prob_table);

    const auto &terrains = TerrainList::get_instance();
    try {
        dungeon.outer_wall = terrains.get_terrain_id(wall_obj.value("outer", std::string("GRANITE")));
        dungeon.inner_wall = terrains.get_terrain_id(wall_obj.value("inner", std::string("GRANITE")));
        dungeon.stream1 = terrains.get_terrain_id(wall_obj.value("stream1", std::string("MAGMA_VEIN")));
        dungeon.stream2 = terrains.get_terrain_id(wall_obj.value("stream2", std::string("QUARTZ_VEIN")));
    } catch (const std::exception &) {
        return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
    }
    return PARSE_ERROR_NONE;
}

/*!
 * @brief final_floor オブジェクトをパースし、ダンジョン最下層の固有設定を直接フィル
 * @details `guardian` (MonraceId 整数) / `object` (BaseitemId 整数) / `artifact`
 *          (FixedArtifactId 整数) を受け取り、それぞれの整合性も検証する。
 */
static errr set_dungeon_final_floor(DungeonDefinition &dungeon, const nlohmann::json &final_floor_obj)
{
    if (final_floor_obj.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!final_floor_obj.is_object()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    if (final_floor_obj.contains("guardian")) {
        const auto value = final_floor_obj["guardian"].get<int>();
        const auto monrace_id = i2enum<MonraceId>(value);
        const auto &monraces = MonraceList::get_instance();
        if (!MonraceList::is_valid(monrace_id) || !monraces.contains(monrace_id)) {
            msg_format(_("不正な final_floor.guardian ID '%d'。", "Invalid final_floor.guardian ID '%d'."), value);
            return PARSE_ERROR_INVALID_FLAG;
        }
        dungeon.final_guardian = monrace_id;
    }
    if (final_floor_obj.contains("object")) {
        const auto value = final_floor_obj["object"].get<int>();
        const auto value_short = static_cast<short>(value);
        try {
            static_cast<void>(BaseitemList::get_instance().get_baseitem(value_short));
        } catch (const std::exception &) {
            msg_format(_("不正な final_floor.object ID '%d'。", "Invalid final_floor.object ID '%d'."), value);
            return PARSE_ERROR_INVALID_FLAG;
        }
        dungeon.final_object = value_short;
    }
    if (final_floor_obj.contains("artifact")) {
        const auto value = final_floor_obj["artifact"].get<int>();
        const auto artifact_id = i2enum<FixedArtifactId>(value);
        const auto &artifacts = ArtifactList::get_instance();
        if ((artifact_id != FixedArtifactId::NONE) && !artifacts.contains(artifact_id)) {
            msg_format(_("不正な final_floor.artifact ID '%d'。", "Invalid final_floor.artifact ID '%d'."), value);
            return PARSE_ERROR_INVALID_FLAG;
        }
        dungeon.final_artifact = artifact_id;
    }

    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON flags 配列、および MONSTER_RATE / TRAP_RATE 等のスカラ
 *        + FIXED_ROOM / ALLIANCE 等のサブ構造を DungeonDefinition に直接フィル
 */
static errr set_dungeon_feature_flags(DungeonDefinition &dungeon, const nlohmann::json &dungeon_data)
{
    if (dungeon_data.contains("flags")) {
        for (const auto &flag : dungeon_data["flags"]) {
            const auto f = flag.get<std::string>();
            if (f.empty()) {
                continue;
            }
            if (!grab_one_dungeon_flag(dungeon, f)) {
                return PARSE_ERROR_INVALID_FLAG;
            }
        }
    }
    if (dungeon_data.contains("monster_rate")) {
        dungeon.monster_rate = dungeon_data["monster_rate"].get<int>();
    }
    if (dungeon_data.contains("trap_rate")) {
        dungeon.trap_rate = dungeon_data["trap_rate"].get<int>();
    }
    if (dungeon_data.contains("normal_monster_rate")) {
        dungeon.normal_monster_rate = static_cast<PROB>(dungeon_data["normal_monster_rate"].get<int>());
    }
    if (dungeon_data.contains("alliance")) {
        const auto alliance_tag = dungeon_data["alliance"].get<std::string>();
        for (const auto &a : alliance_list) {
            if (a.second->tag == alliance_tag) {
                dungeon.alliance_idx = static_cast<AllianceType>(a.second->id);
                break;
            }
        }
    }
    if (dungeon_data.contains("fixed_rooms")) {
        for (const auto &fr : dungeon_data["fixed_rooms"]) {
            const auto depth = fr.value("depth", 0);
            const auto id = fr.value("id", 0);
            const auto percentage = fr.value("percentage", 0);
            dungeon.fixed_room_list.push_back(std::make_tuple(depth, id, percentage));
        }
    }
    return PARSE_ERROR_NONE;
}

/*!
 * @brief monster_flags 配列を直接フィル
 *        フラグ文字列の他、`R_CHAR_X` (出現許可シンボル) / `X_FOO` (性別) を扱う
 */
static errr set_dungeon_monster_flags(DungeonDefinition &dungeon, const nlohmann::json &flags_array)
{
    for (const auto &flag_node : flags_array) {
        const auto f = flag_node.get<std::string>();
        if (f.empty()) {
            continue;
        }

        // 後方互換: R_CHAR_xxx 記法 (新フォーマットは `symbols` 配列を使う)
        const auto &m_tokens = str_split(f, '_');
        if (m_tokens.size() >= 3 && m_tokens[0] == "R" && m_tokens[1] == "CHAR") {
            dungeon.r_chars.insert(dungeon.r_chars.end(), m_tokens[2].begin(), m_tokens[2].end());
            continue;
        }

        uint32_t sex;
        if (info_grab_one_const(sex, r_info_sex, f)) {
            dungeon.mon_sex = static_cast<MonsterSex>(sex);
            continue;
        }

        if (!grab_one_basic_monster_flag(dungeon, f)) {
            return PARSE_ERROR_INVALID_FLAG;
        }
    }
    return PARSE_ERROR_NONE;
}

/*!
 * @brief symbols 配列を直接フィル (出現許可モンスターシンボル)
 *        従来の `R_CHAR_xxx` 記法に代わる、可読性の高いシンボル配列形式。
 */
static errr set_dungeon_monster_symbols(DungeonDefinition &dungeon, const nlohmann::json &symbols_array)
{
    for (const auto &symbol_node : symbols_array) {
        const auto s = symbol_node.get<std::string>();
        if (s.empty()) {
            continue;
        }
        dungeon.r_chars.insert(dungeon.r_chars.end(), s.begin(), s.end());
    }
    return PARSE_ERROR_NONE;
}

/*!
 * @brief monster_spells 配列を直接フィル
 */
static errr set_dungeon_monster_spells(DungeonDefinition &dungeon, const nlohmann::json &flags_array)
{
    for (const auto &flag_node : flags_array) {
        const auto f = flag_node.get<std::string>();
        if (f.empty()) {
            continue;
        }
        const auto &s_tokens = str_split(f, '_');
        if (s_tokens.size() == 3 && s_tokens[1] == "IN") {
            if (s_tokens[0] != "1") {
                return PARSE_ERROR_GENERIC;
            }
            continue; // MonsterRaceDefinitions.jsonc からのコピペ対策
        }
        if (!grab_one_spell_monster_flag(dungeon, f)) {
            return PARSE_ERROR_INVALID_FLAG;
        }
    }
    return PARSE_ERROR_NONE;
}

/*!
 * @brief specific_items 配列を direct-fill (階層別アイテム生成ルール)
 */
static errr set_dungeon_specific_items(DungeonDefinition &dungeon, const nlohmann::json &items_array)
{
    for (const auto &item : items_array) {
        const auto floor_level = item.value("floor", 0);
        FloorItemGenerationRule rule;
        rule.probability = item.value("probability", 0);
        rule.dice = Dice(item.value("dice_num", 0), item.value("dice_sides", 0));
        rule.item_id = item.value("item_id", 0);
        dungeon.specific_item_generation_map[floor_level] = rule;
    }
    return PARSE_ERROR_NONE;
}

/*!
 * @brief specific_vaults 配列を direct-fill (階層別 Vault 指定)
 */
static errr set_dungeon_specific_vaults(DungeonDefinition &dungeon, const nlohmann::json &vaults_array)
{
    for (const auto &v : vaults_array) {
        const auto floor_level = v.value("floor", 0);
        const auto vault_id = v.value("vault_id", 0);
        dungeon.specific_vault_map[floor_level] = i2enum<VaultTypeId>(vault_id);
    }
    return PARSE_ERROR_NONE;
}

/*!
 * @brief room_rates 配列を direct-fill (部屋種別ごとの生成率)
 */
static errr set_dungeon_room_rates(DungeonDefinition &dungeon, const nlohmann::json &rates_array)
{
    for (const auto &r : rates_array) {
        const auto type = r.value("type", 0);
        const auto rate = r.value("rate", 0);
        dungeon.room_rate[i2enum<RoomType>(type)] = rate;
    }
    return PARSE_ERROR_NONE;
}

/*!
 * @brief position オブジェクトから wilderness 位置を初期化
 */
static void set_dungeon_position(DungeonDefinition &dungeon, const nlohmann::json &pos_obj)
{
    const auto y = pos_obj.value("y", 0);
    const auto x = pos_obj.value("x", 0);
    dungeon.initialize_position({ y, x });
}

/*!
 * @brief ダンジョン情報(JSON Object)のパース関数
 * @details 構造化 JSON (version 2) を直接 DungeonDefinition に書き込む方式
 *          (旧 token-emit 方式は提案 38 で廃止)。version 1 (lines 配列) は
 *          legacy `parse_dungeons_info()` トークンパーサを使う互換パスで継続。
 */
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

    // version 2 (structured) パス - direct fill 方式
    if (!dungeon_data.contains("id")) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    const auto id = dungeon_data["id"].get<int>();
    if (id < error_idx) {
        return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
    }
    error_idx = id;

    DungeonDefinition dungeon;

    // name (必須)
    if (dungeon_data.contains("name")) {
        auto name = read_localized_name(dungeon_data["name"]);
        if (!name) {
            return PARSE_ERROR_INVALID_FLAG;
        }
        dungeon.name = std::move(*name);
    }

    // tag
    if (dungeon_data.contains("tag")) {
        dungeon.tag = dungeon_data["tag"].get<std::string>();
    }

    // description
    if (dungeon_data.contains("description")) {
        if (auto err = set_dungeon_description(dungeon, dungeon_data["description"]); err != PARSE_ERROR_NONE) {
            return err;
        }
    }

    // position (wilderness)
    if (dungeon_data.contains("position")) {
        set_dungeon_position(dungeon, dungeon_data["position"]);
    }

    // generation
    if (dungeon_data.contains("generation")) {
        if (auto err = set_dungeon_generation(dungeon, dungeon_data["generation"]); err != PARSE_ERROR_NONE) {
            return err;
        }
    }

    // floor
    if (dungeon_data.contains("floor")) {
        if (auto err = set_dungeon_floor(dungeon, dungeon_data["floor"]); err != PARSE_ERROR_NONE) {
            return err;
        }
    }

    // wall
    if (dungeon_data.contains("wall")) {
        if (auto err = set_dungeon_wall(dungeon, dungeon_data["wall"]); err != PARSE_ERROR_NONE) {
            return err;
        }
    }

    // flags + 各種スカラ (monster_rate / trap_rate / alliance / fixed_rooms)
    if (auto err = set_dungeon_feature_flags(dungeon, dungeon_data); err != PARSE_ERROR_NONE) {
        return err;
    }

    // final_floor (最下層: ガーディアン / 報酬オブジェクト / 報酬アーティファクト)
    if (dungeon_data.contains("final_floor")) {
        if (auto err = set_dungeon_final_floor(dungeon, dungeon_data["final_floor"]); err != PARSE_ERROR_NONE) {
            return err;
        }
    }

    // monster_flags
    if (dungeon_data.contains("monster_flags")) {
        if (auto err = set_dungeon_monster_flags(dungeon, dungeon_data["monster_flags"]); err != PARSE_ERROR_NONE) {
            return err;
        }
    }

    // monster_symbols (出現許可シンボル一文字配列)
    if (dungeon_data.contains("monster_symbols")) {
        if (auto err = set_dungeon_monster_symbols(dungeon, dungeon_data["monster_symbols"]); err != PARSE_ERROR_NONE) {
            return err;
        }
    }

    // monster_spells
    if (dungeon_data.contains("monster_spells")) {
        if (auto err = set_dungeon_monster_spells(dungeon, dungeon_data["monster_spells"]); err != PARSE_ERROR_NONE) {
            return err;
        }
    }

    // specific_items
    if (dungeon_data.contains("specific_items")) {
        if (auto err = set_dungeon_specific_items(dungeon, dungeon_data["specific_items"]); err != PARSE_ERROR_NONE) {
            return err;
        }
    }

    // specific_vaults
    if (dungeon_data.contains("specific_vaults")) {
        if (auto err = set_dungeon_specific_vaults(dungeon, dungeon_data["specific_vaults"]); err != PARSE_ERROR_NONE) {
            return err;
        }
    }

    // room_rates
    if (dungeon_data.contains("room_rates")) {
        if (auto err = set_dungeon_room_rates(dungeon, dungeon_data["room_rates"]); err != PARSE_ERROR_NONE) {
            return err;
        }
    }

    auto &dungeons = DungeonList::get_instance();
    dungeons.emplace(i2enum<DungeonId>(id), std::move(dungeon));
    return PARSE_ERROR_NONE;
}
