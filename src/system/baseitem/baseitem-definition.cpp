/*!
 * @brief ベースアイテム定義の実装
 * @date 2024/11/16
 * @author deskull, Hourier
 */

#include "system/baseitem/baseitem-definition.h"
#include "alliance/alliance.h"
#include "locale/language-switcher.h"
#include "util/string-processor.h"

BaseitemDefinition::BaseitemDefinition()
    : bi_key(ItemKindType::NONE)
    , alliance_idx(AllianceType::NONE)
    , symbol_definition(DisplaySymbol(0, '\0'))
{
}

/*!
 * @brief 必須項目 (name / bi_key / symbol / level / weight / cost) を除いた複製を返す
 * @return 省略可能な項目だけを引き継いだベースアイテム定義
 * @details 同一IDを再読込する際、JSONC で省略された省略可能項目を元の定義から保つために使う。
 * 必須項目は再読込で必ず指定されるため複製しない。alliance_idx は bakabakaband 独自の省略可能項目。
 */
BaseitemDefinition BaseitemDefinition::clone_without_required_fields() const
{
    BaseitemDefinition cloned;
    cloned.flavor_name = this->flavor_name;
    cloned.text = this->text;
    cloned.pval = this->pval;
    cloned.ac = this->ac;
    cloned.damage_dice = this->damage_dice;
    cloned.to_h = this->to_h;
    cloned.to_d = this->to_d;
    cloned.to_a = this->to_a;
    cloned.alloc_tables = this->alloc_tables;
    cloned.act_idx = this->act_idx;
    cloned.flags = this->flags;
    cloned.gen_flags = this->gen_flags;
    cloned.easy_know = this->easy_know;
    cloned.alliance_idx = this->alliance_idx;
    return cloned;
}

/*!
 * @brief 正常なベースアイテムかを判定する
 * @return 正常なベースアイテムか否か
 * @details ID 0は「異常アイテム」という名前の文字通り異常アイテムであり、個別に弾く
 * その他、ベースアイテムIDは歴史的事情により歯抜けが多数あり、それらは名前が空欄になるようにオブジェクトを生成している
 * @todo v3.1以降で歯抜けを埋めるようにベースアイテムを追加していきたい (詳細未定)
 */
bool BaseitemDefinition::is_valid() const
{
    return !this->name.empty() && (this->name != _("異常アイテム", "Invalid item"));
}

/*!
 * @brief ベースアイテム名を返す
 * @return ベースアイテム名
 */
std::string BaseitemDefinition::stripped_name() const
{
    const auto tokens = str_split(this->name, ' ');
    std::stringstream ss;
    for (const auto &token : tokens) {
        if (token == "" || token == "~" || token == "&" || token == "#") {
            continue;
        }

        auto offset = 0;
        auto endpos = token.size();
        auto is_kanji = false;
        if (token[0] == '~' || token[0] == '#') {
            offset++;
        }
#ifdef JP
        if (token.size() > 2) {
            is_kanji = iskanji(token[endpos - 2]);
        }

#endif
        if (!is_kanji && (token[endpos - 1] == '~' || token[endpos - 1] == '#')) {
            endpos--;
        }

        ss << token.substr(offset, endpos);
    }

    ss << " ";
    return ss.str();
}

bool BaseitemDefinition::order_cost(const BaseitemDefinition &other) const
{
    return this->cost < other.cost;
}

/*!
 * @brief 最初から簡易な名称が明らかなベースアイテムにその旨のフラグを立てる
 */
void BaseitemDefinition::decide_easy_know()
{
    switch (this->bi_key.tval()) {
    case ItemKindType::LIFE_BOOK:
    case ItemKindType::SORCERY_BOOK:
    case ItemKindType::NATURE_BOOK:
    case ItemKindType::CHAOS_BOOK:
    case ItemKindType::DEATH_BOOK:
    case ItemKindType::TRUMP_BOOK:
    case ItemKindType::ARCANE_BOOK:
    case ItemKindType::CRAFT_BOOK:
    case ItemKindType::DEMON_BOOK:
    case ItemKindType::CRUSADE_BOOK:
    case ItemKindType::MUSIC_BOOK:
    case ItemKindType::HISSATSU_BOOK:
    case ItemKindType::HEX_BOOK:
    case ItemKindType::FLASK:
    case ItemKindType::JUNK:
    case ItemKindType::BOTTLE:
    case ItemKindType::FLAVOR_SKELETON:
    case ItemKindType::SPIKE:
    case ItemKindType::WHISTLE:
    case ItemKindType::FOOD:
    case ItemKindType::POTION:
    case ItemKindType::SCROLL:
    case ItemKindType::ROD:
    case ItemKindType::STATUE:
    case ItemKindType::READING_MATTER:
        this->easy_know = true;
        return;
    default:
        this->easy_know = false;
        return;
    }
}

const DisplaySymbol &BaseitemDefinition::get_symbol() const
{
    return this->symbol_definition;
}

void BaseitemDefinition::init_symbol(const DisplaySymbol &ds)
{
    this->symbol_definition = ds;
}

void BaseitemDefinition::init_color(uint8_t color)
{
    this->symbol_definition.color = color;
}

void BaseitemDefinition::init_character(char character)
{
    this->symbol_definition.character = character;
}
