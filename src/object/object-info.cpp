/*!
 * @brief オブジェクトの実装 / Object code, part 1
 * @date 2014/01/10
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 *\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2014 Deskull rearranged comment for Doxygen.\n
 */

#include "object/object-info.h"
#include "inventory/inventory-slot-types.h"
#include "player-base/player-class.h"
#include "player/player-realm.h"
#include "sv-definition/sv-bow-types.h"
#include "sv-definition/sv-junk-types.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-ring-types.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/item-entity.h"
#include "system/monrace/extended-slot.h"
#include "util/int-char-converter.h"

namespace {

/*!
 * @brief 指定されたアイテム種別を受け入れる拡張スロットを探す
 * @param creature 対象クリーチャー
 * @param tval アイテム種別
 * @return 装備可能な拡張スロットの ID (INVEN_EXTENDED_BASE + idx)、
 *         該当なしなら -1
 * @details Phase 2.5: モンスターの体構造が持つ拡張スロットのうち、
 *          指定アイテム種別を受け入れ可能かつ空きのものを探す。
 */
short find_extended_slot(CreatureEntity &creature, ItemKindType tval)
{
    const auto count = creature.get_extended_slot_count();
    for (size_t i = 0; i < count; ++i) {
        const auto slot_type = creature.get_extended_slot_type(i);
        bool accepts = false;
        switch (slot_type) {
        case ExtendedSlotType::TAIL_RING:
            accepts = (tval == ItemKindType::RING);
            break;
        case ExtendedSlotType::SECOND_NECK:
            accepts = (tval == ItemKindType::AMULET || tval == ItemKindType::WHISTLE);
            break;
        case ExtendedSlotType::THIRD_HEAD:
            accepts = (tval == ItemKindType::CROWN || tval == ItemKindType::HELM);
            break;
        case ExtendedSlotType::WING_LEFT:
        case ExtendedSlotType::WING_RIGHT:
            // 翼装飾 (アミュレット相当の装飾) を受け入れる
            accepts = (tval == ItemKindType::AMULET);
            break;
        case ExtendedSlotType::MAX:
            break;
        }
        if (!accepts) {
            continue;
        }
        if (i >= creature.extended_inventory.size() || !creature.extended_inventory[i] || !creature.extended_inventory[i]->is_valid()) {
            return static_cast<short>(INVEN_EXTENDED_BASE + i);
        }
    }
    return -1;
}

}

/*!
 * @brief オブジェクトの発動効果名称を返す (メインルーチン)
 * @param o_ptr 名称を取得する元のオブジェクト構造体参照ポインタ
 * @return 発動名称
 */
std::string activation_explanation(const ItemEntity *o_ptr)
{
    const auto flags = o_ptr->get_flags();
    if (flags.has_not(TR_ACTIVATE)) {
        return _("なし", "nothing");
    }

    if (o_ptr->has_activation()) {
        return o_ptr->build_activation_description();
    }

    const auto tval = o_ptr->bi_key.tval();
    const auto sval = o_ptr->bi_key.sval();
    if (tval == ItemKindType::WHISTLE) {
        return _("ペット呼び寄せ : 100+d100ターン毎", "call pet every 100+d100 turns");
    }

    if (tval == ItemKindType::CAPTURE) {
        return _("モンスターを捕える、又は解放する。", "captures or releases a monster.");
    }

    if (tval == ItemKindType::BOW && sval == SV_FLAMETHROWER) {
        return _("火炎放射", "Flame throwing");
    }

    if (tval == ItemKindType::BOW && sval == SV_ROSMARINUS) {
        return _("神秘の霧", "Sacred mist");
    }

    if (tval == ItemKindType::JUNK && sval == SV_STUNGUN) {
        return _("電気ショック", "Electorical shock");
    }

    if (tval == ItemKindType::BOW && sval == SV_RAYGUN) {
        return _("はかいこうせん", "Blaster");
    }

    if (tval == ItemKindType::HELM && sval == SV_SCOUTER) {
        return _("調査", "Probing");
    }

    return _("何も起きない", "Nothing");
}

/*!
 * @brief オブジェクト選択時の選択アルファベットラベルを返す /
 * Convert an inventory index into a one character label
 * @param i プレイヤーの所持/装備オブジェクトID
 * @return 対応するアルファベット
 * @details Note that the label does NOT distinguish inven/equip.
 */
char index_to_label(int i)
{
    return i < INVEN_MAIN_HAND ? I2A(i) : I2A(i - INVEN_MAIN_HAND);
}

/*!
 * @brief オブジェクトの該当装備部位IDを返す /
 * Determine which equipment slot (if any) an item likes
 * @param item 名称を取得する元のオブジェクト構造体への参照
 * @return 対応する装備部位ID
 */
