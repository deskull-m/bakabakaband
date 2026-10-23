#include "info-reader/party-reader.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "locale/localized-string.h"
#include "system/creature-party/creature-party-definition.h"
#include "system/creature-party/creature-party-list.h"
#include "system/enums/monrace/monrace-id.h"
#include "util/dice.h"
#include "util/enum-converter.h"
#include "view/display-messages.h"
#include <nlohmann/json.hpp>

/*!
 * @brief パーティ名をJSONから読み込む
 * @param name_data パーティ名のJSON Object
 * @param party パーティ定義への参照
 * @return エラーコード
 */
static ParseErrorType set_party_name(nlohmann::json &name_data, CreaturePartyDefinition &party)
{
    if (!name_data.is_object()) {
        return ParseErrorType::GENERIC;
    }

    if (!name_data.contains("ja") || !name_data["ja"].is_string()) {
        return ParseErrorType::GENERIC;
    }

    std::string ja_name = name_data["ja"].get<std::string>();
    std::string en_name = ja_name; // デフォルトは日本語名と同じ

    if (name_data.contains("en") && name_data["en"].is_string()) {
        en_name = name_data["en"].get<std::string>();
    }

    party.name = LocalizedString(ja_name, en_name);

    return ParseErrorType::NONE;
}

/*!
 * @brief パーティメンバー情報をJSONから読み込む
 * @param members_data パーティメンバー配列のJSON Array
 * @param party パーティ定義への参照
 * @return エラーコード
 */
static ParseErrorType set_party_members(nlohmann::json &members_data, CreaturePartyDefinition &party)
{
    if (!members_data.is_array()) {
        return ParseErrorType::GENERIC;
    }

    for (auto &member_data : members_data) {
        if (!member_data.is_object()) {
            return ParseErrorType::GENERIC;
        }

        // monrace_idの読み込み
        if (!member_data.contains("monrace_id") || !member_data["monrace_id"].is_number_integer()) {
            msg_print(_("パーティメンバーのモンスターID読込失敗。", "Failed to load party member monster ID."));
            return ParseErrorType::GENERIC;
        }
        const auto monrace_id = i2enum<MonraceId>(member_data["monrace_id"].get<int>());

        // count_diceの読み込み
        Dice count_dice;
        if (const auto dice_err = info_set_dice(member_data["count_dice"], count_dice, true); !dice_err) {
            msg_print(_("パーティメンバーの出現数ダイス読込失敗。", "Failed to load party member count dice."));
            return dice_err.error();
        }

        // probabilityの読み込み
        if (!member_data.contains("probability") || !member_data["probability"].is_number_integer()) {
            msg_print(_("パーティメンバーの出現確率読込失敗。", "Failed to load party member probability."));
            return ParseErrorType::GENERIC;
        }
        const auto probability = member_data["probability"].get<int>();
        if (probability < 0 || probability > 100) {
            msg_print(_("パーティメンバーの出現確率が範囲外: {}", "Party member probability out of range: {}"), probability);
            return ParseErrorType::GENERIC;
        }

        party.add_member(monrace_id, count_dice, probability);
    }

    return ParseErrorType::NONE;
}

/*!
 * @brief パーティ情報(JSON Object)のパース関数
 * @param party_data パーティデータの格納されたJSON Object
 * @return エラーコード
 */
ParseErrorType parse_creature_parties_info(nlohmann::json &party_data)
{
    if (!party_data["id"].is_number_integer()) {
        return ParseErrorType::TOO_FEW_ARGUMENTS;
    }

    const auto party_id = party_data["id"].get<int>();
    if (party_id < error_idx) {
        return ParseErrorType::NON_SEQUENTIAL_RECORDS;
    }

    error_idx = party_id;
    auto &parties = CreaturePartyList::get_instance();
    auto &party = parties.emplace(party_id);
    party.idx = party_id;

    // パーティ名の読み込み
    ParseErrorType err = set_party_name(party_data["name"], party);
    if (err != ParseErrorType::NONE) {
        msg_print(_("パーティ名読込失敗。ID: '{}'。", "Failed to load party name. ID: '{}'."), error_idx);
        return err;
    }

    // タグの読み込み
    if (party_data.contains("tag") && party_data["tag"].is_string()) {
        party.tag = party_data["tag"].get<std::string>();
    }

    // 階層レベルの読み込み
    if (const auto result = info_set_integer(party_data["min_level"], party.min_level, true, Range(0, 999)); !result) {
        msg_print(_("パーティ最小レベル読込失敗。ID: '{}'。", "Failed to load party min level. ID: '{}'."), error_idx);
        return result.error();
    }

    if (const auto result = info_set_integer(party_data["max_level"], party.max_level, true, Range(0, 999)); !result) {
        msg_print(_("パーティ最大レベル読込失敗。ID: '{}'。", "Failed to load party max level. ID: '{}'."), error_idx);
        return result.error();
    }

    // レアリティの読み込み
    if (const auto result = info_set_integer(party_data["rarity"], party.rarity, true, Range(1, 255)); !result) {
        msg_print(_("パーティレアリティ読込失敗。ID: '{}'。", "Failed to load party rarity. ID: '{}'."), error_idx);
        return result.error();
    }

    // ユニーク許可フラグの読み込み
    if (party_data.contains("allow_unique") && party_data["allow_unique"].is_boolean()) {
        party.allow_unique = party_data["allow_unique"].get<bool>();
    }

    // パーティメンバーの読み込み
    if (!party_data.contains("members")) {
        msg_print(_("パーティメンバー情報なし。ID: '{}'。", "No party members. ID: '{}'."), error_idx);
        return ParseErrorType::GENERIC;
    }

    err = set_party_members(party_data["members"], party);
    if (err != ParseErrorType::NONE) {
        msg_print(_("パーティメンバー読込失敗。ID: '{}'。", "Failed to load party members. ID: '{}'."), error_idx);
        return err;
    }

    return ParseErrorType::NONE;
}
