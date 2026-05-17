#include "player-info/equipment-info.h"
#include "inventory/inventory-slot-types.h"
#include "object-hook/hook-weapon.h"
#include "object/tval-types.h"
#include "pet/pet-util.h"
#include "player-base/player-class.h"
#include "player-status/player-hand-types.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief プレイヤーが現在右手/左手に武器を持っているか判定する /
 * @param creature クリーチャーへの参照
 * @param slot 判定する手のID(右手:INVEN_MAIN_HAND 左手:INVEN_SUB_HAND)
 * @return 持っているならばTRUE
 */
bool has_melee_weapon(CreatureEntity &creature, int slot)
{
    const auto o_ptr = creature.inventory[slot].get();
    return o_ptr->is_valid() && o_ptr->is_melee_weapon();
}

/*!
 * @brief プレイヤーの現在開いている手の状態を返す
 * @param creature クリーチャーへの参照
 * @param riding_control 乗馬中により片手を必要としている状態ならばTRUEを返す。
 * @return 開いている手のビットフラグ
 */
BIT_FLAGS16 empty_hands(CreatureEntity &creature, bool riding_control)
{
    BIT_FLAGS16 status = EMPTY_HAND_NONE;
    if (!creature.inventory[INVEN_MAIN_HAND]->is_valid()) {
        status |= EMPTY_HAND_MAIN;
    }
    if (!creature.inventory[INVEN_SUB_HAND]->is_valid()) {
        status |= EMPTY_HAND_SUB;
    }

    if (riding_control && (status != EMPTY_HAND_NONE) && creature.get_riding() && none_bits(creature.pet_extra_flags, PF_TWO_HANDS)) {
        if (any_bits(status, EMPTY_HAND_SUB)) {
            reset_bits(status, EMPTY_HAND_SUB);
        } else if (any_bits(status, EMPTY_HAND_MAIN)) {
            reset_bits(status, EMPTY_HAND_MAIN);
        }
    }

    return status;
}

bool can_two_hands_wielding(CreatureEntity &creature)
{
    return !creature.get_riding() || any_bits(creature.pet_extra_flags, PF_TWO_HANDS);
}

/*!
 * @brief プレイヤーが防具重量制限のある職業時にペナルティを受ける状態にあるかどうかを返す。
 * @param creature クリーチャーへの参照
 * @return ペナルティが適用されるならばTRUE。
 */
bool heavy_armor(CreatureEntity &creature)
{
    CreatureClass pc(creature);
    if (!pc.is_martial_arts_pro() && !pc.equals(PlayerClassType::NINJA)) {
        return false;
    }

    WEIGHT monk_arm_wgt = 0;
    if (creature.inventory[INVEN_MAIN_HAND]->bi_key.tval() > ItemKindType::SWORD) {
        monk_arm_wgt += creature.inventory[INVEN_MAIN_HAND]->weight;
    }

    if (creature.inventory[INVEN_SUB_HAND]->bi_key.tval() > ItemKindType::SWORD) {
        monk_arm_wgt += creature.inventory[INVEN_SUB_HAND]->weight;
    }

    monk_arm_wgt += creature.inventory[INVEN_BODY]->weight;
    monk_arm_wgt += creature.inventory[INVEN_HEAD]->weight;
    monk_arm_wgt += creature.inventory[INVEN_OUTER]->weight;
    monk_arm_wgt += creature.inventory[INVEN_ARMS]->weight;
    monk_arm_wgt += creature.inventory[INVEN_FEET]->weight;

    return monk_arm_wgt > (100 + (creature.get_level() * 4));
}
