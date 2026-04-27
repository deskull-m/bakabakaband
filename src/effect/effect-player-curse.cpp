#include "effect/effect-player-curse.h"
#include "blue-magic/blue-magic-checker.h"
#include "effect/effect-player.h"
#include "mind/mind-mirror-master.h"
#include "object-enchant/object-curse.h"
#include "player/player-damage.h"
#include "status/bad-status-setter.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"
#include "world/world.h"

void effect_player_curse_1(CreatureEntity &creature, EffectPlayerType *ep_ptr)
{
    if ((randint0(100 + ep_ptr->rlev / 2) < creature.get_skill_save()) && !check_multishadow(creature)) {
        msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
    } else {
        if (!check_multishadow(creature)) {
            curse_equipment(creature, 15, 0);
        }
        ep_ptr->get_damage = take_hit(creature, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
    }
}

void effect_player_curse_2(CreatureEntity &creature, EffectPlayerType *ep_ptr)
{
    if ((randint0(100 + ep_ptr->rlev / 2) < creature.get_skill_save()) && !check_multishadow(creature)) {
        msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
    } else {
        if (!check_multishadow(creature)) {
            curse_equipment(creature, 25, std::min(ep_ptr->rlev / 2 - 15, 5));
        }
        ep_ptr->get_damage = take_hit(creature, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
    }
}

void effect_player_curse_3(CreatureEntity &creature, EffectPlayerType *ep_ptr)
{
    if ((randint0(100 + ep_ptr->rlev / 2) < creature.get_skill_save()) && !check_multishadow(creature)) {
        msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
    } else {
        if (!check_multishadow(creature)) {
            curse_equipment(creature, 33, std::min(ep_ptr->rlev / 2 - 15, 15));
        }
        ep_ptr->get_damage = take_hit(creature, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
    }
}

void effect_player_curse_4(CreatureEntity &creature, EffectPlayerType *ep_ptr)
{
    if ((randint0(100 + ep_ptr->rlev / 2) < creature.get_skill_save()) && (ep_ptr->m_ptr->r_idx != MonraceId::KENSHIROU) && !check_multishadow(creature)) {
        msg_print(_("しかし秘孔を跳ね返した！", "You resist the effects!"));
        return;
    }

    ep_ptr->get_damage = take_hit(creature, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
    if (!check_multishadow(creature)) {
        (void)BadStatusSetter(creature).mod_cut(static_cast<TIME_EFFECT>(Dice::roll(10, 10)));
    }
}
