#pragma once

#include <string_view>

class CreatureEntity;
class EffectPlayerType;
void effect_player_elements(CreatureEntity &creature, EffectPlayerType *ep_ptr, std::string_view attack_message, int (*damage_func)(CreatureEntity &, int, std::string_view, bool));
void effect_player_poison(CreatureEntity &creature, EffectPlayerType *ep_ptr);
void effect_player_nuke(CreatureEntity &creature, EffectPlayerType *ep_ptr);
void effect_player_missile(CreatureEntity &creature, EffectPlayerType *ep_ptr);
void effect_player_holy_fire(CreatureEntity &creature, EffectPlayerType *ep_ptr);
void effect_player_hell_fire(CreatureEntity &creature, EffectPlayerType *ep_ptr);
void effect_player_arrow(CreatureEntity &creature, EffectPlayerType *ep_ptr);
void effect_player_plasma(CreatureEntity &creature, EffectPlayerType *ep_ptr);
void effect_player_nether(CreatureEntity &creature, EffectPlayerType *ep_ptr);
void effect_player_water(CreatureEntity &creature, EffectPlayerType *ep_ptr);
void effect_player_chaos(CreatureEntity &creature, EffectPlayerType *ep_ptr);
void effect_player_shards(CreatureEntity &creature, EffectPlayerType *ep_ptr);
void effect_player_sound(CreatureEntity &creature, EffectPlayerType *ep_ptr);
void effect_player_confusion(CreatureEntity &creature, EffectPlayerType *ep_ptr);
void effect_player_disenchant(CreatureEntity &creature, EffectPlayerType *ep_ptr);
void effect_player_nexus(CreatureEntity &creature, EffectPlayerType *ep_ptr);
void effect_player_force(CreatureEntity &creature, EffectPlayerType *ep_ptr);
void effect_player_rocket(CreatureEntity &creature, EffectPlayerType *ep_ptr);
void effect_player_inertial(CreatureEntity &creature, EffectPlayerType *ep_ptr);
void effect_player_lite(CreatureEntity &creature, EffectPlayerType *ep_ptr);
void effect_player_dark(CreatureEntity &creature, EffectPlayerType *ep_ptr);
void effect_player_time(CreatureEntity &creature, EffectPlayerType *ep_ptr);
void effect_player_gravity(CreatureEntity &creature, EffectPlayerType *ep_ptr);
void effect_player_disintegration(CreatureEntity &creature, EffectPlayerType *ep_ptr);
void effect_player_death_ray(CreatureEntity &creature, EffectPlayerType *ep_ptr);
void effect_player_mana(CreatureEntity &creature, EffectPlayerType *ep_ptr);
void effect_player_psy_spear(CreatureEntity &creature, EffectPlayerType *ep_ptr);
void effect_player_meteor(CreatureEntity &creature, EffectPlayerType *ep_ptr);
void effect_player_icee(CreatureEntity &creature, EffectPlayerType *ep_ptr);
void effect_player_hand_doom(CreatureEntity &creature, EffectPlayerType *ep_ptr);
void effect_player_void(CreatureEntity &creature, EffectPlayerType *ep_ptr);
void effect_player_abyss(CreatureEntity &creature, EffectPlayerType *ep_ptr);
void effect_player_spider_string(CreatureEntity &creature, EffectPlayerType *ep_ptr);
