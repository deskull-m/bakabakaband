/*!
 * @brief アイテムが特定種別のものであるかどうかの判定関数群
 * @date 2018/12/15
 * @author deskull
 */

#include "object/object-kind-hook.h"
#include "object/tval-types.h"
#include "sv-definition/sv-amulet-types.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-ring-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include <algorithm>
#include <map>
#include <sstream>
#include <vector>

// @brief 3冊目の魔法書からは上質アイテムとして扱う.
static const int SV_BOOK_MIN_GOOD = 2;

/*!
 * @brief オブジェクトが薬かどうかを判定する /
 * @param bi_id 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトが薬ならばTRUEを返す
 */
bool kind_is_potion(short bi_id)
{
    return BaseitemList::get_instance().get_baseitem(bi_id).bi_key.tval() == ItemKindType::POTION;
}

bool kind_is_nasty(short bi_id)
{
    return BaseitemList::get_instance().get_baseitem(bi_id).flags.has(TR_NASTY);
}

bool kind_is_sushi(short bi_id)
{
    return BaseitemList::get_instance().get_baseitem(bi_id).flags.has(TR_SUSHI);
}

/*!
 * @brief ベースアイテムが上質として扱われるかどうかを返す。
 * Hack -- determine if a template is "good"
 * @param bi_id 判定したいベースアイテムのID
 * @return ベースアイテムが上質ならばTRUEを返す。
 */
bool kind_is_good(short bi_id)
{
    const auto &baseitem = BaseitemList::get_instance().get_baseitem(bi_id);
    switch (baseitem.bi_key.tval()) {
        /* Armor -- Good unless damaged */
    case ItemKindType::HARD_ARMOR:
    case ItemKindType::SOFT_ARMOR:
    case ItemKindType::DRAG_ARMOR:
    case ItemKindType::SHIELD:
    case ItemKindType::CLOAK:
    case ItemKindType::BOOTS:
    case ItemKindType::GLOVES:
    case ItemKindType::HELM:
    case ItemKindType::CROWN:
        return baseitem.to_a >= 0;

    /* Weapons -- Good unless damaged */
    case ItemKindType::BOW:
    case ItemKindType::SWORD:
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::DIGGING:
        return (baseitem.to_h >= 0) && (baseitem.to_d >= 0);

    /* Ammo -- Arrows/Bolts are good */
    case ItemKindType::BOLT:
    case ItemKindType::ARROW:
        return true;

    /* Books -- High level books are good (except Arcane books) */
    case ItemKindType::LIFE_BOOK:
    case ItemKindType::SORCERY_BOOK:
    case ItemKindType::NATURE_BOOK:
    case ItemKindType::CHAOS_BOOK:
    case ItemKindType::DEATH_BOOK:
    case ItemKindType::TRUMP_BOOK:
    case ItemKindType::CRAFT_BOOK:
    case ItemKindType::DEMON_BOOK:
    case ItemKindType::CRUSADE_BOOK:
    case ItemKindType::MUSIC_BOOK:
    case ItemKindType::HISSATSU_BOOK:
    case ItemKindType::HEX_BOOK:
        return baseitem.bi_key.sval() >= SV_BOOK_MIN_GOOD;

    /* Rings -- Rings of Speed are good */
    case ItemKindType::RING:
        return (baseitem.bi_key.sval() == SV_RING_SPEED) || (baseitem.bi_key.sval() == SV_RING_LORDLY);

    /* Amulets -- Amulets of the Magi and Resistance are good */
    case ItemKindType::AMULET:
        return (baseitem.bi_key.sval() == SV_AMULET_THE_MAGI) || (baseitem.bi_key.sval() == SV_AMULET_RESISTANCE);
    default:
        return false;
    }
}
