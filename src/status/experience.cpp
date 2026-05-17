#include "status/experience.h"
#include "player-base/player-race.h"
#include "player-info/race-types.h"
#include "player/player-status.h"
#include "system/creature-entity.h"
#include "view/display-messages.h"

/*
 * Gain experience
 */
void gain_exp_64(CreatureEntity &creature, int32_t amount, uint32_t amount_frac)
{
    if (creature.is_dead()) {
        return;
    }
    if (CreatureRace(&creature).equals(PlayerRaceType::ANDROID)) {
        return;
    }

    s64b_add(&(creature.exp), &(creature.exp_frac), amount, amount_frac);

    if (creature.get_exp() < creature.get_max_exp()) {
        creature.add_max_exp(amount / 5);
    }

    check_experience(creature);
}

/*
 * Gain experience
 */
void gain_exp(CreatureEntity &creature, int32_t amount)
{
    gain_exp_64(creature, amount, 0L);
}

/*
 * Lose experience
 */
void lose_exp(CreatureEntity &creature, int32_t amount)
{
    if (CreatureRace(&creature).equals(PlayerRaceType::ANDROID)) {
        return;
    }
    if (amount > creature.get_exp()) {
        amount = creature.get_exp();
    }

    creature.sub_exp(amount);

    check_experience(creature);
}

/*
 * Restores any drained experience
 */
bool restore_level(CreatureEntity &creature)
{
    if (creature.get_exp() < creature.get_max_exp()) {
        msg_print(_("経験値が戻ってきた気がする。", "You feel your experience returning."));
        creature.set_exp(creature.get_max_exp());
        check_experience(creature);
        return true;
    }

    return false;
}

/*
 * Drain experience
 * If resisted to draining, return false
 */
bool drain_exp(CreatureEntity &creature, int32_t drain, int32_t slip, int hold_exp_prob)
{
    if (CreatureRace(&creature).equals(PlayerRaceType::ANDROID)) {
        return false;
    }

    if (creature.has_hold_exp() && evaluate_percent(hold_exp_prob)) {
        msg_print(_("しかし自己の経験値を守りきった！", "You keep hold of your experience!"));
        return false;
    }

    if (creature.has_hold_exp()) {
        msg_print(_("経験値を少し吸い取られた気がする！", "You feel your experience slipping away!"));
        lose_exp(creature, slip);
    } else {
        msg_print(_("経験値が体から吸い取られた気がする！", "You feel your experience draining away!"));
        lose_exp(creature, drain);
    }

    return true;
}
