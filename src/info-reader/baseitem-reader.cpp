/*
 * @brief ベースアイテム定義の読み込み処理
 * @author Hourier
 * @date 2022/10/10
 */

#include "info-reader/baseitem-reader.h"
#include "alliance/alliance.h"
#include "artifact/random-art-effects.h"
#include "info-reader/baseitem-tokens-table.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/symbol-reader-util.h"
#include "locale/japanese.h"
#include "object-enchant/tr-types.h"
#include "object/tval-types.h"
#include "player/player-skill.h"
#include "sv-definition/sv-rod-types.h"
#include "sv-definition/sv-staff-types.h"
#include "sv-definition/sv-wand-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-key.h"
#include "system/baseitem/baseitem-list.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include "view/display-messages.h"
#include <array>
#include <iterator>
#include <limits>
#include <nlohmann/json.hpp>
#include <tuple>
#include <utility>

// fake_spell_flags は呪文のビット表 (cmd-spell.h) だが、ヘッダ全体を取り込むと循環依存になりうるため extern 宣言のみ参照する。
extern const uint32_t fake_spell_flags[4];

namespace {
// 金貨は cost が生成する額 (基本価格の 3〜9 倍) の元になるため、十分小さく保つ。
constexpr auto GOLD_COST_MAX = 1024;
}

int item_subtype_max(const BaseitemKey &key)
{
    if (key.is_weapon()) {
        // 武器熟練度の種別ごとの配列を副種別で引く。
        return static_cast<int>(std::tuple_size_v<decltype(skill_table::w_max)::mapped_type>) - 1;
    }
    if (key.is_spell_book()) {
        // 魔法書の副種別で呪文のビット表を引く。
        return static_cast<int>(std::size(fake_spell_flags)) - 1;
    }
    // 魔道具術師の取り込み先の要素数に合わせる。
    switch (key.tval()) {
    case ItemKindType::STAFF:
        return SV_STAFF_MAX - 1;
    case ItemKindType::WAND:
        return SV_WAND_MAX - 1;
    case ItemKindType::ROD:
        return SV_ROD_MAX - 1;
    default:
        return ITEM_SUBTYPE_DEFAULT_MAX;
    }
}

BaseitemReader::BaseitemReader(const nlohmann::json &baseitem_data, int &previous_id)
    : baseitem_data(baseitem_data)
    , previous_id(previous_id)
{
}

/*!
 * @brief ベースアイテム情報(JSON Object)のパース関数
 * @note BaseitemReader が保持する baseitem_data を参照する
 * @return エラーコード
 */
