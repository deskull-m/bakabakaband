#include "effect/effect-monster-lite-dark.h"
#include "effect/effect-monster-util.h"
#include "monster-race/race-flags-resistance.h"
#include "monster/monster-info.h"
#include "object-enchant/tr-types.h"
#include "system/creature-entity.h"
#include "system/monrace/monrace-definition.h"

ProcessResult effect_monster_lite_weak(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (!em_ptr->dam) {
        em_ptr->skipped = true;
        return ProcessResult::PROCESS_CONTINUE;
    }

    if (em_ptr->monrace->resistance_flags.has_not(MonsterResistanceType::HURT_LITE)) {
        em_ptr->dam = 0;
        return ProcessResult::PROCESS_CONTINUE;
    }

    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
        em_ptr->monrace->r_resistance_flags.set(MonsterResistanceType::HURT_LITE);
    }

    em_ptr->note = _("は光に身をすくめた！", " cringes from the light!");
    em_ptr->note_dies = _("は光を受けてしぼんでしまった！", " shrivels away in the light!");
    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_lite(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    // [提案C1第6弾] 付与種族が光耐性 (TR_RES_LITE) を持てばネイティブ耐性と同様に軽減 (opt-in・既定OFF)。
    // 光耐性は光弱点 (HURT_LITE) より優先 (else if で弱点経路を回避)。
    const auto native_lite_resist = em_ptr->monrace->resistance_flags.has(MonsterResistanceType::RESIST_LITE);
    if (native_lite_resist || target_race_resists_element(em_ptr, TR_RES_LITE)) {
        em_ptr->note = _("には耐性がある！", " resists!");
        em_ptr->dam *= 2;
        em_ptr->dam /= (randint1(6) + 6);
        if (native_lite_resist && is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
            em_ptr->monrace->r_resistance_flags.set(MonsterResistanceType::RESIST_LITE);
        }
    } else if (em_ptr->monrace->resistance_flags.has(MonsterResistanceType::HURT_LITE)) {
        if (is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
            em_ptr->monrace->r_resistance_flags.set(MonsterResistanceType::HURT_LITE);
        }

        em_ptr->note = _("は光に身をすくめた！", " cringes from the light!");
        em_ptr->note_dies = _("は光を受けてしぼんでしまった！", " shrivels away in the light!");
        em_ptr->dam *= 2;
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_dark(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    // [提案C1第6弾] 付与種族が闇耐性 (TR_RES_DARK) を持てばネイティブ耐性と同様に軽減 (opt-in・既定OFF)。
    const auto native_dark_resist = em_ptr->monrace->resistance_flags.has(MonsterResistanceType::RESIST_DARK);
    if (!native_dark_resist && !target_race_resists_element(em_ptr, TR_RES_DARK)) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    em_ptr->note = _("には耐性がある！", " resists!");
    em_ptr->dam *= 2;
    em_ptr->dam /= (randint1(6) + 6);
    if (native_dark_resist && is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
        em_ptr->monrace->r_resistance_flags.set(MonsterResistanceType::RESIST_DARK);
    }

    return ProcessResult::PROCESS_CONTINUE;
}
