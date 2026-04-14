#include "player/race-resistances.h"
#include "inventory/inventory-slot-types.h"
#include "mind/mind-elementalist.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/tr-types.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/race-info.h"
#include "player-info/samurai-data-type.h"
#include "player/special-defense-types.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief プレイヤーの職業/種族による免疫フラグを返す
 * @param creature クリーチャーへの参照
 * @param flags フラグを保管する配列
 */
void player_immunity(CreatureEntity &creature, TrFlags &flags)
{
    flags.clear();

    const auto race = CreatureRace(&creature);
    const auto p_flags = (race.tr_flags() | CreatureClass(creature).tr_flags());

    if (p_flags.has(TR_IM_ACID)) {
        flags.set(TR_RES_ACID);
    }
    if (p_flags.has(TR_IM_COLD)) {
        flags.set(TR_RES_COLD);
    }
    if (p_flags.has(TR_IM_ELEC)) {
        flags.set(TR_RES_ELEC);
    }
    if (p_flags.has(TR_IM_FIRE)) {
        flags.set(TR_RES_FIRE);
    }
    if (p_flags.has(TR_IM_DARK)) {
        flags.set(TR_RES_DARK);
    }
    if (p_flags.has(TR_IM_LITE)) {
        flags.set(TR_RES_LITE);
    }

    if (race.equals(PlayerRaceType::SPECTRE)) {
        flags.set(TR_RES_NETHER);
    }
}

/*!
 * @brief プレイヤーの一時的魔法効果による免疫フラグを返す
 * @param creature クリーチャーへの参照
 * @param flags フラグを保管する配列
 */
void tim_player_immunity(CreatureEntity &creature, TrFlags &flags)
{
    flags.clear();

    if (creature.special_defense & DEFENSE_ACID) {
        flags.set(TR_RES_ACID);
    }
    if (creature.special_defense & DEFENSE_ELEC) {
        flags.set(TR_RES_ELEC);
    }
    if (creature.special_defense & DEFENSE_FIRE) {
        flags.set(TR_RES_FIRE);
    }
    if (creature.special_defense & DEFENSE_COLD) {
        flags.set(TR_RES_COLD);
    }
    if (creature.get_timed_effect(CreatureTimedEffect::WRAITH_FORM)) {
        flags.set(TR_RES_DARK);
    }
}

/*!
 * @brief プレイヤーの装備による免疫フラグを返す
 * @param creature クリーチャーへの参照
 * @param flags フラグを保管する配列
 */
void known_obj_immunity(CreatureEntity &creature, TrFlags &flags)
{
    flags.clear();

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        const auto *o_ptr = creature.inventory[i].get();
        if (!o_ptr->is_valid()) {
            continue;
        }

        const auto o_flags = o_ptr->get_flags_known();
        if (o_flags.has(TR_IM_ACID)) {
            flags.set(TR_RES_ACID);
        }
        if (o_flags.has(TR_IM_ELEC)) {
            flags.set(TR_RES_ELEC);
        }
        if (o_flags.has(TR_IM_FIRE)) {
            flags.set(TR_RES_FIRE);
        }
        if (o_flags.has(TR_IM_COLD)) {
            flags.set(TR_RES_COLD);
        }
    }
}

/*!
 * @brief プレイヤーの種族による弱点フラグを返す
 * @param creature クリーチャーへの参照
 * @param flags フラグを保管する配列
 */
void player_vulnerability_flags(CreatureEntity &creature, TrFlags &flags)
{
    flags.clear();

    if (creature.muta.has(PlayerMutationType::VULN_ELEM) || CreatureClass(creature).samurai_stance_is(SamuraiStanceType::KOUKIJIN)) {
        flags.set(TR_RES_ACID);
        flags.set(TR_RES_ELEC);
        flags.set(TR_RES_FIRE);
        flags.set(TR_RES_COLD);
    }

    const auto p_flags = CreatureRace(&creature).tr_flags();

    if (p_flags.has(TR_VUL_ACID)) {
        flags.set(TR_RES_ACID);
    }
    if (p_flags.has(TR_VUL_COLD)) {
        flags.set(TR_RES_COLD);
    }
    if (p_flags.has(TR_VUL_ELEC)) {
        flags.set(TR_RES_ELEC);
    }
    if (p_flags.has(TR_VUL_FIRE)) {
        flags.set(TR_RES_FIRE);
    }
    if (p_flags.has(TR_VUL_LITE)) {
        flags.set(TR_RES_LITE);
    }
}
