#include "info-reader/general-parser.h"
#include "artifact/fixed-art-types.h"
#include "dungeon/quest.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/random-grid-effect-types.h"
#include "io/tokenizer.h"
#include "main/angband-headers.h"
#include "object-enchant/trg-types.h"
#include "player-info/class-types.h"
#include "player-info/race-types.h"
#include "realm/realm-types.h"
#include "room/vault-builder.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/building-type-definition.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/floor-info.h"
#include "system/system-variables.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "util/angband-files.h"
#include "util/string-processor.h"
#include <string>

dungeon_grid letter[255];

void dungeon_grid::set_terrain_id(TerrainTag tag)
{
    this->feature = TerrainList::get_instance().get_terrain_id(tag);
}

void dungeon_grid::set_trap_id(TerrainTag tag)
{
    this->trap = TerrainList::get_instance().get_terrain_id(tag);
}

/*!
 * @brief パース関数に基づいてデータファイルからデータを読み取る /
 * Initialize an "*_info" array, by parsing an ascii "template" file
 * @param fp 読み取りに使うファイルポインタ
 * @param buf 読み取りに使うバッファ領域
 * @param head ヘッダ構造体
 * @param parse_info_txt_line パース関数
 * @return エラーコード, エラー行番号
 */
std::tuple<errr, int> init_info_txt(FILE *fp, char *buf, angband_header *head, Parser parse_info_txt_line)
{
    error_idx = -1;
    auto error_line = 0;

    util::SHA256 sha256;

    while (true) {
        // init_info_txtの呼び出し側のbufは1024バイト
        const auto line_str = angband_fgets(fp, 1024);
        if (!line_str) {
            break;
        }
        error_line++;
        const auto len = line_str->copy(buf, 1024 - 1);
        buf[len] = '\0';
        const std::string_view line = buf;
        if (line.empty() || line.starts_with('#')) {
            continue;
        }

        if (!line.substr(1).starts_with(':')) {
            return { PARSE_ERROR_GENERIC, error_line };
        }

        if (line.starts_with('V')) {
            continue;
        }

        // 文字コードの差異を吸収するため、日本語が含まれる可能性のある
        // 「N:」「D:」「J:」はハッシュ計算から除外する
        if (!line.starts_with('N') && !line.starts_with('D') && !line.starts_with('J')) {
            sha256.update(line);
        }

        if (auto err = parse_info_txt_line(line, head); err != 0) {
            return { err, error_line };
        }
    }

    head->digest = sha256.digest();

    return { PARSE_ERROR_NONE, error_line };
}

/*!
 * @brief 地形情報の「A:」情報をパースする
 * Process "A:<ID>" -- alliance
 * @param buf 解析文字列
 * @return エラーコード
 */
parse_error_type parse_line_alliance(FloorType *floor_ptr, char *buf)
{
    char *zz[1];
    int num = tokenize(buf + 2, 6, zz, 0);
    if (num != 1) {
        return PARSE_ERROR_GENERIC;
    }
    floor_ptr->allianceID = static_cast<AllianceType>(atoi(zz[0]));
    return PARSE_ERROR_NONE;
}

/*!
 * @brief 地形情報の「V:」情報をパースする
 * Process "V:<ID>:<y>:<x>:<offsety>:<offsetx>:<transno>" -- info for vault_setting
 * @param floor_ptr 現在フロアへの参照ポインタ
 * @param buf 解析文字列
 * @return エラーコード
 */
parse_error_type parse_line_vault(FloorType *floor_ptr, char *buf)
{
    char *zz[6];
    town_vault tv;
    int num = tokenize(buf + 2, 6, zz, 0);
    if (num != 6) {
        return PARSE_ERROR_GENERIC;
    }

    tv.id = static_cast<VaultTypeId>(atoi(zz[0]));
    tv.y = atoi(zz[1]);
    tv.x = atoi(zz[2]);
    tv.yoffset = atoi(zz[3]);
    tv.xoffset = atoi(zz[4]);
    tv.transno = atoi(zz[5]);
    floor_ptr->vault_list.push_back(tv);
    return PARSE_ERROR_NONE;
}

