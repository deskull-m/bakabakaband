#include "io-dump/player-status-dump-json.h"
#include "external-lib/include-json.h"
#include "locale/japanese.h"
#include "locale/localized-string.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/class-info.h"
#include "player-info/mimic-info-table.h"
#include "player/player-personality.h"
#include "player/player-realm.h"
#include "player/player-sex.h"
#include "player/race-info-table.h"
#include "system/creature-entity.h"
#include "system/dungeon/dungeon-record.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "util/enum-converter.h"
#include "world/world.h"
#include <sstream>

/*!
 * @brief 文字列をUTF-8に変換してJSONに安全に追加する
 * @param str 変換する文字列
 * @return UTF-8変換後の文字列、変換に失敗した場合は元の文字列
 */
static std::string to_utf8_safe(std::string_view str)
{
#ifdef JP
    auto utf8_str = sys_to_utf8(str);
    return utf8_str ? *utf8_str : std::string(str);
#else
    return std::string(str);
#endif
}

/*!
 * @brief LocalizedStringをUTF-8に変換してJSONに安全に追加する
 * @param ls LocalizedString
 * @return UTF-8変換後の文字列
 */
static std::string localized_to_utf8_safe(const LocalizedString &ls)
{
    return to_utf8_safe(ls.string());
}

