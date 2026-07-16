#include "effect/effect-monster-resist-hurt.h"
#include "effect/effect-monster-util.h"
#include "monster-race/race-brightness-flags.h"
#include "monster-race/race-flags-resistance.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "object-enchant/tr-types.h"
#include "system/creature-entity.h"
#include "system/creature-timed-effect-types.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "util/bit-flags-calculator.h"
#include <algorithm>

namespace {
/*!
 * @brief モンスターが属性に免疫を持つ場合の共通処理 (提案D4)
 * @details ダメージ 1/9・「かなり耐性がある」メッセージ・思い出フラグ記録を集約。
 *          属性別に重複していた同一処理の統合 (挙動不変、monrace フラグ読取・
 *          思い出記録のセマンティクスは維持)。
 */
void apply_monster_element_immune(CreatureEntity &creature, EffectMonster *em_ptr, MonsterResistanceType immune_flag)
{
    em_ptr->note = _("にはかなり耐性がある！", " resists a lot.");
    em_ptr->dam /= 9;
    if (is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
        em_ptr->monrace->r_resistance_flags.set(immune_flag);
    }
}

/*!
 * @brief モンスターが属性に弱点を持つ場合の共通処理 (提案D4)
 * @details ダメージ 2 倍・「ひどい痛手をうけた」メッセージ・思い出フラグ記録を集約。
 */
void apply_monster_element_hurt(CreatureEntity &creature, EffectMonster *em_ptr, MonsterResistanceType hurt_flag)
{
    em_ptr->note = _("はひどい痛手をうけた。", " is hit hard.");
    em_ptr->dam *= 2;
    if (is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
        em_ptr->monrace->r_resistance_flags.set(hurt_flag);
    }
}

/*!
 * @brief player_race 由来の属性耐性による被ダメージ軽減 (提案C1第2弾)
 * @details プレイヤーの部分耐性と同様、被ダメージを約 1/3 に軽減する。
 */
void apply_monster_race_resistance(EffectMonster *em_ptr)
{
    em_ptr->note = _("には耐性がある。", " resists.");
    em_ptr->dam = (em_ptr->dam + 2) / 3;
}

/*!
 * @brief 種族耐性 (C1第3弾) OR-in 付き部分耐性の共通処理 (提案D4第2弾)
 * @param native_resist monrace 固有耐性フラグを持つか (思い出記録は固有耐性時のみ)
 * @details 「耐性がある」メッセージ・ダメージ *3/(1d6+6)・思い出フラグ記録を集約した
 *          部分耐性の単一実装。native_resist が true の時のみ思い出 (r_resistance_flags) を
 *          記録する。二次属性 (chaos / shards / disenchant / nexus) の byte 一致ブロックや、
 *          native 固定の `apply_monster_element_resist` はいずれも本関数へ委譲する。挙動不変。
 */
void apply_monster_element_resist_native(CreatureEntity &creature, EffectMonster *em_ptr, MonsterResistanceType resist_flag, bool native_resist)
{
    em_ptr->note = _("には耐性がある。", " resists.");
    em_ptr->dam *= 3;
    em_ptr->dam /= randint1(6) + 6;
    if (native_resist && is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
        em_ptr->monrace->r_resistance_flags.set(resist_flag);
    }
}

/*!
 * @brief モンスターが属性に (monrace 固有の) 部分耐性を持つ場合の共通処理 (提案D4第2弾)
 * @details 複数ハンドラ (plasma / force / inertia / time 等) に byte 一致で重複していた
 *          部分耐性ブロックの統合。monrace 固有耐性のため native_resist=true 固定で
 *          `apply_monster_element_resist_native` へ委譲する (挙動不変)。
 */
void apply_monster_element_resist(CreatureEntity &creature, EffectMonster *em_ptr, MonsterResistanceType resist_flag)
{
    apply_monster_element_resist_native(creature, em_ptr, resist_flag, true);
}
}

/*!
 * @brief モンスターの耐性がなく、効果もない場合の処理
 * @param em_ptr 魔法効果情報への参照ポインタ
 * @return 効果処理を続ける
 */