/*!
 * @brief 地形情報の「F:」情報をパースする
 * Process "F:<letter>:<terrain>:<cave_info>:<monster>:<object>:<ego>:<artifact>:<trap>:<special>" -- info for dungeon grid
 * @param floor フロアへの参照
 * @param buf 解析文字列
 * @return エラーコード
 */
parse_error_type parse_line_feature(const FloorType &floor, char *buf)
{
    if (init_flags & INIT_ONLY_BUILDINGS) {
        return PARSE_ERROR_NONE;
    }

    char *zz[9];
    int num = tokenize(buf + 2, 9, zz, 0);
    if (num <= 1) {
        return PARSE_ERROR_GENERIC;
    }

    const auto &terrains = TerrainList::get_instance();
    int index = zz[0][0];
    letter[index].set_terrain_id(TerrainTag::NONE);
    letter[index].monster = 0;
    letter[index].object = 0;
    letter[index].ego = EgoType::NONE;
    letter[index].artifact = FixedArtifactId::NONE;
    letter[index].set_trap_id(TerrainTag::NONE);
    letter[index].cave_info = 0;
    letter[index].special = 0;
    letter[index].random = RANDOM_NONE;
    letter[index].force_monster_place = true;

    if (num >= 2) {
        if (strcmp(zz[1], "FIXED_MONSTER_PLACE") == 0) {
            letter[index].monster = (MONSTER_IDX)atoi(zz[4]);
            letter[index].cave_info = atoi(zz[3]);
            letter[index].force_monster_place = false;
            letter[index].feature = terrains.get_terrain_id(zz[2]);
            return PARSE_ERROR_NONE;
        }
    }

    switch (num) {
    case 9:
        letter[index].special = (int16_t)atoi(zz[8]);
        [[fallthrough]];
    case 8:
        if ((zz[7][0] == '*') && !zz[7][1]) {
            letter[index].random |= RANDOM_TRAP;
        } else {
            try {
                letter[index].trap = terrains.get_terrain_id(zz[7]);
            } catch (const std::exception &) {
                return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
            }
        }
        [[fallthrough]];
    case 7:
        if (zz[6][0] == '*') {
            letter[index].random |= RANDOM_ARTIFACT;
            if (zz[6][1]) {
                letter[index].artifact = i2enum<FixedArtifactId>(atoi(zz[6] + 1));
            }
        } else if (zz[6][0] == '!') {
            if (floor.is_in_quest()) {
                const auto &quests = QuestList::get_instance();
                letter[index].artifact = quests.get_quest(floor.quest_number).reward_fa_id;
            }
        } else {
            letter[index].artifact = i2enum<FixedArtifactId>(atoi(zz[6]));
        }
        [[fallthrough]];
    case 6:
        if (zz[5][0] == '*') {
            letter[index].random |= RANDOM_EGO;
            if (zz[5][1]) {
                letter[index].ego = i2enum<EgoType>(atoi(zz[5] + 1));
            }
        } else {
            letter[index].ego = i2enum<EgoType>(atoi(zz[5]));
        }
        [[fallthrough]];
    case 5:
        if (zz[4][0] == '*') {
            letter[index].random |= RANDOM_OBJECT;
            if (zz[4][1]) {
                letter[index].object = (OBJECT_IDX)atoi(zz[4] + 1);
            }
        } else if (zz[4][0] == '!') {
            if (floor.is_in_quest()) {
                const auto &quests = QuestList::get_instance();
                const auto &quest = quests.get_quest(floor.quest_number);
                if (quest.has_reward()) {
                    const auto &artifact = quest.get_reward();
                    if (artifact.gen_flags.has_not(ItemGenerationTraitType::INSTA_ART)) {
                        letter[index].object = BaseitemList::get_instance().lookup_baseitem_id(artifact.bi_key);
                    }
                }
            }
        } else {
            letter[index].object = (OBJECT_IDX)atoi(zz[4]);
        }
        [[fallthrough]];
    case 4:
        if (zz[3][0] == '*') {
            letter[index].random |= RANDOM_MONSTER;
            if (zz[3][1]) {
                letter[index].monster = (MONSTER_IDX)atoi(zz[3] + 1);
            }
        } else if (zz[3][0] == 'c') {
            if (!zz[3][1]) {
                return PARSE_ERROR_GENERIC;
            }
            letter[index].monster = -atoi(zz[3] + 1);
        } else {
            letter[index].monster = (MONSTER_IDX)atoi(zz[3]);
        }
        [[fallthrough]];
    case 3:
        letter[index].cave_info = atoi(zz[2]);
        [[fallthrough]];
    case 2:
        if ((zz[1][0] == '*') && !zz[1][1]) {
            letter[index].random |= RANDOM_FEATURE;
        } else {
            try {
                letter[index].feature = terrains.get_terrain_id(zz[1]);
            } catch (const std::exception &) {
                return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
            }
        }

        break;
    }

    return PARSE_ERROR_NONE;
}

