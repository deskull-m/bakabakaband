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
#include "system/monrace/monrace-definition.h"
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
 * @brief クリーチャーの基本情報を JSON に追加
 * @param j JSON オブジェクト
 * @param creature クリーチャーへの参照
 */
static void add_basic_info_to_json(nlohmann::json &j, CreatureEntity &creature)
{
    j["basic"]["name"] = to_utf8_safe(creature.name);
    j["basic"]["level"] = creature.get_level();
    j["basic"]["experience"] = creature.get_exp();
    j["basic"]["max_experience"] = creature.get_max_exp();
    j["basic"]["age"] = creature.get_age();
    j["basic"]["height"] = creature.get_ht();
    j["basic"]["weight"] = creature.get_wt();
    j["basic"]["prestige"] = creature.get_prestige();

    if (creature.race != nullptr) {
        j["basic"]["race"] = localized_to_utf8_safe(creature.get_race_info()->title);
    }
    if (creature.pclass_ref != nullptr) {
        j["basic"]["class"] = localized_to_utf8_safe(creature.get_class_info()->title);
    }

    // モンスターはプレイヤー固有の性別・性格・魔法領域・変身形態を持たない
    if (!creature.is_player()) {
        const auto &monrace = creature.get_monrace();
        j["basic"]["monrace"] = localized_to_utf8_safe(monrace.name);
        return;
    }

    j["basic"]["sex"] = localized_to_utf8_safe(creature.get_sex_info().title);
    j["basic"]["personality"] = localized_to_utf8_safe(personality_info[creature.ppersonality].title);

    if (creature.get_mimic_form() != MimicKindType::NONE) {
        j["basic"]["mimic_form"] = localized_to_utf8_safe(mimic_info.at(creature.get_mimic_form()).title);
    }

    if (creature.realm1 != RealmType::NONE) {
        j["basic"]["realm1"] = localized_to_utf8_safe(PlayerRealm::get_name(creature.realm1));
    }
    if (creature.realm2 != RealmType::NONE) {
        j["basic"]["realm2"] = localized_to_utf8_safe(PlayerRealm::get_name(creature.realm2));
    }
}

/*!
 * @brief クリーチャーの能力値を JSON に追加
 * @param j JSON オブジェクト
 * @param creature クリーチャーへの参照
 */
static void add_stats_to_json(nlohmann::json &j, CreatureEntity &creature)
{
    nlohmann::json stats = nlohmann::json::array();

    const char *stat_names[6] = { "STR", "INT", "WIS", "DEX", "CON", "CHR" };
    for (int i = 0; i < A_MAX; i++) {
        nlohmann::json stat;
        stat["name"] = stat_names[i];
        stat["current"] = creature.stat_cur[i];
        stat["max"] = creature.stat_max[i];
        stat["use"] = creature.stat_use[i];
        stat["top"] = creature.stat_top[i];
        stats.push_back(stat);
    }

    j["stats"] = stats;
}

/*!
 * @brief クリーチャーの状態を JSON に追加
 * @param j JSON オブジェクト
 * @param creature クリーチャーへの参照
 */
static void add_status_to_json(nlohmann::json &j, CreatureEntity &creature)
{
    j["status"]["hitpoints"] = creature.hp;
    j["status"]["max_hitpoints"] = creature.maxhp;
    j["status"]["mana"] = creature.get_csp();
    j["status"]["max_mana"] = creature.get_msp();
    j["status"]["armor_class"] = creature.ac;
    j["status"]["display_armor_class"] = creature.dis_ac;

    // 所持金とアイテム
    j["status"]["gold"] = creature.get_au();

    // ダンジョン情報
    j["status"]["dungeon_level"] = creature.get_floor()->dun_level;
    const auto &dungeon_record = DungeonRecords::get_instance().get_record(creature.recall_dungeon);
    j["status"]["max_dungeon_level"] = dungeon_record.get_max_level();

    // ターン数
    const auto &world = AngbandWorld::get_instance();
    j["status"]["game_turn"] = world.game_turn;
}

/*!
 * @brief クリーチャーの戦闘能力を JSON に追加
 * @param j JSON オブジェクト
 * @param creature クリーチャーへの参照
 */
