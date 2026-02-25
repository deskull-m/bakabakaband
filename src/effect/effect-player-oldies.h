#pragma once

class EffectPlayerType;
class PlayerType;
class CreatureEntity;
void effect_player_old_heal(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
void effect_player_old_speed(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
void effect_player_old_slow(CreatureEntity &creature);
void effect_player_old_sleep(PlayerType *player_ptr, EffectPlayerType *ep_ptr);
