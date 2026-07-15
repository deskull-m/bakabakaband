#include "effect/effect-monster-lite-dark.h"
#include "effect/effect-monster-util.h"
#include "monster-race/race-flags-resistance.h"
#include "monster/monster-info.h"
#include "object-enchant/tr-types.h"
#include "system/creature-entity.h"
#include "system/monrace/monrace-definition.h"
#include "term/z-rand.h"

namespace {
/*!
 * @brief 光/闇属性の部分耐性の共通処理 (提案D4第2弾)
 * @param native_resist monrace 固有耐性フラグを持つか (思い出記録は固有耐性時のみ)
 * @details 「耐性がある！」メッセージ・ダメージ *2/(1d6+6)・思い出記録 (native_resist 時) を
 *          集約。lite (第1分岐) と dark で byte 一致だったブロックを統合。挙動不変。
 *          resist-hurt の `apply_monster_element_resist_native` とは軽減率 (*2 vs *3)・
 *          メッセージ (" resists!" vs " resists.") が異なるため別ヘルパ。
 */
void apply_monster_lite_dark_resist(CreatureEntity &creature, EffectMonster *em_ptr, MonsterResistanceType resist_flag, bool native_resist)
{
    em_ptr->note = _("には耐性がある！", " resists!");
    em_ptr->dam *= 2;
    em_ptr->dam /= (randint1(6) + 6);
    if (native_resist && is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
        em_ptr->monrace->r_resistance_flags.set(resist_flag);
    }
}
}

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
        apply_monster_lite_dark_resist(creature, em_ptr, MonsterResistanceType::RESIST_LITE, native_lite_resist);
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

    apply_monster_lite_dark_resist(creature, em_ptr, MonsterResistanceType::RESIST_DARK, native_dark_resist);
    return ProcessResult::PROCESS_CONTINUE;
}
