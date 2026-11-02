#include "info-reader/json-reader-util.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/symbol-reader-util.h"
#include "locale/character-encoding.h"
#include "locale/language-switcher.h"
#include "term/gameterm.h"
#include "util/dice.h"

/*!
 * @brief 定義ファイルのルートとレコード配列を、入力を変更せず検証する。
 * @param root JSON文書のルート
 * @param key 必須のレコード配列キー
 * @param allow_empty 空配列を許容するか（Vaultではfalse）
 * @return エラーコード
 */
tl::expected<void, ParseErrorType> info_validate_json_array(const nlohmann::json &root, std::string_view key, bool allow_empty)
{
    if (!root.is_object()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }
    const auto records = root.find(key);
    if (records == root.end()) {
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }
    if (!records->is_array()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }
    if (!allow_empty && records->empty()) {
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }
    return {};
}

const nlohmann::json &get_json_value(const nlohmann::json &json, std::string_view key)
{
    static const nlohmann::json null_json;
    if (!json.is_object()) {
        return null_json;
    }

    const auto it = json.find(key);
    return it != json.end() ? *it : null_json;
}

/*!
 * @brief JSON Objectから文字列をセットする
 *
 * 引数で与えられたJSON Objectから日本語版の場合は"ja"、英語版の場合は"en"のキーで文字列を取得し、
 * data に格納する。キーが存在しない場合は is_required が真の場合はエラーを返し、偽の場合は何もせずに終了する。
 *
 * @param json 文字列の格納されたJSON Object
 * @param data 文字列を格納する変数への参照
 * @param is_required 必須かどうか
 * @return エラーコード
 */
tl::expected<void, ParseErrorType> info_set_string(const nlohmann::json &json, std::string &data, bool is_required)
{
    const auto unexist_result = [is_required]() -> tl::expected<void, ParseErrorType> {
        if (is_required) {
            return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
        }
        return {};
    };

    if (json.is_null()) {
        return unexist_result();
    }

    if (!json.is_object()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }

    const auto str = json.find(_("ja", "en"));
    if (str == json.end()) {
        return unexist_result();
    }
    if (!str->is_string()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }

#ifdef JP
    auto str_sys = utf8_to_sys(str->get<std::string>());
    if (!str_sys) {
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }
    data = std::move(*str_sys);
#else
    data = str->get<std::string>();
#endif

    return {};
}

/*!
 * @brief JSON Objectからダイスの値を取得する
 * @param json ダイスの値が格納されたJSON Object
 * @param dice ダイスの値を格納する変数への参照
 * @param is_required 必須かどうか
 * 必須でJSON Objectがnullや文字列でない場合はエラーを返す。必須でない場合は何もせずに終了する。
 * @return エラーコード
 */
tl::expected<void, ParseErrorType> info_set_dice(const nlohmann::json &json, Dice &dice, bool is_required)
{
    if (json.is_null()) {
        if (is_required) {
            return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
        }
        return {};
    }
    if (!json.is_string()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }

    try {
        dice = Dice::parse(json.get<std::string>());
        return {};
    } catch (const std::runtime_error &) {
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }
}

/*!
 * @brief JSON Objectからbool値を取得する
 * @param json bool値が格納されたJSON Object
 * @param bool_value bool値を格納する変数への参照
 * @param is_required 必須かどうか
 * null は、必須なら ParseErrorType::TOO_FEW_ARGUMENTS を返し、任意なら何もせずに成功する。
 * null 以外の非bool値は ParseErrorType::INVALID_TYPE を返す。bool値のときのみ bool_value を変更する。
 * @return 成功時は空の値、失敗時は解析エラーコード
 */
tl::expected<void, ParseErrorType> info_set_bool(const nlohmann::json &json, bool &bool_value, bool is_required)
{
    if (json.is_null()) {
        if (is_required) {
            return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
        }
        return {};
    }

    if (!json.is_boolean()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }

    bool_value = json.get<bool>();
    return {};
}

/*!
 * @brief 表示記号の文字と色を読み取り、既存リーダーの検証順序を保つ。
 * @param json character と color を持つオブジェクト
 * @param policy 通常は型・文字数・色の順。モンスターは型不一致も欠落として扱い、色を文字数より先に検証する。
 * @return 表示記号、または欠落・型・文字数・未知の色に対応する解析エラー
 * @details 文字は1バイトなら制御文字や非ASCIIも許容する。地形固有の lit とメッセージは呼出側で扱う。
 */
tl::expected<DisplaySymbol, ParseErrorType> info_read_symbol(const nlohmann::json &json, SymbolReadPolicy policy)
{
    const auto monster = policy == SymbolReadPolicy::MONSTER_COLOR_FIRST;
    if (json.is_null()) {
        return tl::unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }
    if (!json.is_object()) {
        return tl::unexpected(monster ? ParseErrorType::TOO_FEW_ARGUMENTS : ParseErrorType::INVALID_TYPE);
    }

    const auto &character_obj = get_json_value(json, "character");
    if (!character_obj.is_string()) {
        return tl::unexpected(monster || character_obj.is_null() ? ParseErrorType::TOO_FEW_ARGUMENTS : ParseErrorType::INVALID_TYPE);
    }
    const auto &character = character_obj.get_ref<const std::string &>();
    if (!monster && character.size() != 1) {
        return tl::unexpected(ParseErrorType::INVALID_VALUE);
    }

    const auto &color_obj = get_json_value(json, "color");
    if (!color_obj.is_string()) {
        return tl::unexpected(monster || color_obj.is_null() ? ParseErrorType::TOO_FEW_ARGUMENTS : ParseErrorType::INVALID_TYPE);
    }
    // 文字列であることは確かめたので、ここで返るエラーは未知の色の INVALID_FLAG だけになる
    const auto color = info_read_token(color_obj, color_list);
    if (!color) {
        return tl::unexpected(color.error());
    }
    if (character.size() != 1) {
        return tl::unexpected(ParseErrorType::INVALID_VALUE);
    }

    return DisplaySymbol(*color, character.front());
}