/*!
 * @brief 地形情報の「B:」情報をパースする
 * Process "B:<Index>:<Command>:..." -- Building definition
 * @param buf 解析文字列
 * @return エラーコード
 */
parse_error_type parse_line_building(char *buf)
{
    char *zz[1000];
    char *s;

#ifdef JP
    if (buf[2] == '$') {
        return PARSE_ERROR_NONE;
    }
    s = buf + 2;
#else
    if (buf[2] != '$') {
        return PARSE_ERROR_NONE;
    }
    s = buf + 3;
#endif
    int index = atoi(s);
    s = angband_strchr(s, ':');
    if (!s) {
        return PARSE_ERROR_GENERIC;
    }

    *s++ = '\0';
    if (!*s) {
        return PARSE_ERROR_GENERIC;
    }

    switch (s[0]) {
    case 'N': {
        if (tokenize(s + 2, 3, zz, 0) == 3) {
            strcpy(buildings[index].name, zz[0]);
            strcpy(buildings[index].owner_name, zz[1]);
            strcpy(buildings[index].owner_race, zz[2]);
            break;
        }

        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    case 'A': {
        if (tokenize(s + 2, 8, zz, 0) >= 7) {
            int action_index = atoi(zz[0]);
            strcpy(buildings[index].act_names[action_index], zz[1]);
            buildings[index].member_costs[action_index] = (PRICE)atoi(zz[2]);
            buildings[index].other_costs[action_index] = (PRICE)atoi(zz[3]);
            buildings[index].letters[action_index] = zz[4][0];
            buildings[index].actions[action_index] = static_cast<int16_t>(atoi(zz[5]));
            buildings[index].action_restr[action_index] = static_cast<int16_t>(atoi(zz[6]));
            break;
        }

        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    case 'C': {
        auto pct_max = PLAYER_CLASS_TYPE_MAX;
        auto n = tokenize(s + 2, pct_max, zz, 0);
        for (auto i = 0; i < pct_max; i++) {
            buildings[index].member_class[i] = (i < n) ? atoi(zz[i]) : 1;
        }

        break;
    }
    case 'R': {
        auto n = tokenize(s + 2, MAX_RACES, zz, 0);
        for (int i = 0; i < MAX_RACES; i++) {
            buildings[index].member_race[i] = (i < n) ? atoi(zz[i]) : 1;
        }

        break;
    }
    case 'M': {
        int n;
        n = tokenize(s + 2, MAX_MAGIC, zz, 0);
        for (int i = 0; i < MAX_MAGIC; i++) {
            buildings[index].member_realm[i + 1] = ((i < n) ? static_cast<int16_t>(atoi(zz[i])) : 1);
        }

        break;
    }
    case 'Z': {
        break;
    }
    default: {
        return PARSE_ERROR_UNDEFINED_DIRECTIVE;
    }
    }

    return PARSE_ERROR_NONE;
}
