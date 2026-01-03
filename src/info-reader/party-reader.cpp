#include "info-reader/party-reader.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "locale/localized-string.h"
#include "main/angband-headers.h"
#include "system/creature-party/creature-party-definition.h"
#include "system/creature-party/creature-party-list.h"
#include "system/enums/monrace/monrace-id.h"
#include "util/dice.h"
#include "util/enum-converter.h"
#include "view/display-messages.h"

/*!
 * @brief パーティ名をJSONから読み込む
 * @param name_data パーティ名のJSON Object
 * @param party パーティ定義への参照
 * @return エラーコード
 */
static errr set_party_name(nlohmann::json &name_data, CreaturePartyDefinition &party)
{
    if (!name_data.is_object()) {
        return PARSE_ERROR_GENERIC;
    }

    if (!name_data.contains("ja") || !name_data["ja"].is_string()) {
        return PARSE_ERROR_GENERIC;
    }

    std::string ja_name = name_data["ja"].get<std::string>();
    std::string en_name = ja_name; // デフォルトは日本語名と同じ

    if (name_data.contains("en") && name_data["en"].is_string()) {
        en_name = name_data["en"].get<std::string>();
    }

    party.name = LocalizedString(ja_name, en_name);

    return PARSE_ERROR_NONE;
}

/*!
 * @brief パーティメンバー情報をJSONから読み込む
 * @param members_data パーティメンバー配列のJSON Array
 * @param party パーティ定義への参照
 * @return エラーコード
 */
static errr set_party_members(nlohmann::json &members_data, CreaturePartyDefinition &party)
{
    if (!members_data.is_array()) {
        return PARSE_ERROR_GENERIC;
    }

    for (auto &member_data : members_data) {
        if (!member_data.is_object()) {
            return PARSE_ERROR_GENERIC;
        }

        // monrace_idの読み込み
        if (!member_data.contains("monrace_id") || !member_data["monrace_id"].is_number_integer()) {
            msg_format(_("パーティメンバーのモンスターID読込失敗。", "Failed to load party member monster ID."));
            return PARSE_ERROR_GENERIC;
        }
        const auto monrace_id = i2enum<MonraceId>(member_data["monrace_id"].get<int>());

        // count_diceの読み込み
        Dice count_dice;
        const auto dice_err = info_set_dice(member_data["count_dice"], count_dice, true);
        if (dice_err) {
            msg_format(_("パーティメンバーの出現数ダイス読込失敗。", "Failed to load party member count dice."));
            return dice_err;
        }

        // probabilityの読み込み
        if (!member_data.contains("probability") || !member_data["probability"].is_number_integer()) {
            msg_format(_("パーティメンバーの出現確率読込失敗。", "Failed to load party member probability."));
            return PARSE_ERROR_GENERIC;
        }
        const auto probability = member_data["probability"].get<int>();
        if (probability < 0 || probability > 100) {
            msg_format(_("パーティメンバーの出現確率が範囲外: %d", "Party member probability out of range: %d"), probability);
            return PARSE_ERROR_GENERIC;
        }

        party.add_member(monrace_id, count_dice, probability);
    }

    return PARSE_ERROR_NONE;
}

/*!
 * @brief パーティ情報(JSON Object)のパース関数
 * @param party_data パーティデータの格納されたJSON Object
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_creature_parties_info(nlohmann::json &party_data, angband_header *)
{
    if (!party_data["id"].is_number_integer()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto party_id = party_data["id"].get<int>();
    if (party_id < error_idx) {
        return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
    }

    error_idx = party_id;
    auto &parties = CreaturePartyList::get_instance();
    auto &party = parties.emplace(party_id);
    party.idx = party_id;

    // パーティ名の読み込み
    errr err = set_party_name(party_data["name"], party);
    if (err) {
        msg_format(_("パーティ名読込失敗。ID: '%d'。", "Failed to load party name. ID: '%d'."), error_idx);
        return err;
    }

    // タグの読み込み
    if (party_data.contains("tag") && party_data["tag"].is_string()) {
        party.tag = party_data["tag"].get<std::string>();
    }

    // 階層レベルの読み込み
    err = info_set_integer(party_data["min_level"], party.min_level, true, Range(0, 999));
    if (err) {
        msg_format(_("パーティ最小レベル読込失敗。ID: '%d'。", "Failed to load party min level. ID: '%d'."), error_idx);
        return err;
    }

    err = info_set_integer(party_data["max_level"], party.max_level, true, Range(0, 999));
    if (err) {
        msg_format(_("パーティ最大レベル読込失敗。ID: '%d'。", "Failed to load party max level. ID: '%d'."), error_idx);
        return err;
    }

    // レアリティの読み込み
    err = info_set_integer(party_data["rarity"], party.rarity, true, Range(1, 255));
    if (err) {
        msg_format(_("パーティレアリティ読込失敗。ID: '%d'。", "Failed to load party rarity. ID: '%d'."), error_idx);
        return err;
    }

    // ユニーク許可フラグの読み込み
    if (party_data.contains("allow_unique") && party_data["allow_unique"].is_boolean()) {
        party.allow_unique = party_data["allow_unique"].get<bool>();
    }

    // パーティメンバーの読み込み
    if (!party_data.contains("members")) {
        msg_format(_("パーティメンバー情報なし。ID: '%d'。", "No party members. ID: '%d'."), error_idx);
        return PARSE_ERROR_GENERIC;
    }

    err = set_party_members(party_data["members"], party);
    if (err) {
        msg_format(_("パーティメンバー読込失敗。ID: '%d'。", "Failed to load party members. ID: '%d'."), error_idx);
        return err;
    }

    return PARSE_ERROR_NONE;
}