static void add_combat_to_json(nlohmann::json &j, CreatureEntity &creature)
{
    j["combat"]["base_to_hit"] = creature.to_h_b;
    j["combat"]["melee_to_hit"] = creature.to_h_m;
    j["combat"]["melee_to_damage"] = creature.to_d_m;
    j["combat"]["ranged_to_hit"] = creature.to_h_b;

    // 攻撃回数
    j["combat"]["num_blow"] = creature.num_blow[0];
    j["combat"]["num_fire"] = creature.num_fire;
}

/*!
 * @brief クリーチャーのスキルを JSON に追加
 * @param j JSON オブジェクト
 * @param creature クリーチャーへの参照
 */
static void add_skills_to_json(nlohmann::json &j, CreatureEntity &creature)
{
    j["skills"]["fighting"] = creature.skill_thn;
    j["skills"]["shooting"] = creature.skill_thb;
    j["skills"]["saving_throw"] = creature.skill_sav;
    j["skills"]["stealth"] = creature.skill_stl;
    j["skills"]["perception"] = creature.skill_fos;
    j["skills"]["searching"] = creature.skill_srh;
    j["skills"]["disarming"] = creature.skill_dis;
    j["skills"]["magic_device"] = creature.skill_dev;
    j["skills"]["infravision"] = creature.see_infra;
    j["skills"]["speed"] = creature.speed - 110;
}

/*!
 * @brief クリーチャーの死亡/勝利情報を JSON に追加
 * @param j JSON オブジェクト
 * @param creature クリーチャーへの参照
 */
static void add_death_info_to_json(nlohmann::json &j, CreatureEntity &creature)
{
    if (creature.is_dead()) {
        j["death"]["is_dead"] = true;
        j["death"]["cause"] = to_utf8_safe(creature.died_from);
        if (creature.killer_monrace_id != MonraceId::PLAYER) {
            j["death"]["killer_id"] = enum2i(creature.killer_monrace_id);
        }
        if (!creature.last_message.empty()) {
            j["death"]["last_message"] = to_utf8_safe(creature.last_message);
        }
    }

    if (creature.is_true_winner()) {
        j["death"]["is_winner"] = true;
    }
}

/*!
 * @brief クリーチャーの履歴を JSON に追加
 * @param j JSON オブジェクト
 * @param creature クリーチャーへの参照
 */
static void add_history_to_json(nlohmann::json &j, CreatureEntity &creature)
{
    nlohmann::json history = nlohmann::json::array();
    for (int i = 0; i < 4; i++) {
        if (creature.history[i][0] != '\0') {
            history.push_back(to_utf8_safe(creature.history[i]));
        }
    }
    j["history"] = history;
}

/*!
 * @brief クリーチャー（プレイヤー・モンスター）のステータス情報を JSON 形式で出力する
 * @param creature クリーチャーへの参照
 * @return JSON 文字列
 * @details モンスターは性別・種族等一部のフィールドのみ出力し、プレイヤー専用のセクション
 * (sex/personality/realm/mimic_form) はスキップする。
 */
std::string dump_player_status_json(CreatureEntity &creature)
{
    nlohmann::json j;

    // バージョン情報
    j["version"] = {
        { "format", "bakabakaband-creature-status" },
        { "version", 1 }
    };

    // 基本情報
    add_basic_info_to_json(j, creature);

    // 能力値
    add_stats_to_json(j, creature);

    // 状態
    add_status_to_json(j, creature);

    // 戦闘能力
    add_combat_to_json(j, creature);

    // スキル
    add_skills_to_json(j, creature);

    // 死亡/勝利情報
    add_death_info_to_json(j, creature);

    // 履歴
    add_history_to_json(j, creature);

    // 整形して返す
    return j.dump(2);
}

/*!
 * @brief クリーチャー（プレイヤー・モンスター）のステータス情報を JSON 形式でファイルに出力する
 * @param creature クリーチャーへの参照
 * @param fff ファイルポインタ
 */
void dump_player_status_json_to_file(CreatureEntity &creature, FILE *fff)
{
    auto json_str = dump_player_status_json(creature);
    fprintf(fff, "%s\n", json_str.c_str());
}
