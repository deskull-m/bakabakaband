#pragma once

enum class BlowEffectType {
    NONE = 0,
    FEAR = 1,
    SLEEP = 2,
    HEAL = 3,
};

struct mam_type;
class CreatureEntity;
class FloorType;
void describe_melee_method(mam_type *mam_ptr);
void decide_monster_attack_effect(CreatureEntity &creature, mam_type *mam_ptr);
void describe_monster_missed_monster(FloorType &floor, mam_type *mam_ptr);
