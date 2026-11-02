#include "info-reader/activation-reader.h"
#include "artifact/random-art-effects.h"
#include "info-reader/parse-error-types.h"
#include "object-enchant/activation-info-table.h"
#include <nlohmann/json.hpp>

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(発動能力用) /
 * Grab one activation index flag
 * @details 発動効果テーブルに登録された名前だけを受け入れる。番号は効果を足すとずれるため、番号での指定は受け付けない。
 * @param what 発動の名前
 * @return 発動能力ID
 */
RandomArtActType grab_one_activation_flag(std::string_view what)
{
    for (const auto &activation : activation_info) {
        if (what == activation.flag) {
            return activation.index;
        }
    }

    // 不明な発動名は NONE を返して呼出側の各リーダーが INVALID_FLAG として扱う
    // (他のリーダー同様、個別メッセージは出さず読込エラーとして集約する)。
    return RandomArtActType::NONE;
}

/*!
 * @brief 任意の発動指定を読み込む (アーティファクト・ベースアイテム・エゴ共通)
 * @param activation_json 発動名の JSON 値。省略された項目は null として渡す
 * @return 省略・null 時は NONE、指定時は発動能力 ID、型不正は INVALID_TYPE、未知名は INVALID_FLAG
 * @details 3 リーダーで重複していた「null→省略 / 非文字列→型エラー / grab→未知名エラー」の
 *          判定を集約する。bakaba の grab_one_activation_flag は未知名でも個別メッセージを出さない
 *          集約方式のため、本関数も未知名時はメッセージ無しで INVALID_FLAG を返す (上流と異なる)。
 */
tl::expected<RandomArtActType, ParseErrorType> read_activation(const nlohmann::json &activation_json)
{
    if (activation_json.is_null()) {
        return RandomArtActType::NONE;
    }
    if (!activation_json.is_string()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }
    const auto activation = grab_one_activation_flag(activation_json.get<std::string>());
    if (activation == RandomArtActType::NONE) {
        return tl::make_unexpected(ParseErrorType::INVALID_FLAG);
    }
    return activation;
}