ParseErrorType BaseitemReader::read() const
{
    if (!this->baseitem_data.is_object()) {
        return ParseErrorType::INVALID_TYPE;
    }

    const auto &id_data = get_json_value(this->baseitem_data, "id");
    if (id_data.is_null()) {
        msg_print(_("ベースアイテムIDの読込失敗。", "Failed to load baseitem ID."));
        return ParseErrorType::TOO_FEW_ARGUMENTS;
    }
    if (!id_data.is_number_integer()) {
        msg_print(_("ベースアイテムIDの読込失敗。", "Failed to load baseitem ID."));
        return ParseErrorType::INVALID_TYPE;
    }

    const auto validate_id = [this](const auto id) -> ParseErrorType {
        if (std::cmp_less_equal(id, this->previous_id)) {
            msg_print(_("ベースアイテムIDが非連番です。ID: '{}'、直前ID: '{}'。", "Baseitem ID is not sequential. ID: '{}', previous ID: '{}'."), id, this->previous_id);
            return ParseErrorType::NON_SEQUENTIAL_RECORDS;
        }
        if (std::cmp_less(id, 0) || std::cmp_greater(id, std::numeric_limits<short>::max())) {
            msg_print(_("ベースアイテムIDが範囲外です。ID: '{}'。", "Baseitem ID is out of bounds. ID: '{}'."), id);
            return ParseErrorType::INVALID_VALUE;
        }
        return ParseErrorType::NONE;
    };
    const auto id_error = id_data.is_number_unsigned() ? validate_id(id_data.get<nlohmann::json::number_unsigned_t>()) : validate_id(id_data.get<nlohmann::json::number_integer_t>());
    if (id_error != ParseErrorType::NONE) {
        return id_error;
    }
    const auto item_id = id_data.get<int>();

    auto &baseitems = BaseitemList::get_instance();
    this->previous_id = item_id;
    const short short_id = static_cast<short>(item_id);
    // ベースアイテムをローカルに組み立て、全フィールドの読込に成功したら replace_baseitem() で
    // 差し替える。これにより読込途中のエラーでも格納済みの定義が壊れず、name/bi_key 変更時に
    // キャッシュも無効化される。既存IDの再読込では省略可能項目を引き継ぐため複製から始める。
    auto baseitem = item_id < static_cast<int>(baseitems.size()) ? baseitems.get_baseitem(short_id).clone_without_required_fields() : BaseitemDefinition{};
    if (const auto err = info_set_string(get_json_value(this->baseitem_data, "name"), baseitem.name, true); !err) {
        msg_print(_("アイテムの名称読込失敗。ID: '{}'。", "Failed to load item name. ID: '{}'."), item_id);
        return err.error();
    }
    if (const auto err = info_set_string(get_json_value(this->baseitem_data, "flavor_name"), baseitem.flavor_name, false); !err) {
        msg_print(_("アイテム未識別名の読込失敗。ID: '{}'。", "Failed to load item unidentified name. ID: '{}'."), item_id);
        return err.error();
    }
    if (const auto err = info_set_string(get_json_value(this->baseitem_data, "flavor"), baseitem.text, false); !err) {
        msg_print(_("アイテムのフレーバーテキスト読込失敗。ID: '{}'。", "Failed to load flavor text of item. ID: '{}'."), item_id);
        return err.error();
    }
    if (const auto err = this->set_baseitem_symbol(baseitem); err != ParseErrorType::NONE) {
        msg_print(_("アイテムのシンボル読込失敗。ID: '{}'。", "Failed to load symbol of item. ID: '{}'."), item_id);
        return err;
    }
    if (const auto err = this->set_baseitem_kind(baseitem); err != ParseErrorType::NONE) {
        msg_print(_("アイテム種別の読込失敗。ID: '{}'。", "Failed to load kind of item. ID: '{}'."), item_id);
        return err;
    }
    if (const auto err = this->set_baseitem_parameter_value(baseitem); err != ParseErrorType::NONE) {
        msg_print(_("アイテムのパラメータ値読込失敗。ID: '{}'。", "Failed to load parameter value of item. ID: '{}'."), item_id);
        return err;
    }
    if (const auto err = info_set_integer(get_json_value(this->baseitem_data, "level"), baseitem.level, true, Range(0, 128)); !err) {
        msg_print(_("アイテムのレベル読込失敗。ID: '{}'。", "Failed to load level of item. ID: '{}'."), item_id);
        return err.error();
    }
    if (const auto err = info_set_integer(get_json_value(this->baseitem_data, "weight"), baseitem.weight, true, Range(0, 9999)); !err) {
        msg_print(_("アイテムの重量読込失敗。ID: '{}'。", "Failed to load weight of item. ID: '{}'."), item_id);
        return err.error();
    }
    const auto cost_max = baseitem.bi_key.tval() == ItemKindType::GOLD ? GOLD_COST_MAX : ITEM_COST_MAX;
    if (const auto err = info_set_integer(get_json_value(this->baseitem_data, "cost"), baseitem.cost, true, Range(0, cost_max)); !err) {
        msg_print(_("アイテムの売値読込失敗。ID: '{}'。", "Failed to load cost of item. ID: '{}'."), item_id);
        return err.error();
    }
    if (const auto err = info_set_integer(get_json_value(this->baseitem_data, "base_ac"), baseitem.ac, false, Range(-99, 99)); !err) {
        msg_print(_("アイテムのベースAC読込失敗。ID: '{}'。", "Failed to load base AC of item. ID: '{}'."), item_id);
        return err.error();
    }
    if (const auto err = info_set_dice(get_json_value(this->baseitem_data, "base_dice"), baseitem.damage_dice, false); !err) {
        msg_print(_("アイテムのベースダイス読込失敗。ID: '{}'。", "Failed to load base dice of item. ID: '{}'."), item_id);
        return err.error();
    }
    if (const auto err = info_set_integer(get_json_value(this->baseitem_data, "hit_bonus"), baseitem.to_h, false, Range(-99, 99)); !err) {
        msg_print(_("アイテムの命中補正値読込失敗。ID: '{}'。", "Failed to load hit bonus of item. ID: '{}'."), item_id);
        return err.error();
    }
    if (const auto err = info_set_integer(get_json_value(this->baseitem_data, "damage_bonus"), baseitem.to_d, false, Range(-99, 99)); !err) {
        msg_print(_("アイテムのダメージ補正値読込失敗。ID: '{}'。", "Failed to load damage bonus of item. ID: '{}'."), item_id);
        return err.error();
    }
    if (const auto err = info_set_integer(get_json_value(this->baseitem_data, "ac_bonus"), baseitem.to_a, false, Range(-99, 99)); !err) {
        msg_print(_("アイテムのAC補正値読込失敗。ID: '{}'。", "Failed to load AC bonus of item. ID: '{}'."), item_id);
        return err.error();
    }
    if (const auto err = this->set_baseitem_allocations(baseitem); err != ParseErrorType::NONE) {
        msg_print(_("アイテムの生成情報読込失敗。ID: '{}'。", "Failed to load generation info of item. ID: '{}'."), item_id);
        return err;
    }
    if (const auto err = this->set_baseitem_alliance(baseitem); err != ParseErrorType::NONE) {
        msg_print(_("アイテムのアライアンス情報読込失敗。ID: '{}'。", "Failed to load alliance of item. ID: '{}'."), item_id);
        return err;
    }
    if (const auto err = this->set_baseitem_activate(baseitem); err != ParseErrorType::NONE) {
        msg_print(_("アイテムの発動能力読込失敗。ID: '{}'。", "Failed to load activation of item. ID: '{}'."), item_id);
        return err;
    }
    if (const auto err = this->set_baseitem_flags(baseitem); err != ParseErrorType::NONE) {
        msg_print(_("アイテムのフラグ読込失敗。ID: '{}'。", "Failed to load flags of item. ID: '{}'."), item_id);
        return err;
    }

    if (item_id >= static_cast<int>(baseitems.size())) {
        baseitems.resize(item_id + 1);
    }
    baseitems.replace_baseitem(short_id, std::move(baseitem));
    return ParseErrorType::NONE;
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(ベースアイテム用)
 * @param baseitem 保管先ベースアイテムへの参照
 * @param what 参照元の文字列
 * @return 見つけたらtrue
 */
bool BaseitemReader::grab_one_baseitem_flag(BaseitemDefinition &baseitem, std::string_view what) const
{
    if (TrFlags::grab_one_flag(baseitem.flags, baseitem_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<ItemGenerationTraitType>::grab_one_flag(baseitem.gen_flags, baseitem_geneneration_flags, what)) {
        return true;
    }

    msg_print(_("未知のアイテム・フラグ '{}'。", "Unknown object flag '{}'."), what);
    return false;
}

/*!
 * @brief JSON Objectからベースアイテムのシンボルをセットする
 * @note BaseitemReader が保持する baseitem_data を参照する
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return エラーコード
 */
ParseErrorType BaseitemReader::set_baseitem_symbol(BaseitemDefinition &baseitem) const
{
    const auto symbol = info_read_symbol(get_json_value(this->baseitem_data, "symbol"));
    if (!symbol) {
        return symbol.error();
    }

    baseitem.init_symbol(*symbol);
    return ParseErrorType::NONE;
}

/*!
 * @brief JSON Objectからベースアイテム種別をセットする
 * @note BaseitemReader が保持する baseitem_data を参照する
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return エラーコード
 */
ParseErrorType BaseitemReader::set_baseitem_kind(BaseitemDefinition &baseitem) const
{
    const auto &itemkind_data = get_json_value(this->baseitem_data, "itemkind");
    if (itemkind_data.is_null()) {
        return ParseErrorType::TOO_FEW_ARGUMENTS;
    }
    if (!itemkind_data.is_object()) {
        return ParseErrorType::INVALID_TYPE;
    }

    ItemKindType type_value{};
    if (const auto err = info_set_integer(get_json_value(itemkind_data, "type_value"), type_value, true, Range(0, 128)); !err) {
        return err.error();
    }
    int subtype_value{};
    if (const auto err = info_set_integer(get_json_value(itemkind_data, "subtype_value"), subtype_value, true, Range(0, 128)); !err) {
        return err.error();
    }

    baseitem.bi_key = { type_value, subtype_value };
    return ParseErrorType::NONE;
}

/*!
 * @brief JSON Objectからベースアイテムのpvalをセットする
 * @note BaseitemReader が保持する baseitem_data を参照する
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return エラーコード
 */
ParseErrorType BaseitemReader::set_baseitem_parameter_value(BaseitemDefinition &baseitem) const
{
    const auto &pval_data = get_json_value(this->baseitem_data, "parameter_value");
    if (const auto err = info_set_integer(pval_data, baseitem.pval, false, Range(-9999, 9999)); !err) {
        return err.error();
    }
    if ((baseitem.bi_key.tval() == ItemKindType::ROD) && (baseitem.pval <= 0)) {
        return ParseErrorType::INVALID_PVAL;
    }

    return ParseErrorType::NONE;
}

/*!
 * @brief JSON Objectからアイテムの階層/希少度情報をセットする
 * @note BaseitemReader が保持する baseitem_data を参照する
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return エラーコード
 */
ParseErrorType BaseitemReader::set_baseitem_allocations(BaseitemDefinition &baseitem) const
{
    const auto &allocations_data = get_json_value(this->baseitem_data, "allocations");
    if (allocations_data.is_null()) {
        return ParseErrorType::NONE;
    }
    if (!allocations_data.is_array()) {
        return ParseErrorType::INVALID_TYPE;
    }

    for (auto i = 0U; const auto &alloc : allocations_data) {
        if (i >= baseitem.alloc_tables.size()) {
            return ParseErrorType::INVALID_VALUE;
        }

        if (!alloc.is_object()) {
            return ParseErrorType::INVALID_TYPE;
        }

        auto &table = baseitem.alloc_tables[i];
        if (const auto err = info_set_integer(get_json_value(alloc, "depth"), table.level, true, Range(0, 128)); !err) {
            return err.error();
        }
        if (const auto err = info_set_integer(get_json_value(alloc, "rarity"), table.chance, true, Range(0, 256)); !err) {
            return err.error();
        }
        i++;
    }
    return ParseErrorType::NONE;
}

/*!
 * @brief JSON Objectからベースアイテムの発動能力をセットする
 * @note BaseitemReader が保持する baseitem_data を参照する
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return エラーコード
 */
ParseErrorType BaseitemReader::set_baseitem_activate(BaseitemDefinition &baseitem) const
{
    const auto activation = read_activation(get_json_value(this->baseitem_data, "activate"));
    if (!activation) {
        return activation.error();
    }
    if (*activation == RandomArtActType::NONE) {
        return ParseErrorType::NONE;
    }

    baseitem.act_idx = *activation;
    baseitem.flags.set(tr_type::TR_ACTIVATE);
    return ParseErrorType::NONE;
}

/*!
 * @brief JSON Objectからアライアンス情報をセットする
 * @note BaseitemReader が保持する baseitem_data を参照する
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return エラーコード
 */
ParseErrorType BaseitemReader::set_baseitem_alliance(BaseitemDefinition &baseitem) const
{
    const auto &alliance_data = get_json_value(this->baseitem_data, "alliance");
    if (alliance_data.is_null()) {
        baseitem.alliance_idx = AllianceType::NONE;
        return ParseErrorType::NONE;
    }
    if (!alliance_data.is_string()) {
        return ParseErrorType::INVALID_TYPE;
    }

    const auto alliance_tag = alliance_data.get<std::string>();
    bool found = false;
    for (const auto &[type, alliance_ptr] : alliance_list) {
        if (alliance_ptr->tag == alliance_tag) {
            baseitem.alliance_idx = type;
            found = true;
            break;
        }
    }

    if (!found) {
        msg_print(_("未知のアライアンスタグ '{}'。", "Unknown alliance tag '{}'."), alliance_tag);
        return ParseErrorType::INVALID_FLAG;
    }

    return ParseErrorType::NONE;
}

/*!
 * @brief JSON Objectからベースアイテムフラグをセットする
 * @note BaseitemReader が保持する baseitem_data を参照する
 * @param baseitem 保管先のベースアイテム情報インスタンス
 * @return エラーコード
 */
ParseErrorType BaseitemReader::set_baseitem_flags(BaseitemDefinition &baseitem) const
{
    const auto &flag_data = get_json_value(this->baseitem_data, "flags");
    if (flag_data.is_null()) {
        return ParseErrorType::NONE;
    }
    if (!flag_data.is_array()) {
        return ParseErrorType::INVALID_TYPE;
    }

    for (auto &flag : flag_data) {
        if (!flag.is_string()) {
            return ParseErrorType::INVALID_TYPE;
        }
        if (!this->grab_one_baseitem_flag(baseitem, flag.get<std::string>())) {
            return ParseErrorType::INVALID_FLAG;
        }
    }
    return ParseErrorType::NONE;
}
