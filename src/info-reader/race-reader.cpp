#include "info-reader/race-reader.h"
#include "alliance/alliance.h"
#include "artifact/fixed-art-types.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/race-info-tokens-table.h"
#include "locale/japanese.h"
#include "main/angband-headers.h"
#include "player-ability/player-ability-types.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monrace/monrace-message.h"
#include "term/gameterm.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <string>

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(モンスター用1) /
 * Grab one (basic) flag in a MonraceDefinition from a textual string
 * @param monrace 保管先のモンスター種族構造体
 * @param what 参照元の文字列ポインタ
 * @return 見つけたらtrue
 */
static bool grab_one_basic_flag(MonraceDefinition &monrace, std::string_view what)
{
    if (EnumClassFlagGroup<MonsterFeedType>::grab_one_flag(monrace.meat_feed_flags, r_info_meat_feed, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterResistanceType>::grab_one_flag(monrace.resistance_flags, r_info_flagsr, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterAuraType>::grab_one_flag(monrace.aura_flags, r_info_aura_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterBehaviorType>::grab_one_flag(monrace.behavior_flags, r_info_behavior_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterVisualType>::grab_one_flag(monrace.visual_flags, r_info_visual_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterKindType>::grab_one_flag(monrace.kind_flags, r_info_kind_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterEraType>::grab_one_flag(monrace.era_flags, r_info_era_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterDropType>::grab_one_flag(monrace.drop_flags, r_info_drop_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterWildernessType>::grab_one_flag(monrace.wilderness_flags, r_info_wilderness_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterFeatureType>::grab_one_flag(monrace.feature_flags, r_info_feature_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterPopulationType>::grab_one_flag(monrace.population_flags, r_info_population_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterSpeakType>::grab_one_flag(monrace.speak_flags, r_info_speak_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterBrightnessType>::grab_one_flag(monrace.brightness_flags, r_info_brightness_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterSpecialType>::grab_one_flag(monrace.special_flags, r_info_special_flags, what)) {
        return true;
    }
    if (EnumClassFlagGroup<MonsterMiscType>::grab_one_flag(monrace.misc_flags, r_info_misc_flags, what)) {
        return true;
    }
    msg_format(_("モンスターフラグ読込失敗'%d'。", "Failed to load monster flag data.'%d'."), what);
    return false;
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(モンスター用2) /
 * Grab one (spell) flag in a MonraceDefinition from a textual string
 * @param monrace 保管先のモンスター種族構造体
 * @param what 参照元の文字列ポインタ
 * @return 見つけたらtrue
 */
static bool grab_one_spell_flag(MonraceDefinition &monrace, std::string_view what)
{
    if (EnumClassFlagGroup<MonsterAbilityType>::grab_one_flag(monrace.ability_flags, r_info_ability_flags, what)) {
        return true;
    }

    msg_format(_("未知のモンスター・フラグ '%s'。", "Unknown monster flag '%s'."), what.data());
    return false;
}

/*!
 * @brief JSON Objectからモンスター名をセットする
 * @param name_data 名前情報の格納されたJSON Object
 * @param monrace 保管先のモンスター種族構造体
 * @return エラーコード
 */
static errr set_mon_name(const nlohmann::json &name_data, MonraceDefinition &monrace)
{
    if (name_data.is_null()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    if (!name_data["ja"].is_string() || !name_data["en"].is_string()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto ja_name = name_data["ja"].get<std::string>();
    const auto en_name = name_data["en"].get<std::string>();

#ifdef JP
    auto ja_name_sys = utf8_to_sys(ja_name);
    if (!ja_name_sys) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    monrace.name = { *ja_name_sys, en_name };
#else
    monrace.name = { "", en_name };
#endif
    /*
        } else if (tokens[0] == "T") {
            if (tokens.size() < 2 || tokens[1].size() == 0) {
                return PARSE_ERROR_TOO_FEW_ARGUMENTS;
            }
            r_ptr->tag = tokens[1];
        } else if (tokens[0] == "D") {
            // D:text_ja
            // D:$text_en
            if (tokens.size() < 2 || buf.length() < 3) {
    */
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからモンスターシンボルをセットする
 * @param symbol_data シンボル情報の格納されたJSON Object
 * @param monrace 保管先のモンスター種族構造体
 * @return エラーコード
 */
static errr set_mon_symbol(const nlohmann::json &symbol_data, MonraceDefinition &monrace)
{
    if (symbol_data.is_null()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto &character_obj = symbol_data["character"];
    if (!character_obj.is_string()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto &color_obj = symbol_data["color"];
    if (!color_obj.is_string()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto color = color_list.find(color_obj.get<std::string>());
    if (color == color_list.end()) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    if (color->second > 127) {
        return PARSE_ERROR_GENERIC;
    }

    monrace.symbol_definition = { color->second, character_obj.get<std::string>().front() };
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからモンスター速度をセットする
 * @param speed_data 速度情報の格納されたJSON Object
 * @param monrace 保管先のモンスター種族構造体
 * @return エラーコード
 */
static errr set_mon_speed(const nlohmann::json &speed_data, MonraceDefinition &monrace)
{
    int speed;
    if (auto err = info_set_integer(speed_data, speed, true, Range(-50, 99))) {
        return err;
    }
    monrace.speed = speed + STANDARD_SPEED;
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからモンスターの進化をセットする
 * @param evolve_data 進化情報の格納されたJSON Object
 * @param monrace 保管先のモンスター種族構造体
 * @return エラーコード
 */
static errr set_mon_evolve(nlohmann::json &evolve_data, MonraceDefinition &monrace)
{
    if (evolve_data.is_null()) {
        return PARSE_ERROR_NONE;
    }

    if (auto err = info_set_integer(evolve_data["need_exp"], monrace.next_exp, true, Range(0, 9999999))) {
        return err;
    }
    if (auto err = info_set_integer(evolve_data["to"], monrace.next_r_idx, true, Range(0, 9999))) {
        return err;
    }

    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからモンスターの性別をセットする
 * @param sex_data 性別情報の格納されたJSON Object
 * @param monrace 保管先のモンスター種族構造体
 * @return エラーコード
 */
static errr set_mon_sex(const nlohmann::json &sex_data, MonraceDefinition &monrace)
{
    if (sex_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!sex_data.is_string()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    uint32_t sex;
    if (!info_grab_one_const(sex, r_info_sex, sex_data.get<std::string>())) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    monrace.sex = static_cast<MonsterSex>(sex);
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからモンスターの固定アーティファクトドロップ情報をセットする
 * @param artifact_data 固定アーティファクトドロップ情報の格納されたJSON Object
 * @param monrace 保管先のモンスター種族構造体
 * @return エラーコード
 */
static errr set_mon_artifacts(nlohmann::json &artifact_data, MonraceDefinition &monrace)
{
    if (artifact_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!artifact_data.is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    for (auto &artifact : artifact_data.items()) {
        FixedArtifactId fa_id;
        if (auto err = info_set_integer(artifact.value()["drop_artifact_id"], fa_id, true, Range(0, 1024))) {
            return err;
        }
        int prob;
        if (auto err = info_set_integer(artifact.value()["drop_probability"], prob, true, Range(1, 100))) {
            return err;
        }

        monrace.drop_artifacts.emplace_back(fa_id, prob);
    }
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからモンスターの護衛情報をセットする
 * @param escort_data 護衛情報の格納されたJSON Object
 * @param monrace 保管先のモンスター種族構造体
 * @return エラーコード
 */
static errr set_mon_escorts(nlohmann::json &escort_data, MonraceDefinition &monrace)
{
    if (escort_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!escort_data.is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    for (const auto &escort : escort_data.items()) {
        MonraceId monrace_id;
        if (auto err = info_set_integer(escort.value()["escorts_id"], monrace_id, true, Range(0, 8192))) {
            return err;
        }

        Dice dice;
        if (auto err = info_set_dice(escort.value()["escort_num"], dice, true)) {
            return err;
        }

        monrace.emplace_reinforce(monrace_id, dice);
    }
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからモンスターの打撃攻撃をセットする
 * @param blow_data 打撃攻撃情報の格納されたJSON Object
 * @param monrace 保管先のモンスター種族構造体
 * @return エラーコード
 */
static errr set_mon_blows(nlohmann::json &blow_data, MonraceDefinition &monrace)
{
    if (blow_data.is_null()) {
        monrace.behavior_flags.set(MonsterBehaviorType::NEVER_BLOW);
        return PARSE_ERROR_NONE;
    }
    if (!blow_data.is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    for (auto blow_num = 0; auto &blow : blow_data.items()) {
        if (blow_num > 5) {
            return PARSE_ERROR_GENERIC;
        }

        const auto &blow_method = blow.value()["method"];
        const auto &blow_effect = blow.value()["effect"];
        if (blow_method.is_null() || blow_effect.is_null()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto rbm = r_info_blow_method.find(blow_method.get<std::string>());
        if (rbm == r_info_blow_method.end()) {
            return PARSE_ERROR_INVALID_FLAG;
        }

        const auto rbe = r_info_blow_effect.find(blow_effect.get<std::string>());
        if (rbe == r_info_blow_effect.end()) {
            return PARSE_ERROR_INVALID_FLAG;
        }
        auto &mon_blow = monrace.blows[blow_num];
        mon_blow.method = rbm->second;
        mon_blow.effect = rbe->second;

        if (auto err = info_set_dice(blow.value()["damage_dice"], mon_blow.damage_dice, false)) {
            return err;
        }

        blow_num++;
    }
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからモンスターフラグをセットする
 * @param flag_data モンスターフラグ情報の格納されたJSON Object
 * @param monrace 保管先のモンスター種族構造体
 * @return エラーコード
 */
static errr set_mon_flags(const nlohmann::json &flag_data, MonraceDefinition &monrace)
{
    if (flag_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!flag_data.is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    /*
                if (f.size() == 0) {
                    continue;
                }

                const auto &s_tokens = str_split(f, '_', false);

                if (s_tokens.size() == 2 && s_tokens[0] == "PERHP") {
                    info_set_value(r_ptr->cur_hp_per, s_tokens[1]);
                    continue;
                }

                if (s_tokens.size() == 2 && s_tokens[0] == "ALLIANCE") {
                    for (auto a : alliance_list) {
                        if (a.second->tag == s_tokens[1]) {
                            r_ptr->alliance_idx = static_cast<AllianceType>(a.second->id);
                        }
                    }
                    continue;
                }

                if (s_tokens.size() == 2 && s_tokens[0] == "FATHER") {
                    info_set_value(r_ptr->father, s_tokens[1]);
                    continue;
                }

                if (s_tokens.size() == 2 && s_tokens[0] == "MOTHER") {
                    info_set_value(r_ptr->mother, s_tokens[1]);
                    continue;
                }

                if (s_tokens.size() == 2 && s_tokens[0] == "COLLAPSE-OVER") {
                    const auto &p_tokens = str_split(s_tokens[1], '.', false);
                    if (p_tokens.size() != 2) {
                        return PARSE_ERROR_INVALID_FLAG;
                    }
                    if (p_tokens[1].size() > 6) {
                        return PARSE_ERROR_INVALID_FLAG;
                    }
                    int mul = 6 - static_cast<int>(p_tokens[1].size());
                    int deci, fraq;
                    info_set_value(deci, p_tokens[0]);
                    info_set_value(fraq, p_tokens[1]);
                    for (int i = 0; i < mul; i++) {
                        fraq *= 10;
                    }
                    r_ptr->collapse_over = deci * 1000000 + fraq;
                    continue;
                }

                if (s_tokens.size() == 2 && s_tokens[0] == "COLLAPSE") {
                    const auto &p_tokens = str_split(s_tokens[1], '.', false);
                    if (p_tokens.size() != 2) {
                        return PARSE_ERROR_INVALID_FLAG;
                    }
                    if (p_tokens[1].size() > 6) {
                        return PARSE_ERROR_INVALID_FLAG;
                    }
                    int mul = 6 - static_cast<int>(p_tokens[1].size());
                    int deci, fraq;
                    bool is_plus = p_tokens[0][0] != '-';
                    info_set_value(deci, p_tokens[0]);
                    info_set_value(fraq, p_tokens[1]);
                    for (int i = 0; i < mul; i++) {
                        fraq *= 10;
                    }
                    r_ptr->plus_collapse = is_plus ? deci * 1000000 + fraq : deci * 1000000 - fraq;
                    continue;
                }

                if (s_tokens.size() == 2 && s_tokens[0] == "SUICIDE") {
                    // ターン後自滅
                    int num, side;
                    const auto &dices = str_split(s_tokens[1], 'd', true, 10);
                    if (dices.size() != 2) {
                        return PARSE_ERROR_INVALID_FLAG;
                    }
                    info_set_value(num, dices[0]);
                    info_set_value(side, dices[1]);
                    r_ptr->suicide_dice_num = num;
                    r_ptr->suicide_dice_side = side;
                    continue;
                }

                if (s_tokens.size() == 6 && s_tokens[0] == "SPAWN") {
                    // 落とし子自動生成率
                    if (s_tokens[1] == "CREATURE" && s_tokens[3] == "IN") {
                        int num;
                        int deno;
                        MonraceId mon_idx;
                        info_set_value(num, s_tokens[2]);
                        info_set_value(deno, s_tokens[4]);
                        info_set_value(mon_idx, s_tokens[5]);
                        r_ptr->spawn_monsters.push_back({ num, deno, mon_idx });
                        continue;
                    }

                    // 地形変化率
                    if (s_tokens[1] == "FEATURE" && s_tokens[3] == "IN") {
                        int num;
                        int deno;
                        FEAT_IDX feat_idx;
                        info_set_value(num, s_tokens[2]);
                        info_set_value(deno, s_tokens[4]);
                        info_set_value(feat_idx, s_tokens[5]);
                        r_ptr->change_feats.push_back({ num, deno, feat_idx });
                        continue;
                    }

                    // アイテム自然ドロップ率
                    if (s_tokens[1] == "ITEM" && s_tokens[3] == "IN") {
                        int num;
                        int deno;
                        short kind_idx;
                        info_set_value(num, s_tokens[2]);
                        info_set_value(deno, s_tokens[4]);
                        info_set_value(kind_idx, s_tokens[5]);
                        r_ptr->spawn_items.push_back({ num, deno, kind_idx });
                        continue;
                    }
                }

                if (s_tokens.size() == 8 && s_tokens[0] == "DROP" && s_tokens[1] == "KIND" && s_tokens[3] == "IN") {
                    int num;
                    int deno;
                    int dn;
                    int ds;
                    int grade;
                    short kind_idx;
                    info_set_value(num, s_tokens[2]);
                    info_set_value(deno, s_tokens[4]);
                    info_set_value(kind_idx, s_tokens[5]);
                    info_set_value(grade, s_tokens[6]);
                    const auto &dices = str_split(s_tokens[7], 'd', true, 10);
                    if (dices.size() != 2) {
                        return PARSE_ERROR_INVALID_FLAG;
                    }
                    info_set_value(dn, dices[0]);
                    info_set_value(ds, dices[1]);
                    r_ptr->drop_kinds.push_back({ num, deno, kind_idx, grade, ds, dn });
                    continue;
                }

                if (s_tokens.size() == 8 && s_tokens[0] == "DROP" && s_tokens[1] == "TVAL" && s_tokens[3] == "IN") {
                    int num;
                    int deno;
                    int dn;
                    int ds;
                    int grade;
                    short kind_idx;
                    info_set_value(num, s_tokens[2]);
                    info_set_value(deno, s_tokens[4]);
                    info_set_value(kind_idx, s_tokens[5]);
                    info_set_value(grade, s_tokens[6]);
                    const auto &dices = str_split(s_tokens[7], 'd', true, 10);
                    if (dices.size() != 2) {
                        return PARSE_ERROR_INVALID_FLAG;
                    }
                    info_set_value(dn, dices[0]);
                    info_set_value(ds, dices[1]);
                    r_ptr->drop_tvals.push_back({ num, deno, kind_idx, grade, ds, dn });
                    continue;
                }
                if (s_tokens.size() == 7 && s_tokens[0] == "DEAD" && s_tokens[1] == "SPAWN" && s_tokens[3] == "IN") {
                    int num;
                    int deno;
                    int dn;
                    int ds;
                    MonraceId r_idx;
                    info_set_value(num, s_tokens[2]);
                    info_set_value(deno, s_tokens[4]);
                    info_set_value(r_idx, s_tokens[5]);
                    const auto &dices = str_split(s_tokens[6], 'd', true, 10);
                    if (dices.size() != 2) {
                        return PARSE_ERROR_INVALID_FLAG;
                    }
                    info_set_value(dn, dices[0]);
                    info_set_value(ds, dices[1]);
                    r_ptr->dead_spawns.push_back({ num, deno, r_idx, ds, dn });
                    continue;
                }

                if (grab_one_basic_flag(r_ptr, f)) {
                    continue;
                }

                uint32_t sex;
                if (!info_grab_one_const(sex, r_info_sex, f)) {
                    return PARSE_ERROR_INVALID_FLAG;
                }

                r_ptr->sex = static_cast<MonsterSex>(sex);
            }

        } else if (tokens[0] == "S") {
            // S:flags
            if (tokens.size() < 2 || tokens[1].size() == 0) {
                return PARSE_ERROR_TOO_FEW_ARGUMENTS;
            }

            const auto &flags = str_split(tokens[1], '|', true, 10);
            for (const auto &f : flags) {
                if (f.size() == 0) {
                    continue;
                }

                const auto &s_tokens = str_split(f, '_', false, 3);

                // 特殊行動確率
                if (s_tokens.size() == 3 && s_tokens[1] == "IN") {
                    if (s_tokens[0] != "1") {
                        return PARSE_ERROR_GENERIC;
                    }
                    RARITY i;
                    info_set_value(i, s_tokens[2]);
                    r_ptr->freq_spell = 100 / i;
                    continue;
                }

                if (!grab_one_spell_flag(r_ptr, f)) {
                    return PARSE_ERROR_INVALID_FLAG;
                }
            }

        } else if (tokens[0] == "A") {
            // A:artifact_idx:chance
            if (tokens.size() < 3) {
                return PARSE_ERROR_TOO_FEW_ARGUMENTS;
            }

            FixedArtifactId a_idx;
            PERCENTAGE chance;
            info_set_value(a_idx, tokens[1]);
            info_set_value(chance, tokens[2]);
            r_ptr->drop_artifacts.emplace_back(a_idx, chance);
        } else if (tokens[0] == "X") {
            if (tokens.size() < 2) {
                return PARSE_ERROR_TOO_FEW_ARGUMENTS;
            }
            uint32_t sex;
            if (!info_grab_one_const(sex, r_info_sex, tokens[1])) {
                return PARSE_ERROR_INVALID_FLAG;
            }
            r_ptr->sex = static_cast<MonsterSex>(sex);
        } else if (tokens[0] == "V") {
            // V:arena_odds
            if (tokens.size() < 2) {
                return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    */
    for (auto &flag : flag_data.items()) {
        if (!grab_one_basic_flag(monrace, flag.value().get<std::string>())) {
            return PARSE_ERROR_INVALID_FLAG;
        }
    }
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからモンスターの発動能力をセットする
 * @param skill_data 発動能力情報の格納されたJSON Object
 * @param monrace 保管先のモンスター種族構造体
 * @return エラーコード
 */
static errr set_mon_skills(const nlohmann::json &skill_data, MonraceDefinition &monrace)
{
    if (skill_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!skill_data.is_object()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto &prob = skill_data["probability"];
    if (!prob.is_string()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto &prob_token = str_split(prob.get<std::string>(), '_', false, 2);
    if (prob_token.size() == 3 && prob_token[1] == "IN") {
        if (prob_token[0] != "1") {
            return PARSE_ERROR_GENERIC;
        }
        byte denominator;
        info_set_value(denominator, prob_token[2]);
        monrace.freq_spell = 100 / denominator;
    }

    const auto &shoot_dice = skill_data.find("shoot");
    const auto shoot = (shoot_dice != skill_data.end());
    if (shoot) {
        if (auto ret = info_set_dice(shoot_dice->get<std::string>(), monrace.shoot_damage_dice, true)) {
            return ret;
        }
        monrace.ability_flags.set(MonsterAbilityType::SHOOT);
    }

    const auto &skill_list = skill_data.find("list");
    if (skill_list == skill_data.end()) {
        if (!shoot) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
        return PARSE_ERROR_NONE;
    }

    for (auto &skill : skill_list->items()) {
        if (!grab_one_spell_flag(monrace, skill.value().get<std::string>())) {
            return PARSE_ERROR_INVALID_FLAG;
        }
    }
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからモンスターのアライアンス情報をセットする
 * @param skill_data 発動能力情報の格納されたJSON Object
 * @param monrace 保管先のモンスター種族構造体
 * @return エラーコード
 */
static errr set_mon_alliance(const nlohmann::json &alliance_data, MonraceDefinition &monrace)
{
    if (alliance_data.is_null()) {
        return PARSE_ERROR_NONE;
    }

    for (auto a : alliance_list) {
        if (a.second->tag == alliance_data.get<std::string>()) {
            monrace.alliance_idx = static_cast<AllianceType>(a.second->id);
            return PARSE_ERROR_NONE;
        }
    }
    return PARSE_ERROR_INVALID_FLAG;
}

/*
 * @brief JSON Objectからモンスターの死亡時召喚情報をセットする
 * @param summon_data 死亡時召喚情報の格納されたJSON Object
 * @param monrace 保管先のモンスター種族構造体
 * @return エラーコード
 */
static errr set_mon_final_summons(const nlohmann::json &summon_data, MonraceDefinition &monrace)
{
    if (summon_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!summon_data.is_array()) {
        return PARSE_ERROR_INVALID_TYPE;
    }

    for (auto &summon_item : summon_data) {
        int id, probability, min_num, max_num, radius;
        if (auto err = info_set_integer(summon_item["id"], id, true, Range(1, 9999))) {
            return err;
        }
        if (auto err = info_set_integer(summon_item["probability"], probability, true, Range(1, 100))) {
            return err;
        }
        if (auto err = info_set_integer(summon_item["min_num"], min_num, true, Range(0, 99))) {
            return err;
        }
        if (auto err = info_set_integer(summon_item["max_num"], max_num, true, Range(1, 99))) {
            return err;
        }
        if (min_num > max_num) {
            return PARSE_ERROR_INVALID_VALUE;
        }
        if (auto err = info_set_integer(summon_item["radius"], radius, true, Range(1, 20))) {
            return err;
        }
        monrace.emplace_final_summon(i2enum<MonraceId>(id), probability, min_num, max_num, radius);
    }
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからモンスターのメッセージをセットする
 * @param message_data メッセージ情報の格納されたJSON Object
 * @param monrace 保管先のモンスター種族構造体
 * @return エラーコード
 */
static errr set_mon_message(const nlohmann::json &message_data, MonraceDefinition &monrace)
{
    if (message_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!message_data.is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    for (auto &message : message_data.items()) {
        const auto &action_str = message.value()["action"];
        if (action_str.is_null()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
        const auto action = r_info_message_flags.find(action_str.get<std::string>());
        if (action == r_info_message_flags.end()) {
            return PARSE_ERROR_INVALID_FLAG;
        }

        int chance;
        if (auto err = info_set_integer(message.value()["chance"], chance, true, Range(1, 100))) {
            return err;
        }

        bool use_name = true;
        const auto use_name_iter = message.value().find("use_name");
        if (use_name_iter != message.value().end()) {
            const auto &use_name_data = use_name_iter.value();
            if (auto err = info_set_bool(use_name_data, use_name, false)) {
                return err;
            }
        }

        std::string str;
        if (auto err = info_set_string(message.value()["message"], str, false)) {
            return err;
        }

        MonraceMessageList::get_instance().emplace((int)monrace.idx, action->second, chance, use_name, str);
    }
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからモンスターの地形変化率をセットする
 * @param terrain_data 地形変化情報の格納されたJSON Object
 * @param monrace 保管先のモンスター種族構造体
 * @return エラーコード
 */
static errr set_mon_terrain_feature(const nlohmann::json &terrain_data, MonraceDefinition &monrace)
{
    if (terrain_data.is_null()) {
        return PARSE_ERROR_NONE;
    }

    if (!terrain_data.is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    for (const auto &terrain : terrain_data) {
        if (!terrain.contains("id") || !terrain.contains("probability")) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        FEAT_IDX feature_id;
        if (auto err = info_set_integer(terrain["id"], feature_id, true, Range(0, 9999))) {
            return err;
        }

        const auto &probability_str = terrain["probability"].get<std::string>();

        // "X_IN_Y" 形式をパース
        size_t in_pos = probability_str.find("_IN_");
        if (in_pos == std::string::npos) {
            return PARSE_ERROR_INVALID_FLAG;
        }

        const auto numer_str = probability_str.substr(0, in_pos);
        const auto denom_str = probability_str.substr(in_pos + 4); // "_IN_" を除去

        int numerator, denominator;
        try {
            numerator = std::stoi(numer_str);
            denominator = std::stoi(denom_str);
        } catch (...) {
            return PARSE_ERROR_INVALID_FLAG;
        }

        if (numerator <= 0 || denominator <= 0) {
            return PARSE_ERROR_INVALID_FLAG;
        }

        // 分子、分母、地形IDを設定
        monrace.change_feats.push_back({ numerator, denominator, feature_id });
    }

    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからモンスターのモンスター自然生成率をセットする
 * @param spawn_data 自然生成情報の格納されたJSON Object
 * @param monrace 保管先のモンスター種族構造体
 * @return エラーコード
 */
static errr set_mon_spawn_creature(const nlohmann::json &spawn_data, MonraceDefinition &monrace)
{
    if (spawn_data.is_null()) {
        return PARSE_ERROR_NONE;
    }

    if (!spawn_data.is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    for (const auto &spawn : spawn_data) {
        if (!spawn.contains("id") || !spawn.contains("probability")) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        MonraceId monrace_id;
        if (auto err = info_set_integer(spawn["id"], monrace_id, true, Range(0, 9999))) {
            return err;
        }

        const auto &probability_str = spawn["probability"].get<std::string>();

        // "X_IN_Y" 形式をパース
        size_t in_pos = probability_str.find("_IN_");
        if (in_pos == std::string::npos) {
            return PARSE_ERROR_INVALID_FLAG;
        }

        const auto numer_str = probability_str.substr(0, in_pos);
        const auto denom_str = probability_str.substr(in_pos + 4); // "_IN_" を除去

        int numerator, denominator;
        try {
            numerator = std::stoi(numer_str);
            denominator = std::stoi(denom_str);
        } catch (...) {
            return PARSE_ERROR_INVALID_FLAG;
        }

        if (numerator <= 0 || denominator <= 0) {
            return PARSE_ERROR_INVALID_FLAG;
        }

        // 分子、分母、モンスターIDを設定
        monrace.spawn_monsters.push_back({ numerator, denominator, monrace_id });
    }

    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからモンスターのモンスター自然生成アイテム率をセットする
 * @param spawn_data 自然生成情報の格納されたJSON Object
 * @param monrace 保管先のモンスター種族構造体
 * @return エラーコード
 */
static errr set_mon_spawn_item(const nlohmann::json &spawn_data, MonraceDefinition &monrace)
{
    if (spawn_data.is_null()) {
        return PARSE_ERROR_NONE;
    }

    if (!spawn_data.is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    for (const auto &spawn : spawn_data) {
        if (!spawn.contains("id") || !spawn.contains("probability")) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        short item_id;
        if (auto err = info_set_integer(spawn["id"], item_id, true, Range(0, 9999))) {
            return err;
        }

        const auto &probability_str = spawn["probability"].get<std::string>();

        // "X_IN_Y" 形式をパース
        size_t in_pos = probability_str.find("_IN_");
        if (in_pos == std::string::npos) {
            return PARSE_ERROR_INVALID_FLAG;
        }

        const auto numer_str = probability_str.substr(0, in_pos);
        const auto denom_str = probability_str.substr(in_pos + 4); // "_IN_" を除去

        int numerator, denominator;
        try {
            numerator = std::stoi(numer_str);
            denominator = std::stoi(denom_str);
        } catch (...) {
            return PARSE_ERROR_INVALID_FLAG;
        }

        if (numerator <= 0 || denominator <= 0) {
            return PARSE_ERROR_INVALID_FLAG;
        }

        // 分子、分母、アイテムIDを設定
        monrace.spawn_items.push_back({ numerator, denominator, item_id });
    }

    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからモンスターの死亡時召喚情報をセットする
 * @param dead_spawn_data 死亡時召喚情報の格納されたJSON Object
 * @param monrace 保管先のモンスター種族構造体
 * @return エラーコード
 */
static errr set_mon_dead_spawns(const nlohmann::json &dead_spawn_data, MonraceDefinition &monrace)
{
    if (dead_spawn_data.is_null()) {
        return PARSE_ERROR_NONE;
    }

    if (!dead_spawn_data.is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    for (const auto &dead_spawn : dead_spawn_data) {
        if (!dead_spawn.contains("id") || !dead_spawn.contains("probability") || !dead_spawn.contains("dice")) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        MonraceId monrace_id;
        if (auto err = info_set_integer(dead_spawn["id"], monrace_id, true, Range(0, 9999))) {
            return err;
        }

        const auto &probability_str = dead_spawn["probability"].get<std::string>();

        // "X_IN_Y" 形式をパース
        size_t in_pos = probability_str.find("_IN_");
        if (in_pos == std::string::npos) {
            return PARSE_ERROR_INVALID_FLAG;
        }

        const auto numer_str = probability_str.substr(0, in_pos);
        const auto denom_str = probability_str.substr(in_pos + 4); // "_IN_" を除去

        int numerator, denominator;
        try {
            numerator = std::stoi(numer_str);
            denominator = std::stoi(denom_str);
        } catch (...) {
            return PARSE_ERROR_INVALID_FLAG;
        }

        if (numerator <= 0 || denominator <= 0) {
            return PARSE_ERROR_INVALID_FLAG;
        }

        // ダイス情報の解析 ("XdY" 形式)
        const auto &dice_str = dead_spawn["dice"].get<std::string>();
        size_t d_pos = dice_str.find('d');
        if (d_pos == std::string::npos) {
            return PARSE_ERROR_INVALID_FLAG;
        }

        const auto dice_num_str = dice_str.substr(0, d_pos);
        const auto dice_side_str = dice_str.substr(d_pos + 1);

        int dice_num, dice_side;
        try {
            dice_num = std::stoi(dice_num_str);
            dice_side = std::stoi(dice_side_str);
        } catch (...) {
            return PARSE_ERROR_INVALID_FLAG;
        }

        if (dice_num <= 0 || dice_side <= 0) {
            return PARSE_ERROR_INVALID_FLAG;
        }

        // 分子、分母、モンスターID、ダイス面数、ダイス個数を設定
        monrace.dead_spawns.push_back({ numerator, denominator, monrace_id, dice_side, dice_num });
    }

    return PARSE_ERROR_NONE;
}

/*!
 * @brief モンスター種族情報(JSON Object)のパース関数
 * @param mon_data モンスターデータの格納されたJSON Object
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_monraces_info(nlohmann::json &mon_data, angband_header *)
{
    if (!mon_data["id"].is_number_integer()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto monrace_id_int = mon_data["id"].get<int>();
    if (monrace_id_int < error_idx) {
        return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
    }

    error_idx = monrace_id_int;
    const auto monrace_id = i2enum<MonraceId>(monrace_id_int);
    auto &monraces = MonraceList::get_instance();
    auto &monrace = monraces.emplace(monrace_id);
    monrace.idx = monrace_id;

    errr err;
    err = set_mon_name(mon_data["name"], monrace);
    if (err) {
        msg_format(_("モンスター名読込失敗。ID: '%d'。", "Failed to load monster name. ID: '%d'."), error_idx);
        return err;
    }
    err = set_mon_symbol(mon_data["symbol"], monrace);
    if (err) {
        msg_format(_("モンスターシンボル読込失敗。ID: '%d'。", "Failed to load monster symbol. ID: '%d'."), error_idx);
        return err;
    }
    err = set_mon_speed(mon_data["speed"], monrace);
    if (err) {
        msg_format(_("モンスター速度読込失敗。ID: '%d'。", "Failed to load monster speed. ID: '%d'."), error_idx);
        return err;
    }
    err = info_set_dice(mon_data["hit_point"], monrace.hit_dice, true);
    if (err) {
        msg_format(_("モンスターHP読込失敗。ID: '%d'。", "Failed to load monster HP. ID: '%d'."), error_idx);
        return err;
    }
    err = info_set_integer(mon_data["vision"], monrace.aaf, true, Range(0, 999));
    if (err) {
        msg_format(_("モンスター感知範囲読込失敗。ID: '%d'。", "Failed to load monster vision. ID: '%d'."), error_idx);
        return err;
    }
    err = info_set_integer(mon_data["armor_class"], monrace.ac, true, Range(0, 10000));
    if (err) {
        msg_format(_("モンスターAC読込失敗。ID: '%d'。", "Failed to load monster AC. ID: '%d'."), error_idx);
        return err;
    }
    err = info_set_integer(mon_data["alertness"], monrace.sleep, true, Range(0, 255));
    if (err) {
        msg_format(_("モンスター警戒度読込失敗。ID: '%d'。", "Failed to load monster alertness. ID: '%d'."), error_idx);
        return err;
    }
    err = info_set_integer(mon_data["level"], monrace.level, true, Range(0, 255));
    if (err) {
        msg_format(_("モンスターレベル読込失敗。ID: '%d'。", "Failed to load monster level. ID: '%d'."), error_idx);
        return err;
    }
    err = info_set_integer(mon_data["rarity"], monrace.rarity, true, Range(0, 255));
    if (err) {
        msg_format(_("モンスター希少度読込失敗。ID: '%d'。", "Failed to load monster rarity. ID: '%d'."), error_idx);
        return err;
    }
    if (err) {
        msg_format(_("モンスター希少度範囲外。ID: '%d'。", "Monster rarity is out of range. ID: '%d'."), error_idx);
        return err;
    }
    err = info_set_integer(mon_data["exp"], monrace.mexp, true, Range(0, 9999999));
    if (err) {
        msg_format(_("モンスター経験値読込失敗。ID: '%d'。", "Failed to load monster exp. ID: '%d'."), error_idx);
        return err;
    }
    err = set_mon_evolve(mon_data["evolve"], monrace);
    if (err) {
        msg_format(_("モンスター進化情報読込失敗。ID: '%d'。", "Failed to load monster evolve data. ID: '%d'."), error_idx);
        return err;
    }
    err = set_mon_sex(mon_data["sex"], monrace);
    if (err) {
        msg_format(_("モンスター性別読込失敗。ID: '%d'。", "Failed to load monster sex. ID: '%d'."), error_idx);
        return err;
    }
    err = info_set_integer(mon_data["odds_correction_ratio"], monrace.arena_ratio, false, Range(1, 9999));
    if (err) {
        msg_format(_("モンスター賭け倍率読込失敗。ID: '%d'。", "Failed to load monster odds for arena. ID: '%d'."), error_idx);
        return err;
    }
    err = info_set_integer(mon_data["start_hp_percentage"], monrace.cur_hp_per, false, Range(0, 99));
    if (err) {
        msg_format(_("モンスター初期体力読込失敗。ID: '%d'。", "Failed to load monster starting HP. ID: '%d'."), error_idx);
        return err;
    }
    err = info_set_integer(mon_data["mob"], monrace.mob_num, false, Range(0, 9999999));
    if (err) {
        msg_format(_("モンスター基本モブ数読込失敗。ID: '%d'。", "Failed to load monster base mob number. ID: '%d'."), error_idx);
        return err;
    } else {
        monrace.max_num = monrace.mob_num;
    }
    err = set_mon_artifacts(mon_data["artifacts"], monrace);
    if (err) {
        msg_format(_("モンスター固定アーティファクトドロップ情報読込失敗。ID: '%d'。", "Failed to load monster artifact drop data. ID: '%d'."), error_idx);
        return err;
    }
    err = set_mon_escorts(mon_data["escorts"], monrace);
    if (err) {
        msg_format(_("モンスター護衛情報読込失敗。ID: '%d'。", "Failed to load monster escorts. ID: '%d'."), error_idx);
        return err;
    }
    err = set_mon_blows(mon_data["blows"], monrace);
    if (err) {
        msg_format(_("モンスター打撃情報読込失敗。ID: '%d'。", "Failed to load monster blow data. ID: '%d'."), error_idx);
        return err;
    }
    err = set_mon_flags(mon_data["flags"], monrace);
    if (err) {
        msg_format(_("モンスターフラグ読込失敗。ID: '%d'。", "Failed to load monster flag data. ID: '%d'."), error_idx);
        return err;
    }
    err = set_mon_skills(mon_data["skill"], monrace);
    if (err) {
        msg_format(_("モンスター発動能力情報読込失敗。ID: '%d'。", "Failed to load monster skill data. ID: '%d'."), error_idx);
        return err;
    }
    err = set_mon_final_summons(mon_data["final_summon"], monrace);
    if (err) {
        msg_format(_("モンスター死亡時召喚情報読込失敗。ID: '%d'。", "Failed to load final summon data. ID: '%d'."), error_idx);
        return err;
    }
    err = info_set_string(mon_data["flavor"], monrace.text, false);
    if (err) {
        msg_format(_("モンスター説明文読込失敗。ID: '%d'。", "Failed to load monster flavor text. ID: '%d'."), error_idx);
        return err;
    }
    err = set_mon_alliance(mon_data["alliance"], monrace);
    if (err) {
        msg_format(_("モンスターアライアンス情報読込失敗。ID: '%d'。", "Failed to load monster alliance: '%d'."), error_idx);
    }
    err = set_mon_message(mon_data["message"], monrace);
    if (err) {
        msg_format(_("モンスターメッセージ読込失敗。ID: '%d'。", "Failed to load monster message. ID: '%d'."), error_idx);
        return err;
    }
    err = set_mon_terrain_feature(mon_data["terrain_feature"], monrace);
    if (err) {
        msg_format(_("モンスター地形変化情報読込失敗。ID: '%d'。", "Failed to load monster terrain feature data. ID: '%d'."), error_idx);
        return err;
    }
    err = set_mon_spawn_creature(mon_data["spawn_creature"], monrace);
    if (err) {
        msg_format(_("モンスター自然生成情報読み込み失敗。ID: '%d'。", "Failed to load monster spawn creature data. ID: '%d'."), error_idx);
        return err;
    }
    err = set_mon_spawn_item(mon_data["spawn_item"], monrace);
    if (err) {
        msg_format(_("モンスター自然生成アイテム情報読み込み失敗。ID: '%d'。", "Failed to load monster spawn item data. ID: '%d'."), error_idx);
        return err;
    }
    err = set_mon_dead_spawns(mon_data["dead_spawn"], monrace);
    if (err) {
        msg_format(_("モンスター死亡時召喚情報読み込み失敗。ID: '%d'。", "Failed to load monster dead spawn data. ID: '%d'."), error_idx);
        return err;
    }

    return PARSE_ERROR_NONE;
}
