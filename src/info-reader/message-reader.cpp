#include "info-reader/message-reader.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/race-info-tokens-table.h"
#include "locale/character-encoding.h"
#include "system/monrace/monrace-message.h"
#include "view/display-messages.h"
#include <nlohmann/json.hpp>
#include <string>

MessageReader::MessageReader(const nlohmann::json &message_data)
    : message_data(message_data)
{
}

/*!
 * @brief モンスターメッセージ情報(JSON Object)のパース関数
 * @return エラーコード
 */
ParseErrorType MessageReader::read() const
{
    if (const auto err = this->set_mon_message(); err != ParseErrorType::NONE) {
        msg_print(_("モンスターメッセージ読込失敗。", "Failed to load monster message."));
        return err;
    }

    return ParseErrorType::NONE;
}

/*!
 * @brief JSON Objectからid群をセットする
 * @param id_list_data id群情報の格納されたJSON Object
 * @param id_list id群情報を保管するvector
 * @return エラーコード
 */
static ParseErrorType set_id_list(const nlohmann::json &id_list_data, std::vector<int> &id_list)
{
    if (!id_list_data.is_array()) {
        return ParseErrorType::TOO_FEW_ARGUMENTS;
    }

    for (const auto &id_data : id_list_data) {
        if (!id_data.is_number()) {
            return ParseErrorType::INVALID_TYPE;
        }
        int id{};
        if (const auto err = info_set_integer(id_data, id, true, Range(1, 9999)); !err) {
            return err.error();
        }
        id_list.push_back(id);
    }
    return ParseErrorType::NONE;
}

/*!
 * @brief JSON Objectからモンスターのメッセージをセットする
 * @param message_data メッセージ情報の格納されたJSON Object
 * @return エラーコード
 * @details 選択言語が無く他言語だけがあるメッセージは読み飛ばし、後続のメッセージの解析を続ける。
 */
ParseErrorType MessageReader::set_mon_message() const
{
    const auto &group_data = this->message_data;
    const auto message_iter = group_data.find("message");
    if (message_iter == group_data.end() || !message_iter->is_array()) {
        return ParseErrorType::TOO_FEW_ARGUMENTS;
    }
    const auto &message_array = message_iter.value();
    auto id_list = std::vector<int>();

    const auto id_list_iter = group_data.find("id_list");
    const auto has_id_list = (id_list_iter != group_data.end());

    if (has_id_list) {
        ParseErrorType id_err = set_id_list(id_list_iter.value(), id_list);
        if (id_err != ParseErrorType::NONE) {
            return id_err;
        }
    } else {
        const auto &name_obj = get_json_value(group_data, "name");
        if (!name_obj.is_string()) {
            return ParseErrorType::TOO_FEW_ARGUMENTS;
        }

        const auto group_name = name_obj.get<std::string>();
        if (group_name != "DEFAULT") {
            return ParseErrorType::TOO_FEW_ARGUMENTS;
        }
    }

    if (!message_array.is_array()) {
        return ParseErrorType::TOO_FEW_ARGUMENTS;
    }

    for (const auto &message : message_array) {
        const auto action_iter = message.find("action");
        if (action_iter == message.end() || !action_iter->is_string()) {
            return ParseErrorType::TOO_FEW_ARGUMENTS;
        }
        const auto &action_data = action_iter.value();
        const auto action = r_info_message_flags.find(action_data.get<std::string>());
        if (action == r_info_message_flags.end()) {
            return ParseErrorType::INVALID_FLAG;
        }

        const auto chance_iter = message.find("chance");
        if (chance_iter == message.end() || !chance_iter->is_number()) {
            return ParseErrorType::TOO_FEW_ARGUMENTS;
        }
        const auto &chance_data = chance_iter.value();
        int chance{};
        if (const auto err = info_set_integer(chance_data, chance, true, Range(1, 100)); !err) {
            return err.error();
        }

        bool use_name = true;
        const auto use_name_iter = message.find("use_name");
        if (use_name_iter != message.end()) {
            const auto &use_name_data = use_name_iter.value();
            if (const auto err = info_set_bool(use_name_data, use_name, false); !err) {
                return err.error();
            }
        }

        const auto language_iter = message.find("message");
        if (language_iter == message.end() || !language_iter->is_object()) {
            return ParseErrorType::TOO_FEW_ARGUMENTS;
        }
        const auto &language_list = language_iter.value();

        const auto &ja_list = language_list.find("ja");
        const auto &en_list = language_list.find("en");
#ifdef JP
        if (ja_list == language_list.end()) {
            if (en_list == language_list.end()) {
                return ParseErrorType::TOO_FEW_ARGUMENTS;
            }
            continue;
        }
        const auto &message_list = ja_list.value();
#else
        if (en_list == language_list.end()) {
            if (ja_list == language_list.end()) {
                return ParseErrorType::TOO_FEW_ARGUMENTS;
            }
            continue;
        }
        const auto &message_list = en_list.value();
#endif

        for (const auto &message_str : message_list) {
            if (message_str.is_null()) {
                return ParseErrorType::TOO_FEW_ARGUMENTS;
            }
            if (!message_str.is_string()) {
                return ParseErrorType::INVALID_TYPE;
            }
#ifdef JP
            auto str_test = utf8_to_sys(message_str.get<std::string>());
            if (!str_test) {
                return ParseErrorType::INVALID_VALUE;
            }
            auto str = std::move(*str_test);
#else
            auto str = message_str.get<std::string>();
#endif
            if (has_id_list) {
                for (auto id : id_list) {
                    MonraceMessageList::get_instance().emplace(id, action->second, chance, use_name, str);
                }
            } else {
                MonraceMessageList::get_instance().emplace_default(action->second, chance, use_name, str);
            }
        }
    }
    return ParseErrorType::NONE;
}
