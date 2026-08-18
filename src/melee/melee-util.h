#pragma once

#include "system/angband.h"
#include "util/dice.h"

enum class AttributeType;
enum class BlowEffectType;
enum class RaceBlowEffectType;
enum class RaceBlowMethodType;

/* monster-attack-monster type*/
class CreatureEntity;
struct mam_type {
    BlowEffectType attribute{};
    MONSTER_IDX m_idx = 0;
    MONSTER_IDX t_idx = 0;
    CreatureEntity *m_ptr = nullptr;
    CreatureEntity *t_ptr = nullptr;
    GAME_TEXT m_name[MAX_NLEN]{};
    GAME_TEXT t_name[MAX_NLEN]{};
    int damage = 0;
    bool see_m = false;
    bool see_t = false;
    bool see_either = false;
    POSITION y_saver = 0;
    POSITION x_saver = 0;
    RaceBlowMethodType method{};
    bool explode = false;
    bool touched = false;
    concptr act = "";
    AttributeType pt{};
    RaceBlowEffectType effect{};
    ARMOUR_CLASS ac = 0;
    DEPTH rlev = 0;
    bool blinked = false;
    bool do_silly_attack = false;
    int power = 0;
    bool obvious = false;
    Dice damage_dice{};
    bool known = false;
    bool fear = false;
    bool dead = false;
    short weapon_slot_for_blow = -1; //!< 当該打撃で使用する武器スロット (-1 = 武器なし)
    bool do_quake = false; //!< [B-2b5] 地震武器による地震を全打撃終了後に起こすか
};

mam_type *initialize_mam_type(CreatureEntity &creature, mam_type *mam_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx);
