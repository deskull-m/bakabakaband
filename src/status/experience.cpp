#include "status/experience.h"
#include "player-base/player-race.h"
#include "player/player-status.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*
 * Gain experience
 */
void gain_exp_64(PlayerType *player_ptr, int32_t amount, uint32_t amount_frac)
{
    if (player_ptr->is_dead()) {
        return;
    }
    if (PlayerRace(player_ptr).equals(PlayerRaceType::ANDROID)) {
        return;
    }

    auto &creature = static_cast<CreatureEntity &>(*player_ptr);
    s64b_add(&(creature.exp), &(creature.exp_frac), amount, amount_frac);

    if (creature.exp < creature.max_exp) {
        creature.max_exp += amount / 5;
    }

    check_experience(creature);
}

/*
 * Gain experience
 */
void gain_exp(CreatureEntity &creature, int32_t amount)
{
    auto *player_ptr = dynamic_cast<PlayerType *>(&creature);
    if (!player_ptr) {
        return;
    }
    gain_exp_64(player_ptr, amount, 0L);
}

/*
 * Lose experience
 */
void lose_exp(CreatureEntity &creature, int32_t amount)
{
    auto *player_ptr = dynamic_cast<PlayerType *>(&creature);
    if (!player_ptr) {
        return;
    }
    if (PlayerRace(player_ptr).equals(PlayerRaceType::ANDROID)) {
        return;
    }
    if (amount > creature.exp) {
        amount = creature.exp;
    }

    creature.exp -= amount;

    check_experience(creature);
}

/*
 * Restores any drained experience
 */
bool restore_level(CreatureEntity &creature)
{
    if (creature.exp < creature.max_exp) {
        msg_print(_("経験値が戻ってきた気がする。", "You feel your experience returning."));
        creature.exp = creature.max_exp;
        check_experience(creature);
        return true;
    }

    return false;
}

/*
 * Drain experience
 * If resisted to draining, return false
 */
bool drain_exp(PlayerType *player_ptr, int32_t drain, int32_t slip, int hold_exp_prob)
{
    if (PlayerRace(player_ptr).equals(PlayerRaceType::ANDROID)) {
        return false;
    }

    if (player_ptr->hold_exp && evaluate_percent(hold_exp_prob)) {
        msg_print(_("しかし自己の経験値を守りきった！", "You keep hold of your experience!"));
        return false;
    }

    if (player_ptr->hold_exp) {
        msg_print(_("経験値を少し吸い取られた気がする！", "You feel your experience slipping away!"));
        lose_exp(static_cast<CreatureEntity &>(*player_ptr), slip);
    } else {
        msg_print(_("経験値が体から吸い取られた気がする！", "You feel your experience draining away!"));
        lose_exp(static_cast<CreatureEntity &>(*player_ptr), drain);
    }

    return true;
}
