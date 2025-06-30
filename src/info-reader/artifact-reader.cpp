#include "info-reader/artifact-reader.h"
#include "artifact/fixed-art-types.h"
#include "artifact/random-art-effects.h"
#include "info-reader/baseitem-tokens-table.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "locale/japanese.h"
#include "main/angband-headers.h"
#include "object-enchant/tr-types.h"
#include "system/artifact-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(アーティファクト用) /
 * Grab one activation index flag
 * @param a_ptr 保管先のアーティファクト構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return 見つかったらtrue
 */
static bool grab_one_artifact_flag(ArtifactType *a_ptr, std::string_view what)
{
    if (TrFlags::grab_one_flag(a_ptr->flags, baseitem_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<ItemGenerationTraitType>::grab_one_flag(a_ptr->gen_flags, baseitem_geneneration_flags, what)) {
        return true;
    }

    msg_format(_("未知の伝説のアイテム・フラグ '%s'。", "Unknown artifact flag '%s'."), what.data());
    return false;
}

/*!
 * @brief JSON Objectから固定アーティファクトのベースアイテムをセットする
 * @param baseitem_data ベースアイテム情報の格納されたJSON Object
 * @param artifact 保管先のアーティファクト
 * @return エラーコード
 */
static errr set_art_baseitem(nlohmann::json &baseitem_data, ArtifactType &artifact)
{
    if (baseitem_data.is_null()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    ItemKindType type_value;
    if (auto err = info_set_integer(baseitem_data["type_value"], type_value, true, Range(0, 128))) {
        return err;
    }

    int subtype_value;
    if (auto err = info_set_integer(baseitem_data["subtype_value"], subtype_value, true, Range(0, 128))) {
        return err;
    }

    artifact.bi_key = { type_value, subtype_value };
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectから固定アーティファクトの発動能力をセットする
 * @param act_data 発動能力情報の格納されたJSON Object
 * @param artifact 保管先のアーティファクト
 * @return エラーコード
 */
static errr set_art_activate(const nlohmann::json &act_data, ArtifactType &artifact)
{
    if (!act_data.is_string()) {
        return PARSE_ERROR_NONE;
    }

    auto activation = grab_one_activation_flag(act_data.get<std::string>());
    if (activation <= RandomArtActType::NONE) {
        return PARSE_ERROR_INVALID_FLAG;
    }

    artifact.act_idx = activation;
    artifact.flags.set(tr_type::TR_ACTIVATE);
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからモンスターフラグをセットする
 * @param flag_data モンスターフラグ情報の格納されたJSON Object
 * @param artifact 保管先のアーティファクト
 * @return エラーコード
 */
static errr set_art_flags(const nlohmann::json &flag_data, ArtifactType &artifact)
{
    if (flag_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!flag_data.is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    for (auto &flag : flag_data) {
        if (!grab_one_artifact_flag(&artifact, flag.get<std::string>())) {
            return PARSE_ERROR_INVALID_FLAG;
        }
    }
    return PARSE_ERROR_NONE;
}

/*!
 * @brief 固定アーティファクト情報(JSON Object)のパース関数
 * @param art_data 固定アーティファクトデータの格納されたJSON Object
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_artifacts_info(nlohmann::json &art_data, angband_header *)
{
    if (!art_data["id"].is_number_integer()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto int_id = art_data["id"].get<int>();
    if (int_id < error_idx) {
        return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
    }
    const auto artifact_id = i2enum<FixedArtifactId>(int_id);

    error_idx = int_id;
    ArtifactType artifact;
    artifact.flags.set(TR_IGNORE_ACID);
    artifact.flags.set(TR_IGNORE_ELEC);
    artifact.flags.set(TR_IGNORE_FIRE);
    artifact.flags.set(TR_IGNORE_COLD);

    if (auto err = info_set_string(art_data["name"], artifact.name, true)) {
        msg_format(_("アーティファクトの名称読込失敗。ID: '%d'。", "Failed to load artifact name. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_art_baseitem(art_data["base_item"], artifact)) {
        msg_format(_("アーティファクトのベースアイテム読込失敗。ID: '%d'。", "Failed to load base item of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(art_data["parameter_value"], artifact.pval, false, Range(-128, 128))) {
        msg_format(_("アーティファクトのパラメータ値読込失敗。ID: '%d'。", "Failed to load parameter value of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(art_data["level"], artifact.level, true, Range(0, 128))) {
        msg_format(_("アーティファクトのレベル読込失敗。ID: '%d'。", "Failed to load level of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(art_data["rarity"], artifact.rarity, true)) {
        msg_format(_("アーティファクトの希少度読込失敗。ID: '%d'。", "Failed to load rarity of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(art_data["weight"], artifact.weight, true, Range(0, 9999))) {
        msg_format(_("アーティファクトの重量読込失敗。ID: '%d'。", "Failed to load weight of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(art_data["cost"], artifact.cost, true, Range(0, 99999999))) {
        msg_format(_("アーティファクトの売値読込失敗。ID: '%d'。", "Failed to load cost of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(art_data["base_ac"], artifact.ac, false, Range(-99, 99))) {
        msg_format(_("アーティファクトのベースAC読込失敗。ID: '%d'。", "Failed to load base AC of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_dice(art_data["base_dice"], artifact.damage_dice, false)) {
        msg_format(_("アーティファクトのベースダイス読込失敗。ID: '%d'。", "Failed to load base dice of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(art_data["hit_bonus"], artifact.to_h, false, Range(-99, 99))) {
        msg_format(_("アーティファクトの命中補正値読込失敗。ID: '%d'。", "Failed to load hit bonus of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(art_data["damage_bonus"], artifact.to_d, false, Range(-99, 99))) {
        msg_format(_("アーティファクトの命中補正値読込失敗。ID: '%d'。", "Failed to load damage bonus of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(art_data["ac_bonus"], artifact.to_a, false, Range(-999, 999))) {
        msg_format(_("アーティファクトのAC補正値読込失敗。ID: '%d'。", "Failed to load AC bonus of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_art_activate(art_data["activate"], artifact)) {
        msg_format(_("アーティファクトの発動能力読込失敗。ID: '%d'。", "Failed to load activate ability of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_art_flags(art_data["flags"], artifact)) {
        msg_format(_("アーティファクトの能力フラグ読込失敗。ID: '%d'。", "Failed to load ability flags of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_string(art_data["flavor"], artifact.text, false)) {
        msg_format(_("アーティファクトのフレーバーテキスト読込失敗。ID: '%d'。", "Failed to load flavor text of artifact. ID: '%d'."), error_idx);
        return err;
    }
    /*
        if (tokens[0] == "D") {
            // D:JapaneseText
            // D:$EnglishText
            if (tokens.size() < 2 || buf.length() < 3) {
                return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
            }
    #ifdef JP
            if (buf[2] == '$') {
                return PARSE_ERROR_NONE;
            }

            const auto it = artifacts_info.rbegin();
            auto &artifact = it->second;
            artifact.text.append(buf.substr(2));
    #else
            if (buf[2] != '$') {
                return PARSE_ERROR_NONE;
            }
            if (buf.length() == 3) {
                return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
            }

            const auto it = artifacts_info.rbegin();
            auto &artifact = it->second;
            append_english_text(artifact.text, buf.substr(3));
    #endif
            return PARSE_ERROR_NONE;
        }

        if (tokens[0] == "I") {
            // I:tval:sval:pval
            if (tokens.size() < 4) {
                return PARSE_ERROR_TOO_FEW_ARGUMENTS;
            }

            const auto it = artifacts_info.rbegin();
            auto &artifact = it->second;
            constexpr auto base = 10;
            const auto tval = i2enum<ItemKindType>(std::stoi(tokens[1], nullptr, base));
            const auto sval = std::stoi(tokens[2], nullptr, base);
            artifact.bi_key = { tval, sval };
            info_set_value(artifact.pval, tokens[3]);
            return PARSE_ERROR_NONE;
        }

        if (tokens[0] == "W") {
            // W:level:ratiry:weight:cost
            if (tokens.size() < 5) {
                return PARSE_ERROR_TOO_FEW_ARGUMENTS;
            }

            const auto it = artifacts_info.rbegin();
            auto &artifact = it->second;
            info_set_value(artifact.level, tokens[1]);
            info_set_value(artifact.rarity, tokens[2]);
            info_set_value(artifact.weight, tokens[3]);
            info_set_value(artifact.cost, tokens[4]);
            return PARSE_ERROR_NONE;
        }

        if (tokens[0] == "P") {
            // P:ac:dd:ds:to_h:to_d:to_a
            if (tokens.size() < 6) {
                return PARSE_ERROR_TOO_FEW_ARGUMENTS;
            }

            const auto &dice = str_split(tokens[2], 'd', false, 2);
            if (dice.size() != 2) {
                return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
            }

            const auto it = artifacts_info.rbegin();
            auto &artifact = it->second;
            info_set_value(artifact.ac, tokens[1]);
            info_set_value(artifact.dd, dice[0]);
            info_set_value(artifact.ds, dice[1]);
            info_set_value(artifact.to_h, tokens[3]);
            info_set_value(artifact.to_d, tokens[4]);
            info_set_value(artifact.to_a, tokens[5]);
            return PARSE_ERROR_NONE;
        }

        if (tokens[0] == "U") {
            // U:activation_flag
            if (tokens.size() < 2 || tokens[1].size() == 0) {
                return PARSE_ERROR_TOO_FEW_ARGUMENTS;
            }

            auto n = grab_one_activation_flag(tokens[1]);
            if (n <= RandomArtActType::NONE) {
                return PARSE_ERROR_INVALID_FLAG;
            }

            const auto it = artifacts_info.rbegin();
            auto &artifact = it->second;
            artifact.act_idx = n;
            return PARSE_ERROR_NONE;
        }

        if (tokens[0] == "F") {
            // F:flags
            if (tokens.size() < 2 || tokens[1].size() == 0) {
                return PARSE_ERROR_TOO_FEW_ARGUMENTS;
            }

            const auto &flags = str_split(tokens[1], '|', true, 10);
            for (const auto &f : flags) {
                if (f.size() == 0) {
                    continue;
                }

                const auto it = artifacts_info.rbegin();
                auto *a_ptr = &it->second;
                if (!grab_one_artifact_flag(a_ptr, f)) {
                    return PARSE_ERROR_INVALID_FLAG;
                }
            }

        } else if (tokens[0] == "B") {
            // B:activate broken rate
            if (tokens.size() < 2 || tokens[1].size() == 0) {
                return PARSE_ERROR_TOO_FEW_ARGUMENTS;
            }
            const auto it = artifacts_info.rbegin();
            auto *a_ptr = &it->second;
            info_set_value(a_ptr->broken_rate, tokens[1]);
        } else {
            return PARSE_ERROR_UNDEFINED_DIRECTIVE;
        }
    */
    ArtifactList::get_instance().emplace(artifact_id, std::move(artifact));
    return PARSE_ERROR_NONE;
}