ProcessResult effect_monster_nothing(EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_acid(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->monrace->resistance_flags.has(MonsterResistanceType::IMMUNE_ACID)) {
        apply_monster_element_immune(creature, em_ptr, MonsterResistanceType::IMMUNE_ACID);
    } else if (target_race_resists_element(em_ptr, TR_RES_ACID)) {
        apply_monster_race_resistance(em_ptr);
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_elec(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->monrace->resistance_flags.has(MonsterResistanceType::IMMUNE_ELEC)) {
        apply_monster_element_immune(creature, em_ptr, MonsterResistanceType::IMMUNE_ELEC);
    } else if (target_race_resists_element(em_ptr, TR_RES_ELEC)) {
        apply_monster_race_resistance(em_ptr);
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_fire(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->monrace->resistance_flags.has(MonsterResistanceType::IMMUNE_FIRE)) {
        apply_monster_element_immune(creature, em_ptr, MonsterResistanceType::IMMUNE_FIRE);
        return ProcessResult::PROCESS_CONTINUE;
    }

    if (em_ptr->monrace->resistance_flags.has(MonsterResistanceType::HURT_FIRE)) {
        apply_monster_element_hurt(creature, em_ptr, MonsterResistanceType::HURT_FIRE);
    } else if (target_race_resists_element(em_ptr, TR_RES_FIRE)) {
        apply_monster_race_resistance(em_ptr);
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_cold(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->monrace->resistance_flags.has(MonsterResistanceType::IMMUNE_COLD)) {
        apply_monster_element_immune(creature, em_ptr, MonsterResistanceType::IMMUNE_COLD);
        return ProcessResult::PROCESS_CONTINUE;
    }

    if (em_ptr->monrace->resistance_flags.has(MonsterResistanceType::HURT_COLD)) {
        apply_monster_element_hurt(creature, em_ptr, MonsterResistanceType::HURT_COLD);
    } else if (target_race_resists_element(em_ptr, TR_RES_COLD)) {
        apply_monster_race_resistance(em_ptr);
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_pois(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->monrace->resistance_flags.has(MonsterResistanceType::IMMUNE_POISON)) {
        apply_monster_element_immune(creature, em_ptr, MonsterResistanceType::IMMUNE_POISON);
        return ProcessResult::PROCESS_CONTINUE;
    }

    // [提案C1第2弾] player_race 由来の毒耐性を反映 (opt-in・既定OFF)。
    // dam 軽減を D7 の DoT 蓄積 (dam/2) より前に行うことで継続毒も軽減される。
    if (target_race_resists_element(em_ptr, TR_RES_POIS)) {
        apply_monster_race_resistance(em_ptr);
    }

    // [提案D7] suffers_poison_dot が立つ個体は毒攻撃で継続毒 (POISON DoT) を蓄積する。
    // 既定OFFのためバランス不変。蓄積量はダメージの半分で有界 (毎ターン 1 ずつ処理される)。
    if (em_ptr->monrace->suffers_poison_dot && (em_ptr->dam > 0)) {
        const auto added = std::max(1, em_ptr->dam / 2);
        const auto current = em_ptr->m_ptr->get_timed_effect(CreatureTimedEffect::POISON);
        em_ptr->m_ptr->set_timed_effect(CreatureTimedEffect::POISON, static_cast<short>(std::min<int>(MAX_SHORT, current + added)));
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_nuke(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->monrace->resistance_flags.has(MonsterResistanceType::IMMUNE_POISON)) {
        em_ptr->note = _("には耐性がある。", " resists.");
        em_ptr->dam *= 3;
        em_ptr->dam /= randint1(6) + 6;
        if (is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
            em_ptr->monrace->r_resistance_flags.set(MonsterResistanceType::IMMUNE_POISON);
        }

        return ProcessResult::PROCESS_CONTINUE;
    }

    if (one_in_(3)) {
        em_ptr->do_polymorph = true;
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_hell_fire(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->monrace->kind_flags.has_not(MonsterKindType::GOOD)) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    em_ptr->note = _("はひどい痛手をうけた。", " is hit hard.");
    em_ptr->dam *= 2;
    if (is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
        em_ptr->monrace->r_kind_flags.set(MonsterKindType::GOOD);
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_holy_fire(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->monrace->kind_flags.has(MonsterKindType::GOOD)) {
        em_ptr->note = _("には完全な耐性がある！", " is immune.");
        em_ptr->dam = 0;
        if (is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
            em_ptr->monrace->r_kind_flags.set(MonsterKindType::GOOD);
        }
        return ProcessResult::PROCESS_CONTINUE;
    }

    if (em_ptr->monrace->kind_flags.has(MonsterKindType::EVIL)) {
        em_ptr->dam *= 2;
        em_ptr->note = _("はひどい痛手をうけた。", " is hit hard.");
        if (is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
            em_ptr->monrace->r_kind_flags.set(MonsterKindType::EVIL);
        }
        return ProcessResult::PROCESS_CONTINUE;
    }

    em_ptr->note = _("には耐性がある。", " resists.");
    em_ptr->dam *= 3;
    em_ptr->dam /= randint1(6) + 6;

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_plasma(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->monrace->resistance_flags.has_not(MonsterResistanceType::RESIST_PLASMA)) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    apply_monster_element_resist(creature, em_ptr, MonsterResistanceType::RESIST_PLASMA);
    return ProcessResult::PROCESS_CONTINUE;
}

static bool effect_monster_nether_resist(CreatureEntity &creature, EffectMonster *em_ptr)
{
    const auto native_resist = em_ptr->monrace->resistance_flags.has(MonsterResistanceType::RESIST_NETHER);
    if (!native_resist && !target_race_resists_element(em_ptr, TR_RES_NETHER)) {
        return false;
    }

    if (em_ptr->monrace->kind_flags.has(MonsterKindType::UNDEAD)) {
        em_ptr->note = _("には完全な耐性がある！", " is immune.");
        em_ptr->dam = 0;
        if (is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
            em_ptr->monrace->r_kind_flags.set(MonsterKindType::UNDEAD);
        }
    } else {
        em_ptr->note = _("には耐性がある。", " resists.");
        em_ptr->dam *= 3;
        em_ptr->dam /= randint1(6) + 6;
    }

    if (native_resist && is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
        em_ptr->monrace->r_resistance_flags.set(MonsterResistanceType::RESIST_NETHER);
    }

    return true;
}

ProcessResult effect_monster_nether(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (effect_monster_nether_resist(creature, em_ptr) || (em_ptr->monrace->kind_flags.has_not(MonsterKindType::EVIL))) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    em_ptr->note = _("はいくらか耐性を示した。", " resists somewhat.");
    em_ptr->dam /= 2;
    if (is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
        em_ptr->monrace->r_kind_flags.set(MonsterKindType::EVIL);
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_water(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->monrace->resistance_flags.has_not(MonsterResistanceType::RESIST_WATER)) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    if ((em_ptr->m_ptr->get_r_idx() == MonraceId::WATER_ELEM) || (em_ptr->m_ptr->get_r_idx() == MonraceId::UNMAKER)) {
        em_ptr->note = _("には完全な耐性がある！", " is immune.");
        em_ptr->dam = 0;
    } else {
        em_ptr->note = _("には耐性がある。", " resists.");
        em_ptr->dam *= 3;
        em_ptr->dam /= randint1(6) + 6;
    }

    if (is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
        em_ptr->monrace->r_resistance_flags.set(MonsterResistanceType::RESIST_WATER);
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_chaos(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    const auto native_chaos_resist = em_ptr->monrace->resistance_flags.has(MonsterResistanceType::RESIST_CHAOS);
    if (native_chaos_resist || target_race_resists_element(em_ptr, TR_RES_CHAOS)) {
        apply_monster_element_resist_native(creature, em_ptr, MonsterResistanceType::RESIST_CHAOS, native_chaos_resist);
    } else if (em_ptr->monrace->kind_flags.has(MonsterKindType::DEMON) && one_in_(3)) {
        em_ptr->note = _("はいくらか耐性を示した。", " resists somewhat.");
        em_ptr->dam *= 3;
        em_ptr->dam /= randint1(6) + 6;
        if (is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
            em_ptr->monrace->r_kind_flags.set(MonsterKindType::DEMON);
        }
    } else {
        em_ptr->do_polymorph = true;
        em_ptr->do_conf = (5 + randint1(11) + em_ptr->r) / (em_ptr->r + 1);
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_shards(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    const auto native_resist = em_ptr->monrace->resistance_flags.has(MonsterResistanceType::RESIST_SHARDS);
    if (!native_resist && !target_race_resists_element(em_ptr, TR_RES_SHARDS)) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    apply_monster_element_resist_native(creature, em_ptr, MonsterResistanceType::RESIST_SHARDS, native_resist);
    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_rocket(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    const auto native_resist = em_ptr->monrace->resistance_flags.has(MonsterResistanceType::RESIST_SHARDS);
    if (!native_resist && !target_race_resists_element(em_ptr, TR_RES_SHARDS)) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    em_ptr->note = _("はいくらか耐性を示した。", " resists somewhat.");
    em_ptr->dam /= 2;
    if (native_resist && is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
        em_ptr->monrace->r_resistance_flags.set(MonsterResistanceType::RESIST_SHARDS);
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_sound(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    const auto native_resist = em_ptr->monrace->resistance_flags.has(MonsterResistanceType::RESIST_SOUND);
    if (!native_resist && !target_race_resists_element(em_ptr, TR_RES_SOUND)) {
        em_ptr->do_stun = (10 + randint1(15) + em_ptr->r) / (em_ptr->r + 1);
        return ProcessResult::PROCESS_CONTINUE;
    }

    em_ptr->note = _("には耐性がある。", " resists.");
    em_ptr->dam *= 2;
    em_ptr->dam /= randint1(6) + 6;
    if (native_resist && is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
        em_ptr->monrace->r_resistance_flags.set(MonsterResistanceType::RESIST_SOUND);
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_confusion(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    const auto native_resist = em_ptr->monrace->resistance_flags.has(MonsterResistanceType::NO_CONF);
    if (!native_resist && !target_race_resists_element(em_ptr, TR_RES_CONF)) {
        em_ptr->do_conf = (10 + randint1(15) + em_ptr->r) / (em_ptr->r + 1);
        return ProcessResult::PROCESS_CONTINUE;
    }

    em_ptr->note = _("には耐性がある。", " resists.");
    em_ptr->dam *= 3;
    em_ptr->dam /= randint1(6) + 6;
    if (native_resist && is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
        em_ptr->monrace->resistance_flags.set(MonsterResistanceType::NO_CONF);
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_disenchant(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    const auto native_resist = em_ptr->monrace->resistance_flags.has(MonsterResistanceType::RESIST_DISENCHANT);
    if (!native_resist && !target_race_resists_element(em_ptr, TR_RES_DISEN)) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    apply_monster_element_resist_native(creature, em_ptr, MonsterResistanceType::RESIST_DISENCHANT, native_resist);
    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_nexus(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    const auto native_resist = em_ptr->monrace->resistance_flags.has(MonsterResistanceType::RESIST_NEXUS);
    if (!native_resist && !target_race_resists_element(em_ptr, TR_RES_NEXUS)) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    apply_monster_element_resist_native(creature, em_ptr, MonsterResistanceType::RESIST_NEXUS, native_resist);
    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_force(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->monrace->resistance_flags.has_not(MonsterResistanceType::RESIST_FORCE)) {
        em_ptr->do_stun = (randint1(15) + em_ptr->r) / (em_ptr->r + 1);
        return ProcessResult::PROCESS_CONTINUE;
    }

    apply_monster_element_resist(creature, em_ptr, MonsterResistanceType::RESIST_FORCE);
    return ProcessResult::PROCESS_CONTINUE;
}

// Powerful monsters can resists and normal monsters slow down.
ProcessResult effect_monster_inertial(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->monrace->resistance_flags.has(MonsterResistanceType::RESIST_INERTIA)) {
        apply_monster_element_resist(creature, em_ptr, MonsterResistanceType::RESIST_INERTIA);
        return ProcessResult::PROCESS_CONTINUE;
    }

    bool cancel_effect = em_ptr->monrace->kind_flags.has(MonsterKindType::UNIQUE);
    cancel_effect |= (em_ptr->monrace->level > randint1(std::max(1, em_ptr->dam - 10)) + 10);
    if (cancel_effect) {
        em_ptr->obvious = false;
        return ProcessResult::PROCESS_CONTINUE;
    }

    if (set_monster_slow(*creature.get_floor(), em_ptr->g_ptr->m_idx, em_ptr->m_ptr->get_remaining_deceleration() + 50)) {
        em_ptr->note = _("の動きが遅くなった。", " starts moving slower.");
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_time(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->monrace->resistance_flags.has_not(MonsterResistanceType::RESIST_TIME)) {
        em_ptr->do_time = (em_ptr->dam + 1) / 2;
        return ProcessResult::PROCESS_CONTINUE;
    }

    apply_monster_element_resist(creature, em_ptr, MonsterResistanceType::RESIST_TIME);
    return ProcessResult::PROCESS_CONTINUE;
}

static bool effect_monster_gravity_resist_teleport(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (em_ptr->monrace->resistance_flags.has_not(MonsterResistanceType::RESIST_TELEPORT)) {
        em_ptr->obvious = true;
        return false;
    }

    if (em_ptr->monrace->kind_flags.has(MonsterKindType::UNIQUE)) {
        if (is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
            em_ptr->monrace->r_resistance_flags.set(MonsterResistanceType::RESIST_TELEPORT);
        }

        em_ptr->note = _("には効果がなかった。", " is unaffected!");
        return true;
    }

    if (em_ptr->monrace->level <= randint1(100)) {
        em_ptr->obvious = true;
        return false;
    }

    if (is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
        em_ptr->monrace->r_resistance_flags.set(MonsterResistanceType::RESIST_TELEPORT);
    }

    em_ptr->note = _("には耐性がある！", " resists!");
    return true;
}

static void effect_monster_gravity_slow(CreatureEntity &creature, EffectMonster *em_ptr)
{
    bool cancel_effect = em_ptr->monrace->kind_flags.has(MonsterKindType::UNIQUE);
    cancel_effect |= (em_ptr->monrace->level > randint1(std::max(1, em_ptr->dam - 10)) + 10);
    if (cancel_effect) {
        em_ptr->obvious = false;
        return;
    }

    if (set_monster_slow(*creature.get_floor(), em_ptr->g_ptr->m_idx, em_ptr->m_ptr->get_remaining_deceleration() + 50)) {
        em_ptr->note = _("の動きが遅くなった。", " starts moving slower.");
    }
    em_ptr->obvious = true;
}

static void effect_monster_gravity_stun(EffectMonster *em_ptr)
{
    em_ptr->do_stun = Dice::roll((em_ptr->caster_lev / 20) + 3, (em_ptr->dam)) + 1;
    bool has_resistance = em_ptr->monrace->kind_flags.has(MonsterKindType::UNIQUE);
    has_resistance |= (em_ptr->monrace->level > randint1(std::max(1, em_ptr->dam - 10)) + 10);
    has_resistance |= em_ptr->monrace->resistance_flags.has(MonsterResistanceType::RESIST_GRAVITY);
    if (has_resistance) {
        em_ptr->do_stun = 0;
        return;
    }
    em_ptr->obvious = true;
}

/*
 * Powerful monsters can resist and normal monsters slow down
 * Furthermore, this magic can make non-unique monsters slow/stun.
 */
ProcessResult effect_monster_gravity(CreatureEntity &creature, EffectMonster *em_ptr)
{
    em_ptr->do_dist = effect_monster_gravity_resist_teleport(creature, em_ptr) ? 0 : 10;
    if (em_ptr->m_ptr->is_riding()) {
        em_ptr->do_dist = 0;
    }

    if (em_ptr->monrace->resistance_flags.has(MonsterResistanceType::RESIST_GRAVITY)) {
        em_ptr->note = _("には耐性がある！", " resists!");
        em_ptr->dam *= 3;
        em_ptr->dam /= randint1(6) + 6;
        em_ptr->do_dist = 0;
        if (is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
            em_ptr->monrace->r_resistance_flags.set(MonsterResistanceType::RESIST_GRAVITY);
        }
        return ProcessResult::PROCESS_CONTINUE;
    }

    em_ptr->note = _("には効果がなかった。", " is unaffected!");

    effect_monster_gravity_slow(creature, em_ptr);
    effect_monster_gravity_stun(em_ptr);
    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_disintegration(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->monrace->resistance_flags.has_not(MonsterResistanceType::HURT_ROCK)) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    if (is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
        em_ptr->monrace->r_resistance_flags.set(MonsterResistanceType::HURT_ROCK);
    }

    em_ptr->note = _("の皮膚がただれた！", " loses some skin!");
    em_ptr->note_dies = _("は蒸発した！", " evaporates!");
    em_ptr->dam *= 2;
    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_icee_bolt(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    // [提案C1第7弾] 音耐性 (轟音付随のスタン) を種族の TR_RES_SOUND でも無効化。
    if (em_ptr->monrace->resistance_flags.has_not(MonsterResistanceType::RESIST_SOUND) && !target_race_resists_element(em_ptr, TR_RES_SOUND)) {
        em_ptr->do_stun = (randint1(15) + 1) / (em_ptr->r + 1);
    }

    if (em_ptr->monrace->resistance_flags.has(MonsterResistanceType::IMMUNE_COLD)) {
        apply_monster_element_immune(creature, em_ptr, MonsterResistanceType::IMMUNE_COLD);
    } else if (em_ptr->monrace->resistance_flags.has(MonsterResistanceType::HURT_COLD)) {
        apply_monster_element_hurt(creature, em_ptr, MonsterResistanceType::HURT_COLD);
    } else if (target_race_resists_element(em_ptr, TR_RES_COLD)) {
        // [提案C1第7弾] 冷気ダメージも種族の冷気耐性で軽減 (effect_monster_cold と同水準)。
        apply_monster_race_resistance(em_ptr);
    }

    return ProcessResult::PROCESS_CONTINUE;
}

/*!
 * @brief 虚無属性の耐性と効果の発動
 * @param creature クリーチャーへの参照
 * @em_ptr 魔法効果情報への参照ポインタ
 * @return 効果処理を続けるかどうか
 * @details
 * 量子生物に倍打、壁抜けに1.5倍打、テレポート耐性が耐性
 */
ProcessResult effect_monster_void(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->monrace->kind_flags.has(MonsterKindType::QUANTUM)) {
        em_ptr->note = _("の存在確率が減少した。", "'s wave function is reduced.");
        em_ptr->note_dies = _("は観測されなくなった。", "'s wave function collapses.");
        em_ptr->dam *= 2;
        if (is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
            em_ptr->monrace->r_kind_flags.set(MonsterKindType::QUANTUM);
        }
    } else if (em_ptr->monrace->feature_flags.has(MonsterFeatureType::PASS_WALL)) {
        em_ptr->note = _("の存在が薄れていく。", " is fading out.");
        em_ptr->note_dies = _("は消えてしまった。", " has disappeared.");
        em_ptr->dam *= 3;
        em_ptr->dam /= 2;
        if (is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
            em_ptr->monrace->r_feature_flags.set(MonsterFeatureType::PASS_WALL);
        }
    } else if (em_ptr->monrace->resistance_flags.has_any_of({ MonsterResistanceType::RESIST_TELEPORT, MonsterResistanceType::RESIST_FORCE, MonsterResistanceType::RESIST_GRAVITY })) {
        em_ptr->note = _("には耐性がある！", " resists!");
        em_ptr->dam *= 3;
        em_ptr->dam /= randint1(6) + 6;
        if (is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
            if (em_ptr->monrace->resistance_flags.has(MonsterResistanceType::RESIST_TELEPORT)) {
                em_ptr->monrace->r_resistance_flags.set(MonsterResistanceType::RESIST_TELEPORT);
            }
            if (em_ptr->monrace->resistance_flags.has(MonsterResistanceType::RESIST_FORCE)) {
                em_ptr->monrace->r_resistance_flags.set(MonsterResistanceType::RESIST_FORCE);
            }
            if (em_ptr->monrace->resistance_flags.has(MonsterResistanceType::RESIST_GRAVITY)) {
                em_ptr->monrace->r_resistance_flags.set(MonsterResistanceType::RESIST_GRAVITY);
            }
        }
    } else {
        em_ptr->note_dies = _("は消滅してしまった。", " has vanished.");
    }

    return ProcessResult::PROCESS_CONTINUE;
}

/*!
 * @brief 深淵属性の耐性と効果の発動
 * @param creature クリーチャーへの参照
 * @em_ptr 魔法効果情報への参照ポインタ
 * @return 効果処理を続けるかどうか
 * @details
 * 飛ばないテレポート耐性に1.25倍打、暗黒耐性が耐性
 * 1/3で追加に混乱か恐怖
 */
ProcessResult effect_monster_abyss(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    auto dark = { MonsterBrightnessType::SELF_DARK_1, MonsterBrightnessType::SELF_DARK_2, MonsterBrightnessType::HAS_DARK_1, MonsterBrightnessType::HAS_DARK_2 };

    if (em_ptr->monrace->brightness_flags.has_any_of(dark)) {
        em_ptr->note = _("には耐性がある！", " resists!");
        em_ptr->dam *= 3;
        em_ptr->dam /= (randint1(6) + 6);
    } else if (em_ptr->monrace->resistance_flags.has(MonsterResistanceType::RESIST_DARK)) {
        em_ptr->note = _("には耐性がある！", " resists!");
        em_ptr->dam *= 3;
        em_ptr->dam /= (randint1(4) + 5);
        if (is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
            em_ptr->monrace->r_resistance_flags.set(MonsterResistanceType::RESIST_DARK);
        }
    } else if (em_ptr->monrace->feature_flags.has_not(MonsterFeatureType::CAN_FLY)) {
        if (em_ptr->monrace->resistance_flags.has(MonsterResistanceType::RESIST_TELEPORT)) {
            em_ptr->dam *= 5;
            em_ptr->dam /= 4;
            if (is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
                em_ptr->monrace->r_resistance_flags.set(MonsterResistanceType::RESIST_TELEPORT);
            }
        }

        em_ptr->note = _("は深淵に囚われていく。", " is trapped in the abyss.");
        em_ptr->note_dies = _("は深淵に堕ちてしまった。", " has fallen into the abyss.");

        if (one_in_(3) && set_monster_slow(*creature.get_floor(), em_ptr->g_ptr->m_idx, em_ptr->m_ptr->get_remaining_deceleration() + 50)) {
            em_ptr->note = _("の動きが遅くなった。", " starts moving slower.");
        }
    }

    if (em_ptr->monrace->misc_flags.has(MonsterMiscType::ELDRITCH_HORROR) || em_ptr->monrace->misc_flags.has(MonsterMiscType::EMPTY_MIND)) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    if (one_in_(3)) {
        if (one_in_(2)) {
            em_ptr->do_conf = (10 + randint1(15) + em_ptr->r) / (em_ptr->r + 1);
        } else {
            em_ptr->do_fear = (10 + randint1(15) + em_ptr->r) / (em_ptr->r + 1);
        }
    }

    return ProcessResult::PROCESS_CONTINUE;
}

/*!
 * @brief 汚物効果の耐性と効果の発動
 * @param creature クリーチャーへの参照
 * @em_ptr 魔法効果情報への参照ポインタ
 * @return 効果処理を続けるかどうか
 * @details
 * 糞便のブレス：毒耐性で軽減、汚物で特効ダメージ
 */
ProcessResult effect_monster_dirt(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->monrace->misc_flags.has(MonsterMiscType::SCATOLOGIST)) {
        em_ptr->note = _("には完全な耐性がある！", " resists completely.");
        em_ptr->dam = 0;
        if (is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
            em_ptr->monrace->r_misc_flags.set(MonsterMiscType::SCATOLOGIST);
        }
        return ProcessResult::PROCESS_CONTINUE;
    }

    // 毒完全耐性があるモンスターは大幅に軽減
    if (em_ptr->monrace->resistance_flags.has(MonsterResistanceType::IMMUNE_POISON)) {
        apply_monster_element_immune(creature, em_ptr, MonsterResistanceType::IMMUNE_POISON);
        return ProcessResult::PROCESS_CONTINUE;
    }

    // 清浄なモンスター（天使、エレメンタルなど）は追加ダメージ
    bool is_vulnerable = false;
    if (em_ptr->monrace->kind_flags.has(MonsterKindType::ANGEL)) {
        is_vulnerable = true;
        if (is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
            em_ptr->monrace->r_kind_flags.set(MonsterKindType::ANGEL);
        }
    }

    if (is_vulnerable) {
        em_ptr->note = _("は汚物にまみれて苦しんでいる！", " writhes in agony from the filth!");
        em_ptr->dam *= 2;
    }

    // 一定確率で状態異常付与
    if (randint0(100) < 30) {
        if (em_ptr->monrace->resistance_flags.has_not(MonsterResistanceType::NO_CONF)) {
            em_ptr->do_conf = (10 + randint1(15));
            if (em_ptr->note.empty()) {
                em_ptr->note = _("は混乱したようだ。", " looks confused.");
            }
        }
    }

    if (randint0(100) < 20) {
        if (em_ptr->monrace->resistance_flags.has_not(MonsterResistanceType::NO_STUN)) {
            em_ptr->do_stun = (5 + randint1(10));
            if (em_ptr->note.empty()) {
                em_ptr->note = _("はよろめいている。", " is stunned.");
            }
        }
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_spider_string(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    // 蜘蛛型モンスターは蜘蛛糸に完全耐性
    if (em_ptr->monrace->kind_flags.has(MonsterKindType::SPIDER)) {
        em_ptr->note = _("には完全な耐性がある！", " resists completely.");
        em_ptr->dam = 0;
        if (is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
            em_ptr->monrace->r_kind_flags.set(MonsterKindType::SPIDER);
        }
        return ProcessResult::PROCESS_CONTINUE;
    }

    // 無機物系モンスターは糸が効かない
    bool is_vulnerable = true;
    if (em_ptr->monrace->kind_flags.has(MonsterKindType::ELEMENTAL) ||
        em_ptr->monrace->kind_flags.has(MonsterKindType::NONLIVING)) {
        em_ptr->note = _("にはあまり効果がない。", " is not affected much.");
        em_ptr->dam /= 3;
        is_vulnerable = false;
    }

    // 動物系・人間系は特に蜘蛛糸に弱い
    if (em_ptr->monrace->kind_flags.has(MonsterKindType::ANIMAL) ||
        em_ptr->monrace->kind_flags.has(MonsterKindType::HUMAN)) {
        is_vulnerable = true;
        if (is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
            if (em_ptr->monrace->kind_flags.has(MonsterKindType::ANIMAL)) {
                em_ptr->monrace->r_kind_flags.set(MonsterKindType::ANIMAL);
            }
            if (em_ptr->monrace->kind_flags.has(MonsterKindType::HUMAN)) {
                em_ptr->monrace->r_kind_flags.set(MonsterKindType::HUMAN);
            }
        }
    }

    if (is_vulnerable) {
        em_ptr->note = _("は蜘蛛糸に絡まって身動きが取れない！", " is entangled in spider webs!");
        em_ptr->dam = std::max(em_ptr->dam, 1); // 最低1ダメージは与える
    }

    // 蜘蛛糸効果で行動阻害（スロウ効果）
    /* TODO
    if (randint0(100) < 50) {
        if (em_ptr->monrace->resistance_flags.has_not(MonsterResistanceType::NO_SLOW)) {
            em_ptr->do_slow = (10 + randint1(20));
            if (em_ptr->note.empty()) {
                em_ptr->note = _("の動きが鈍くなった。", " moves slower.");
            }
        }
    }
    */

    // 高確率で混乱（糸に絡まってパニック）
    if (randint0(100) < 40) {
        if (em_ptr->monrace->resistance_flags.has_not(MonsterResistanceType::NO_CONF)) {
            em_ptr->do_conf = (5 + randint1(10));
            if (em_ptr->note.empty()) {
                em_ptr->note = _("は混乱したようだ。", " looks confused.");
            }
        }
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_stungun(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->monrace->resistance_flags.has(MonsterResistanceType::IMMUNE_ELEC)) {
        apply_monster_element_immune(creature, em_ptr, MonsterResistanceType::IMMUNE_ELEC);
    }

    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_meteor(CreatureEntity &creature, EffectMonster *em_ptr)
{
    if (em_ptr->monrace->resistance_flags.has_not(MonsterResistanceType::RESIST_METEOR)) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    em_ptr->note = _("には耐性がある！", " resists!");
    em_ptr->dam *= 3;
    em_ptr->dam /= randint1(6) + 6;
    if (is_original_ap_and_seen(creature, *em_ptr->m_ptr)) {
        em_ptr->monrace->r_resistance_flags.set(MonsterResistanceType::RESIST_METEOR);
    }

    return ProcessResult::PROCESS_CONTINUE;
}