/*!
 * @brief プレイヤーの基本情報をJSONに追加
 * @param j JSON オブジェクト
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void add_basic_info_to_json(nlohmann::json &j, CreatureEntity &player)
{
    j["basic"]["name"] = to_utf8_safe(player.name);
    j["basic"]["sex"] = localized_to_utf8_safe(sex_info[player.psex].title);
    j["basic"]["race"] = localized_to_utf8_safe(player.race->title);
    j["basic"]["class"] = localized_to_utf8_safe(class_info.at(player.pclass).title);
    j["basic"]["level"] = player.level;
    j["basic"]["experience"] = player.exp;
    j["basic"]["max_experience"] = player.max_exp;

    if (player.mimic_form != MimicKindType::NONE) {
        j["basic"]["mimic_form"] = localized_to_utf8_safe(mimic_info.at(player.mimic_form).title);
    }

    // 性格
    j["basic"]["personality"] = localized_to_utf8_safe(personality_info[player.ppersonality].title);

    // 領域
    if (player.realm1 != RealmType::NONE) {
        j["basic"]["realm1"] = localized_to_utf8_safe(PlayerRealm::get_name(player.realm1));
    }
    if (player.realm2 != RealmType::NONE) {
        j["basic"]["realm2"] = localized_to_utf8_safe(PlayerRealm::get_name(player.realm2));
    }

    // 年齢、身長、体重
    j["basic"]["age"] = player.age;
    j["basic"]["height"] = player.ht;
    j["basic"]["weight"] = player.wt;
    j["basic"]["prestige"] = player.prestige;
}

/*!
 * @brief プレイヤーの能力値をJSONに追加
 * @param j JSON オブジェクト
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void add_stats_to_json(nlohmann::json &j, CreatureEntity &player)
{
    nlohmann::json stats = nlohmann::json::array();

    const char *stat_names[6] = { "STR", "INT", "WIS", "DEX", "CON", "CHR" };
    for (int i = 0; i < A_MAX; i++) {
        nlohmann::json stat;
        stat["name"] = stat_names[i];
        stat["current"] = player.stat_cur[i];
        stat["max"] = player.stat_max[i];
        stat["use"] = player.stat_use[i];
        stat["top"] = player.stat_top[i];
        stats.push_back(stat);
    }

    j["stats"] = stats;
}

/*!
 * @brief プレイヤーの状態をJSONに追加
 * @param j JSON オブジェクト
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void add_status_to_json(nlohmann::json &j, CreatureEntity &player)
{
    j["status"]["hitpoints"] = player.hp;
    j["status"]["max_hitpoints"] = player.maxhp;
    j["status"]["mana"] = player.csp;
    j["status"]["max_mana"] = player.msp;
    j["status"]["armor_class"] = player.ac;
    j["status"]["display_armor_class"] = player.dis_ac;

    // 所持金とアイテム
    j["status"]["gold"] = player.au;

    // ダンジョン情報
    j["status"]["dungeon_level"] = player.current_floor_ptr->dun_level;
    const auto &dungeon_record = DungeonRecords::get_instance().get_record(player.recall_dungeon);
    j["status"]["max_dungeon_level"] = dungeon_record.get_max_level();

    // ターン数
    const auto &world = AngbandWorld::get_instance();
    j["status"]["game_turn"] = world.game_turn;
}

/*!
 * @brief プレイヤーの戦闘能力をJSONに追加
 * @param j JSON オブジェクト
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void add_combat_to_json(nlohmann::json &j, CreatureEntity &player)
{
    j["combat"]["base_to_hit"] = player.to_h_b;
    j["combat"]["melee_to_hit"] = player.to_h_m;
    j["combat"]["melee_to_damage"] = player.to_d_m;
    j["combat"]["ranged_to_hit"] = player.to_h_b;

    // 攻撃回数
    j["combat"]["num_blow"] = player.num_blow[0];
    j["combat"]["num_fire"] = player.num_fire;
}

/*!
 * @brief プレイヤーのスキルをJSONに追加
 * @param j JSON オブジェクト
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void add_skills_to_json(nlohmann::json &j, CreatureEntity &player)
{
    j["skills"]["fighting"] = player.skill_thn;
    j["skills"]["shooting"] = player.skill_thb;
    j["skills"]["saving_throw"] = player.skill_sav;
    j["skills"]["stealth"] = player.skill_stl;
    j["skills"]["perception"] = player.skill_fos;
    j["skills"]["searching"] = player.skill_srh;
    j["skills"]["disarming"] = player.skill_dis;
    j["skills"]["magic_device"] = player.skill_dev;
    j["skills"]["infravision"] = player.see_infra;
    j["skills"]["speed"] = player.speed - 110;
}

/*!
 * @brief プレイヤーの死亡/勝利情報をJSONに追加
 * @param j JSON オブジェクト
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void add_death_info_to_json(nlohmann::json &j, CreatureEntity &player)
{
    if (player.is_dead()) {
        j["death"]["is_dead"] = true;
        j["death"]["cause"] = to_utf8_safe(player.died_from);
        if (player.killer_monrace_id != MonraceId::PLAYER) {
            j["death"]["killer_id"] = enum2i(player.killer_monrace_id);
        }
        if (!player.last_message.empty()) {
            j["death"]["last_message"] = to_utf8_safe(player.last_message);
        }
    }

    if (player.is_true_winner()) {
        j["death"]["is_winner"] = true;
    }
}

/*!
 * @brief プレイヤーの履歴をJSONに追加
 * @param j JSON オブジェクト
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void add_history_to_json(nlohmann::json &j, CreatureEntity &player)
{
    nlohmann::json history = nlohmann::json::array();
    for (int i = 0; i < 4; i++) {
        if (player.history[i][0] != '\0') {
            history.push_back(to_utf8_safe(player.history[i]));
        }
    }
    j["history"] = history;
}

/*!
 * @brief プレイヤーのステータス情報をJSON形式で出力する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return JSON文字列
 */
std::string dump_player_status_json(CreatureEntity &creature)
{
    auto &player = creature;
    nlohmann::json j;

    // バージョン情報
    j["version"] = {
        { "format", "bakabakaband-player-status" },
        { "version", 1 }
    };

    // 基本情報
    add_basic_info_to_json(j, player);

    // 能力値
    add_stats_to_json(j, player);

    // 状態
    add_status_to_json(j, player);

    // 戦闘能力
    add_combat_to_json(j, player);

    // スキル
    add_skills_to_json(j, player);

    // 死亡/勝利情報
    add_death_info_to_json(j, player);

    // 履歴
    add_history_to_json(j, player);

    // 整形して返す
    return j.dump(2);
}

/*!
 * @brief プレイヤーのステータス情報をJSON形式でファイルに出力する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff ファイルポインタ
 */
void dump_player_status_json_to_file(CreatureEntity &creature, FILE *fff)
{
    auto json_str = dump_player_status_json(creature);
    fprintf(fff, "%s\n", json_str.c_str());
}
