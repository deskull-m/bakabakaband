#include "effect/effect-player-oldies.h"
#include "effect/effect-player.h"
#include "game-option/birth-options.h"
#include "hpmp/hp-mp-processor.h"
#include "player/eldritch-horror.h"
#include "status/bad-status-setter.h"
#include "status/buff-setter.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "view/display-messages.h"

void effect_player_old_heal(CreatureEntity &creature, EffectPlayerType *ep_ptr)
{
    if (creature.is_blind()) {
        msg_print(_("何らかの攻撃によって気分がよくなった。", "You are hit by something invigorating!"));
    }

    (void)hp_player(creature, ep_ptr->dam);
    ep_ptr->dam = 0;
}

void effect_player_old_speed(CreatureEntity &creature, EffectPlayerType *ep_ptr)
{
    if (creature.is_blind()) {
        msg_print(_("何かで攻撃された！", "You are hit by something!"));
    }

    (void)mod_acceleration(creature, randnum1<short>(5), false);
    ep_ptr->dam = 0;
}

void effect_player_old_slow(CreatureEntity &creature)
{
    if (creature.is_blind()) {
        msg_print(_("何か遅いもので攻撃された！", "You are hit by something slow!"));
    }

    (void)BadStatusSetter(creature).mod_deceleration(randint0(4) + 4, false);
}

void effect_player_old_sleep(CreatureEntity &creature, EffectPlayerType *ep_ptr)
{
    if (creature.has_free_act()) {
        return;
    }

    if (creature.is_blind()) {
        msg_print(_("眠ってしまった！", "You fall asleep!"));
    }

    if (ironman_nightmare) {
        msg_print(_("恐ろしい光景が頭に浮かんできた。", "A horrible vision enters your mind."));

        /* Have some nightmares */
        sanity_blast(creature);
    }

    (void)BadStatusSetter(creature).mod_paralysis(static_cast<TIME_EFFECT>(ep_ptr->dam));
    ep_ptr->dam = 0;
}
