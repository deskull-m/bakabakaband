/*!
 * @file weapon-shield.cpp
 * @brief 手装備持ち替え処理実装
 */

#include "action/weapon-shield.h"
#include "flavor/flavor-describer.h"
#include "game-option/birth-options.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "object-hook/hook-weapon.h"
#include "player-info/equipment-info.h"
#include "player-status/player-hand-types.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "view/display-messages.h"

/*!
 * @brief 両手持ちまたは片手持ちのメッセージを表示する
 * @param creature クリーチャーへの参照
 * @param item_name アイテム名
 * @param is_two_handed 両手持ちかどうか
 */
static void display_wielding_message(const std::string &item_name, bool is_two_handed)
{
    if (is_two_handed) {
        msg_format(_("%sを両手で構えた。", "You are wielding %s with both hands."), item_name.data());
        return;
    }

    const auto hand = left_hander ? _("左手", "left") : _("右手", "right");
    msg_format(_("%sを%sで構えた。", "You are wielding %s in your %s hand."), item_name.data(), hand);
}

/*!
 * @brief 利き手への移動処理
 * @param creature クリーチャーへの参照
 */
static void move_to_main_hand(CreatureEntity &creature)
{
    if (!has_melee_weapon(creature, INVEN_SUB_HAND)) {
        return;
    }

    const auto &item_sub_hand = *creature.inventory[INVEN_SUB_HAND];
    const auto item_name = describe_flavor(creature, item_sub_hand, 0);

    if (item_sub_hand.is_cursed()) {
        const auto is_two_handed = item_sub_hand.allow_two_hands_wielding() && can_two_hands_wielding(creature);
        if (is_two_handed) {
            msg_format(_("%sを両手で構えた。", "You are wielding %s with both hands."), item_name.data());
        }
        return;
    }

    auto &item_main_hand = *creature.inventory[INVEN_MAIN_HAND];
    item_main_hand = item_sub_hand.clone();
    inven_item_increase(creature, INVEN_SUB_HAND, -item_sub_hand.number);
    inven_item_optimize(creature, INVEN_SUB_HAND);

    const auto is_two_handed = item_main_hand.allow_two_hands_wielding() && can_two_hands_wielding(creature);
    display_wielding_message(item_name, is_two_handed);
}

/*!
 * @brief 逆手への移動処理
 * @param creature クリーチャーへの参照
 */
static void move_to_sub_hand(CreatureEntity &creature)
{
    const auto &item_main_hand = *creature.inventory[INVEN_MAIN_HAND];
    const auto item_name = item_main_hand.is_valid() ? describe_flavor(creature, item_main_hand, 0) : "";

    if (has_melee_weapon(creature, INVEN_MAIN_HAND)) {
        const auto is_two_handed = item_main_hand.allow_two_hands_wielding() && can_two_hands_wielding(creature);
        if (is_two_handed) {
            msg_format(_("%sを両手で構えた。", "You are wielding %s with both hands."), item_name.data());
        }
        return;
    }

    if ((empty_hands(creature, false) & EMPTY_HAND_MAIN) || item_main_hand.is_cursed()) {
        return;
    }

    auto &item_sub_hand = *creature.inventory[INVEN_SUB_HAND];
    item_sub_hand = item_main_hand.clone();
    inven_item_increase(creature, INVEN_MAIN_HAND, -item_main_hand.number);
    inven_item_optimize(creature, INVEN_MAIN_HAND);
    msg_format(_("%sを持ち替えた。", "You shifted %s to your other hand."), item_name.data());
}

/*!
 * @brief 持ち替え処理
 * @param creature クリーチャーへの参照
 * @param i_idx 持ち替えを行いたい装備部位ID
 */
void verify_equip_slot(CreatureEntity &creature, INVENTORY_IDX i_idx)
{
    if (i_idx == INVEN_MAIN_HAND) {
        move_to_main_hand(creature);
    } else if (i_idx == INVEN_SUB_HAND) {
        move_to_sub_hand(creature);
    }
}
