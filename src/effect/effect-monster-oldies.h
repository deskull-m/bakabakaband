#pragma once

#include "system/angband.h"

class EffectMonster;
class CreatureEntity;
ProcessResult effect_monster_old_poly(EffectMonster *em_ptr);
ProcessResult effect_monster_old_clone(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_star_heal(CreatureEntity &creature, EffectMonster *em_ptr);
void recover_monster_by_heal(CreatureEntity &creature, MONSTER_IDX m_idx, bool show_msg);
ProcessResult effect_monster_old_heal(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_old_speed(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_old_slow(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_old_sleep(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_old_conf(CreatureEntity &creature, EffectMonster *em_ptr);
ProcessResult effect_monster_stasis(EffectMonster *em_ptr, bool to_evil);
ProcessResult effect_monster_stun(EffectMonster *em_ptr);
