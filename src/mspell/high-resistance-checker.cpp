#include "mspell/high-resistance-checker.h"
#include "monster-race/race-ability-flags.h"
#include "monster/smart-learn-types.h"
#include "mspell/smart-mspell-util.h"
#include "player-base/player-race.h"
#include "player-info/race-types.h"
#include "player/player-status-flags.h"
#include "system/creature-entity.h"
#include "util/bit-flags-calculator.h"

void add_cheat_remove_flags_others(CreatureEntity &creature, msr_type *msr_ptr)
{
    if (creature.has_resist_neth()) {
        msr_ptr->smart_flags.set(MonsterSmartLearnType::RES_NETH);
    }

    if (creature.has_resist_lite()) {
        msr_ptr->smart_flags.set(MonsterSmartLearnType::RES_LITE);
    }

    if (creature.has_resist_dark()) {
        msr_ptr->smart_flags.set(MonsterSmartLearnType::RES_DARK);
    }

    if (creature.has_resist_fear()) {
        msr_ptr->smart_flags.set(MonsterSmartLearnType::RES_FEAR);
    }

    if (creature.has_resist_conf()) {
        msr_ptr->smart_flags.set(MonsterSmartLearnType::RES_CONF);
    }

    if (creature.has_resist_chaos()) {
        msr_ptr->smart_flags.set(MonsterSmartLearnType::RES_CHAOS);
    }

    if (creature.has_resist_disen()) {
        msr_ptr->smart_flags.set(MonsterSmartLearnType::RES_DISEN);
    }

    if (creature.has_resist_blind()) {
        msr_ptr->smart_flags.set(MonsterSmartLearnType::RES_BLIND);
    }

    if (creature.has_resist_shard()) {
        msr_ptr->smart_flags.set(MonsterSmartLearnType::RES_NEXUS);
    }

    if (creature.has_resist_sound()) {
        msr_ptr->smart_flags.set(MonsterSmartLearnType::RES_SOUND);
    }

    if (creature.has_resist_shard()) {
        msr_ptr->smart_flags.set(MonsterSmartLearnType::RES_SHARD);
    }

    if (has_reflect(creature)) {
        msr_ptr->smart_flags.set(MonsterSmartLearnType::IMM_REFLECT);
    }

    if (creature.has_free_act()) {
        msr_ptr->smart_flags.set(MonsterSmartLearnType::IMM_FREE);
    }

    if (!creature.msp) {
        msr_ptr->smart_flags.set(MonsterSmartLearnType::IMM_MANA);
    }
}

static void check_nether_resistance(CreatureEntity &creature, msr_type *msr_ptr)
{
    if (msr_ptr->smart_flags.has_not(MonsterSmartLearnType::RES_NETH)) {
        return;
    }

    if (CreatureRace(&creature).equals(PlayerRaceType::SPECTRE)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BR_NETH);
        msr_ptr->ability_flags.reset(MonsterAbilityType::BA_NETH);
        msr_ptr->ability_flags.reset(MonsterAbilityType::BO_NETH);
        return;
    }

    if (int_outof(*msr_ptr->r_ptr, 20)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BR_NETH);
    }

    if (int_outof(*msr_ptr->r_ptr, 50)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BA_NETH);
    }

    if (int_outof(*msr_ptr->r_ptr, 50)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BO_NETH);
    }
}

static void check_lite_resistance(msr_type *msr_ptr)
{
    if (msr_ptr->smart_flags.has_not(MonsterSmartLearnType::RES_LITE)) {
        return;
    }

    if (int_outof(*msr_ptr->r_ptr, 50)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BR_LITE);
    }

    if (int_outof(*msr_ptr->r_ptr, 50)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BA_LITE);
    }

    if (int_outof(*msr_ptr->r_ptr, 50)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BO_LITE);
    }
}

static void check_dark_resistance(CreatureEntity &creature, msr_type *msr_ptr)
{
    if (msr_ptr->smart_flags.has_not(MonsterSmartLearnType::RES_DARK)) {
        return;
    }

    if (has_immune_dark(creature)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BR_DARK);
        msr_ptr->ability_flags.reset(MonsterAbilityType::BA_DARK);
        return;
    }

    if (int_outof(*msr_ptr->r_ptr, 50)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BR_DARK);
    }

    if (int_outof(*msr_ptr->r_ptr, 50)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BA_DARK);
    }
}

