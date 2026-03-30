#include "player/permanent-resistances.h"
#include "inventory/inventory-slot-types.h"
#include "mind/mind-elementalist.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/tr-types.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/equipment-info.h"
#include "player-info/race-types.h"
#include "player/player-personality-types.h"
#include "player/player-status-flags.h"
#include "player/special-defense-types.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief 突然変異による耐性フラグを返す
 * @param creature クリーチャーへの参照
 * @param flags 耐性フラグの配列
 * @todo 最終的にplayer-status系列と統合する
 */
static void add_mutation_flags(CreatureEntity &creature, TrFlags &flags)
{
    auto &player = static_cast<PlayerType &>(creature);
    if (player.muta.none()) {
        return;
    }

    if (player.muta.has(PlayerMutationType::FLESH_ROT)) {
        flags.reset(TR_REGEN);
    }
    if (player.muta.has_any_of({ PlayerMutationType::XTRA_FAT, PlayerMutationType::XTRA_LEGS, PlayerMutationType::SHORT_LEG, PlayerMutationType::WEAK_LOWER_BODY })) {
        flags.set(TR_SPEED);
    }
    if (player.muta.has(PlayerMutationType::ELEC_TOUC)) {
        flags.set(TR_SH_ELEC);
    }
    if (player.muta.has(PlayerMutationType::FIRE_BODY)) {
        flags.set(TR_SH_FIRE);
        flags.set(TR_LITE_1);
    }

    if (player.muta.has(PlayerMutationType::WINGS)) {
        flags.set(TR_LEVITATION);
    }
    if (player.muta.has(PlayerMutationType::FEARLESS)) {
        flags.set(TR_RES_FEAR);
    }
    if (player.muta.has(PlayerMutationType::REGEN)) {
        flags.set(TR_REGEN);
    }
    if (player.muta.has(PlayerMutationType::ESP)) {
        flags.set(TR_TELEPATHY);
    }
    if (player.muta.has(PlayerMutationType::MOTION)) {
        flags.set(TR_FREE_ACT);
    }
}

/*!
 * @brief 性格による耐性フラグを返す
 * @param creature クリーチャーへの参照
 * @param flags 耐性フラグの配列
 * @todo 最終的にplayer-status系列と統合する
 */
static void add_personality_flags(CreatureEntity &creature, TrFlags &flags)
{
    auto &player = static_cast<PlayerType &>(creature);
    if (player.ppersonality == PERSONALITY_SEXY) {
        flags.set(TR_AGGRAVATE);
    }

    if (player.ppersonality == PERSONALITY_CHARGEMAN) {
        flags.set(TR_RES_CONF);
    }

    if (player.ppersonality != PERSONALITY_MUNCHKIN) {
        return;
    }

    flags.set(TR_RES_BLIND);
    flags.set(TR_RES_CONF);
    flags.set(TR_HOLD_EXP);
    if (!CreatureClass(creature).equals(PlayerClassType::NINJA)) {
        flags.set(TR_LITE_1);
    }
    if (player.level > 9) {
        flags.set(TR_SPEED);
    }
}

/*!
 * @brief プレイヤーの職業、種族に応じた耐性フラグを返す
 * Prints ratings on certain abilities
 * @param creature クリーチャーへの参照
 * @param flags フラグを保管する配列
 * @details
 * Obtain the "flags" for the player as if he was an item
 * @todo 最終的にplayer-status系列と統合する
 */
void player_flags(CreatureEntity &creature, TrFlags &flags)
{
    flags.clear();

    flags.set(CreatureClass(creature).tr_flags());
    flags.set(CreatureRace(&creature).tr_flags());

    add_mutation_flags(creature, flags);
    add_personality_flags(creature, flags);
}

void riding_flags(CreatureEntity &creature, TrFlags &flags, TrFlags &negative_flags)
{
    flags.clear();
    negative_flags.clear();

    if (!creature.riding) {
        return;
    }

    if (!creature.is_player()) {
        return;
    }
    auto &player = static_cast<PlayerType &>(creature);

    if (any_bits(has_levitation(player), FLAG_CAUSE_RIDING)) {
        flags.set(TR_LEVITATION);
    } else {
        negative_flags.set(TR_LEVITATION);
    }
}
