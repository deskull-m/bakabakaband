#include "effect/effect-player-switcher.h"
#include "effect/attribute-types.h"
#include "effect/effect-player-curse.h"
#include "effect/effect-player-oldies.h"
#include "effect/effect-player-resist-hurt.h"
#include "effect/effect-player-spirit.h"
#include "effect/effect-player.h"
#include "player/player-damage.h"
#include "system/creature-entity.h"
#include "system/player-type-definition.h"

/*!
 * @brief
 * 魔法の効果によって様々なメッセーを出力したり与えるダメージの増減を行ったりする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr プレイヤー効果構造体への参照ポインタ
 */
void switch_effects_player(CreatureEntity &creature, EffectPlayerType *ep_ptr)
{
    switch (ep_ptr->attribute) {
    case AttributeType::ACID:
        effect_player_elements(creature, ep_ptr, _("酸で攻撃された！", "You are hit by acid!"), acid_dam);
        return;
    case AttributeType::FIRE:
        effect_player_elements(creature, ep_ptr, _("火炎で攻撃された！", "You are hit by fire!"), fire_dam);
        return;
    case AttributeType::COLD:
        effect_player_elements(creature, ep_ptr, _("冷気で攻撃された！", "You are hit by cold!"), cold_dam);
        return;
    case AttributeType::ELEC:
        effect_player_elements(creature, ep_ptr, _("電撃で攻撃された！", "You are hit by lightning!"), elec_dam);
        return;
    case AttributeType::POIS:
        effect_player_poison(creature, ep_ptr);
        return;
    case AttributeType::NUKE:
        effect_player_nuke(creature, ep_ptr);
        return;
    case AttributeType::MISSILE:
    case AttributeType::MONSTER_MELEE:
        effect_player_missile(creature, ep_ptr);
        return;
    case AttributeType::HOLY_FIRE:
        effect_player_holy_fire(creature, ep_ptr);
        return;
    case AttributeType::HELL_FIRE:
        effect_player_hell_fire(creature, ep_ptr);
        return;
    case AttributeType::MONSTER_SHOOT:
        effect_player_arrow(creature, ep_ptr);
        return;
    case AttributeType::PLASMA:
        effect_player_plasma(creature, ep_ptr);
        return;
    case AttributeType::NETHER:
        effect_player_nether(creature, ep_ptr);
        return;
    case AttributeType::WATER:
        effect_player_water(creature, ep_ptr);
        return;
    case AttributeType::CHAOS:
        effect_player_chaos(creature, ep_ptr);
        return;
    case AttributeType::SHARDS:
        effect_player_shards(creature, ep_ptr);
        return;
    case AttributeType::SOUND:
        effect_player_sound(creature, ep_ptr);
        return;
    case AttributeType::CONFUSION:
        effect_player_confusion(creature, ep_ptr);
        return;
    case AttributeType::DISENCHANT:
        effect_player_disenchant(creature, ep_ptr);
        return;
    case AttributeType::NEXUS:
        effect_player_nexus(creature, ep_ptr);
        return;
    case AttributeType::FORCE:
        effect_player_force(creature, ep_ptr);
        return;
    case AttributeType::ROCKET:
        effect_player_rocket(creature, ep_ptr);
        return;
    case AttributeType::INERTIAL:
        effect_player_inertial(creature, ep_ptr);
        return;
    case AttributeType::LITE:
        effect_player_lite(creature, ep_ptr);
        return;
    case AttributeType::DARK:
        effect_player_dark(creature, ep_ptr);
        return;
    case AttributeType::TIME:
        effect_player_time(creature, ep_ptr);
        return;
    case AttributeType::GRAVITY:
        effect_player_gravity(creature, ep_ptr);
        return;
    case AttributeType::DISINTEGRATE:
        effect_player_disintegration(creature, ep_ptr);
        return;
    case AttributeType::OLD_HEAL:
        effect_player_old_heal(creature, ep_ptr);
        return;
    case AttributeType::OLD_SPEED:
        effect_player_old_speed(creature, ep_ptr);
        return;
    case AttributeType::OLD_SLOW:
        effect_player_old_slow(creature);
        return;
    case AttributeType::OLD_SLEEP:
        effect_player_old_sleep(creature, ep_ptr);
        return;
    case AttributeType::MANA:
    case AttributeType::SEEKER:
    case AttributeType::SUPER_RAY:
        effect_player_mana(creature, ep_ptr);
        return;
    case AttributeType::PSY_SPEAR:
        effect_player_psy_spear(creature, ep_ptr);
        return;
    case AttributeType::METEOR:
        effect_player_meteor(creature, ep_ptr);
        return;
    case AttributeType::ICE:
        effect_player_icee(creature, ep_ptr);
        return;
    case AttributeType::DEATH_RAY:
        effect_player_death_ray(creature, ep_ptr);
        return;
    case AttributeType::DRAIN_MANA:
        effect_player_drain_mana(creature, ep_ptr);
        return;
    case AttributeType::MIND_BLAST:
        effect_player_mind_blast(creature, ep_ptr);
        return;
    case AttributeType::BRAIN_SMASH:
        effect_player_brain_smash(creature, ep_ptr);
        return;
    case AttributeType::CAUSE_1:
        effect_player_curse_1(creature, ep_ptr);
        return;
    case AttributeType::CAUSE_2:
        effect_player_curse_2(creature, ep_ptr);
        return;
    case AttributeType::CAUSE_3:
        effect_player_curse_3(creature, ep_ptr);
        return;
    case AttributeType::CAUSE_4:
        effect_player_curse_4(creature, ep_ptr);
        return;
    case AttributeType::HAND_DOOM:
        effect_player_hand_doom(creature, ep_ptr);
        return;
    case AttributeType::VOID_MAGIC:
        effect_player_void(creature, ep_ptr);
        return;
    case AttributeType::ABYSS:
        effect_player_abyss(creature, ep_ptr);
        return;
    case AttributeType::SPIDER_STRING:
        effect_player_spider_string(creature, ep_ptr);
        return;
    default: {
        ep_ptr->dam = 0;
        return;
    }
    }
}
