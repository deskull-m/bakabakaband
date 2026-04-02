#pragma once

#include "effect/attribute-types.h"
#include "system/angband.h"

enum class MonraceId : int16_t;

class CreatureEntity;
class FloorType;
typedef union spell_functions {
    struct debug_spell_type1 {
        bool (*spell_function)(CreatureEntity &, FloorType *);
    } spell1;

    struct debug_spell_type2 {
        bool (*spell_function)(CreatureEntity &);
    } spell2;

    struct debug_spell_type3 {
        bool (*spell_function)(CreatureEntity &, int);
    } spell3;

    struct debug_spell_type4 { // 実質 ty curse
        bool (*spell_function)(CreatureEntity &, bool, int *);
    } spell4;

    struct debug_spell_type5 {
        void (*spell_function)(CreatureEntity &);
    } spell5;

    struct debug_spell_type6 {
        void (*spell_function)(CreatureEntity &);
    } spell6;

} spell_functions;

struct debug_spell_command {
    int type;
    concptr command_name;
    spell_functions command_function;
};

void wiz_debug_spell(CreatureEntity &creature);
void wiz_dimension_door(CreatureEntity &creature);
void wiz_summon_horde(CreatureEntity &creature);
void wiz_teleport_back(CreatureEntity &creature);
void wiz_learn_blue_magic_all(CreatureEntity &creature);
void wiz_fillup_all_smith_essences(CreatureEntity &creature);
void wiz_generate_random_monster(CreatureEntity &creature, int num);
void wiz_summon_random_monster(CreatureEntity &creature, int num);
void wiz_summon_specific_monster(CreatureEntity &creature, MonraceId monrace_id);
void wiz_summon_pet(CreatureEntity &creature, MonraceId monrace_id);
void wiz_summon_clone(CreatureEntity &creature, MonraceId monrace_id);
void wiz_generate_room(CreatureEntity &creature, int v_idx);
void wiz_kill_target(CreatureEntity &creature, int initial_dam = 1000000, AttributeType effect_idx = AttributeType::DEBUG, const bool self = false);