static void check_conf_resistance(msr_type *msr_ptr)
{
    if (msr_ptr->smart_flags.has_not(MonsterSmartLearnType::RES_CONF)) {
        return;
    }

    msr_ptr->ability_flags.reset(MonsterAbilityType::CONF);
    if (int_outof(*msr_ptr->r_ptr, 50)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BR_CONF);
    }
}

static void check_chaos_resistance(msr_type *msr_ptr)
{
    if (msr_ptr->smart_flags.has_not(MonsterSmartLearnType::RES_CHAOS)) {
        return;
    }

    if (int_outof(*msr_ptr->r_ptr, 20)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BR_CHAO);
    }

    if (int_outof(*msr_ptr->r_ptr, 50)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BA_CHAO);
    }
}

static void check_nexus_resistance(msr_type *msr_ptr)
{
    if (msr_ptr->smart_flags.has_not(MonsterSmartLearnType::RES_NEXUS)) {
        return;
    }

    if (int_outof(*msr_ptr->r_ptr, 50)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BR_NEXU);
    }

    msr_ptr->ability_flags.reset(MonsterAbilityType::TELE_LEVEL);
}

static void check_reflection(msr_type *msr_ptr)
{
    if (msr_ptr->smart_flags.has_not(MonsterSmartLearnType::IMM_REFLECT)) {
        return;
    }

    if (int_outof(*msr_ptr->r_ptr, 150)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BO_COLD);
    }

    if (int_outof(*msr_ptr->r_ptr, 150)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BO_FIRE);
    }

    if (int_outof(*msr_ptr->r_ptr, 150)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BO_ACID);
    }

    if (int_outof(*msr_ptr->r_ptr, 150)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BO_ELEC);
    }

    if (int_outof(*msr_ptr->r_ptr, 150)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BO_NETH);
    }

    if (int_outof(*msr_ptr->r_ptr, 150)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BO_WATE);
    }

    if (int_outof(*msr_ptr->r_ptr, 150)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BO_MANA);
    }

    if (int_outof(*msr_ptr->r_ptr, 150)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BO_PLAS);
    }

    if (int_outof(*msr_ptr->r_ptr, 150)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BO_ICEE);
    }

    if (int_outof(*msr_ptr->r_ptr, 150)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BO_VOID);
    }

    if (int_outof(*msr_ptr->r_ptr, 150)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BO_ABYSS);
    }

    if (int_outof(*msr_ptr->r_ptr, 150)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BO_METEOR);
    }

    if (int_outof(*msr_ptr->r_ptr, 150)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BO_LITE);
    }

    if (int_outof(*msr_ptr->r_ptr, 150)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::MISSILE);
    }
}

void check_high_resistances(CreatureEntity &creature, msr_type *msr_ptr)
{
    check_nether_resistance(creature, msr_ptr);
    check_lite_resistance(msr_ptr);
    check_dark_resistance(creature, msr_ptr);
    if (msr_ptr->smart_flags.has(MonsterSmartLearnType::RES_FEAR)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::SCARE);
    }

    check_conf_resistance(msr_ptr);
    check_chaos_resistance(msr_ptr);
    if (msr_ptr->smart_flags.has(MonsterSmartLearnType::RES_DISEN) && int_outof(*msr_ptr->r_ptr, 40)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BR_DISE);
    }

    if (msr_ptr->smart_flags.has(MonsterSmartLearnType::RES_BLIND)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BLIND);
    }

    check_nexus_resistance(msr_ptr);
    if (msr_ptr->smart_flags.has(MonsterSmartLearnType::RES_SOUND) && int_outof(*msr_ptr->r_ptr, 50)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BR_SOUN);
    }

    if (msr_ptr->smart_flags.has(MonsterSmartLearnType::RES_SHARD) && int_outof(*msr_ptr->r_ptr, 40)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BR_SHAR);
    }

    check_reflection(msr_ptr);
    if (msr_ptr->smart_flags.has(MonsterSmartLearnType::IMM_FREE)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::HOLD);
        msr_ptr->ability_flags.reset(MonsterAbilityType::SLOW);
    }

    if (msr_ptr->smart_flags.has(MonsterSmartLearnType::IMM_MANA)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::DRAIN_MANA);
    }
}