short wield_slot(CreatureEntity &creature, const ItemEntity &item)
{
    short desired = -1;
    switch (item.bi_key.tval()) {
    case ItemKindType::DIGGING:
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::SWORD:
        if (!creature.inventory[INVEN_MAIN_HAND]->bi_id) {
            desired = INVEN_MAIN_HAND;
        } else if (creature.inventory[INVEN_SUB_HAND]->bi_id) {
            desired = INVEN_MAIN_HAND;
        } else {
            desired = INVEN_SUB_HAND;
        }
        break;
    case ItemKindType::CAPTURE:
    case ItemKindType::CARD:
    case ItemKindType::SHIELD:
        if (!creature.inventory[INVEN_SUB_HAND]->bi_id) {
            desired = INVEN_SUB_HAND;
        } else if (creature.inventory[INVEN_MAIN_HAND]->bi_id) {
            desired = INVEN_SUB_HAND;
        } else {
            desired = INVEN_MAIN_HAND;
        }
        break;
    case ItemKindType::BOW:
        desired = INVEN_BOW;
        break;
    case ItemKindType::RING:
        desired = !creature.inventory[INVEN_MAIN_RING]->bi_id ? INVEN_MAIN_RING : INVEN_SUB_RING;
        break;
    case ItemKindType::AMULET:
    case ItemKindType::WHISTLE:
        desired = INVEN_NECK;
        break;
    case ItemKindType::ANAL_PLUG:
        desired = INVEN_ASSHOLE;
        break;
    case ItemKindType::LITE:
        desired = INVEN_LITE;
        break;
    case ItemKindType::DRAG_ARMOR:
    case ItemKindType::HARD_ARMOR:
    case ItemKindType::SOFT_ARMOR:
        desired = INVEN_BODY;
        break;
    case ItemKindType::CLOAK:
        desired = INVEN_OUTER;
        break;
    case ItemKindType::CROWN:
    case ItemKindType::HELM:
        desired = INVEN_HEAD;
        break;
    case ItemKindType::GLOVES:
        desired = INVEN_ARMS;
        break;
    case ItemKindType::BOOTS:
        desired = INVEN_FEET;
        break;
    default:
        return -1;
    }

    // [モンスター体構造] 体構造で装備不可なスロットなら、拡張スロットを試す。
    // それでもダメなら -1 (パック行き)。プレイヤーは常に can_equip_to=true。
    if (!creature.can_equip_to(desired)) {
        const auto extended = find_extended_slot(creature, item.bi_key.tval());
        return extended;
    }

    // [Phase 2.5] 通常スロットが既に埋まっている場合、拡張スロットを試す。
    // RING/AMULET 等で MAIN/SUB が両方埋まっているとき、TAIL_RING や
    // SECOND_NECK 等に自動装備する。
    if (creature.inventory[desired]->is_valid() && (item.bi_key.tval() == ItemKindType::RING || item.bi_key.tval() == ItemKindType::AMULET || item.bi_key.tval() == ItemKindType::WHISTLE)) {
        // RING の場合は SUB_RING も埋まっているかチェック
        if (item.bi_key.tval() == ItemKindType::RING) {
            if (creature.inventory[INVEN_MAIN_RING]->is_valid() && creature.inventory[INVEN_SUB_RING]->is_valid()) {
                const auto extended = find_extended_slot(creature, item.bi_key.tval());
                if (extended >= INVEN_EXTENDED_BASE) {
                    return extended;
                }
            }
        } else {
            // AMULET/WHISTLE は NECK が埋まっていれば拡張を試す
            if (creature.inventory[INVEN_NECK]->is_valid()) {
                const auto extended = find_extended_slot(creature, item.bi_key.tval());
                if (extended >= INVEN_EXTENDED_BASE) {
                    return extended;
                }
            }
        }
    }
    return desired;
}

/*!
 * @brief tval/sval指定のベースアイテムがプレイヤーの使用可能な魔法書かどうかを返す
 * @param creature クリーチャーへの参照
 * @param bi_key ベースアイテム特定キー
 * @return 使用可能な魔法書ならばTRUEを返す。
 */
bool check_book_realm(CreatureEntity &creature, const BaseitemKey &bi_key)
{
    if (!bi_key.is_spell_book()) {
        return false;
    }

    const auto tval = bi_key.tval();
    const auto book_realm = PlayerRealm::get_realm_of_book(tval);
    CreatureClass pc(creature);
    if (pc.equals(PlayerClassType::SORCERER)) {
        return PlayerRealm::is_magic(book_realm);
    } else if (pc.equals(PlayerClassType::RED_MAGE)) {
        if (!PlayerRealm::is_magic(book_realm)) {
            return false;
        }
        return ((book_realm == RealmType::ARCANE) || (bi_key.sval() < 2));
    }

    PlayerRealm pr(creature);
    return pr.realm1().equals(book_realm) || pr.realm2().equals(book_realm);
}

/*!
 * @brief 所持品IDからアイテムを返す
 * @param creature クリーチャーへの参照
 * @param i_idx 所持品ID
 * @return 対応するアイテムへの shared_ptr
 * @details 返り値をshared_ptrとして保持せず".get()"で取得した生ポインタや"*"で取得した参照を保持し続けてはいけない.
 * アイテムがインベントリや床から削除された際にダングリングポインタになる可能性がある.
 */
std::shared_ptr<ItemEntity> ref_item(CreatureEntity &creature, short i_idx)
{
    auto &floor = *creature.get_floor();
    return i_idx >= 0 ? creature.inventory[i_idx] : floor.o_list[0 - i_idx];
}
