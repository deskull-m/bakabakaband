#include "effect/effect-monster-oldies.h"
#include "avatar/avatar.h"
#include "effect/effect-monster-util.h"
#include "monster-floor/monster-generator.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "monster/monster-util.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "tracking/health-bar-tracker.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

// Powerful monsters can resist.
ProcessResult effect_monster_old_poly(EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }
    em_ptr->do_polymorph = true;

    bool has_resistance = em_ptr->r_ptr->kind_flags.has(MonsterKindType::UNIQUE);
    has_resistance |= em_ptr->r_ptr->misc_flags.has(MonsterMiscType::QUESTOR);
    has_resistance |= (em_ptr->r_ptr->level > randint1(std::max(1, em_ptr->dam - 10)) + 10);

    if (has_resistance) {
        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->do_polymorph = false;
        em_ptr->obvious = false;
    }

    em_ptr->dam = 0;
    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_old_clone(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    auto has_resistance = (player_ptr->current_floor_ptr->inside_arena);
    has_resistance |= em_ptr->m_ptr->is_pet();
    has_resistance |= em_ptr->r_ptr->kind_flags.has(MonsterKindType::UNIQUE);
    has_resistance |= em_ptr->r_ptr->misc_flags.has(MonsterMiscType::QUESTOR);
    has_resistance |= em_ptr->r_ptr->population_flags.has_any_of({ MonsterPopulationType::NAZGUL, MonsterPopulationType::ONLY_ONE, MonsterPopulationType::BUNBUN_STRIKER });

    if (has_resistance) {
        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->dam = 0;
        return ProcessResult::PROCESS_CONTINUE;
    }

    em_ptr->m_ptr->hp = em_ptr->m_ptr->maxhp;
    if (multiply_monster(player_ptr, em_ptr->g_ptr->m_idx, em_ptr->m_ptr->r_idx, true, 0L)) {
        em_ptr->note = _("が分裂した！", " spawns!");
    }

    em_ptr->dam = 0;
    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_star_heal(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    (void)set_monster_csleep(player_ptr, em_ptr->g_ptr->m_idx, 0);

    if (em_ptr->m_ptr->maxhp < em_ptr->m_ptr->max_maxhp) {
        if (em_ptr->seen_msg) {
            msg_format(_("%s^の強さが戻った。", "%s^ recovers %s vitality."), em_ptr->m_name, em_ptr->m_poss);
        }
        em_ptr->m_ptr->maxhp = em_ptr->m_ptr->max_maxhp;
    }

    if (!em_ptr->dam) {
        HealthBarTracker::get_instance().set_flag_if_tracking(em_ptr->g_ptr->m_idx);
        if (em_ptr->m_ptr->is_riding()) {
            RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::UHEALTH);
        }

        return ProcessResult::PROCESS_FALSE;
    }

    effect_monster_old_heal(player_ptr, em_ptr);
    return ProcessResult::PROCESS_TRUE;
}

// who == 0ならばプレイヤーなので、それの判定.
static void effect_monster_old_heal_check_player(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->is_monster()) {
        return;
    }

    chg_virtue(player_ptr, Virtue::VITALITY, 1);
    if (em_ptr->r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
        chg_virtue(player_ptr, Virtue::INDIVIDUALISM, 1);
    }

    if (em_ptr->m_ptr->is_friendly()) {
        chg_virtue(player_ptr, Virtue::HONOUR, 1);
    } else if (em_ptr->r_ptr->kind_flags.has_not(MonsterKindType::EVIL)) {
        if (em_ptr->r_ptr->kind_flags.has(MonsterKindType::GOOD)) {
            chg_virtue(player_ptr, Virtue::COMPASSION, 2);
        } else {
            chg_virtue(player_ptr, Virtue::COMPASSION, 1);
        }
    }

    if (em_ptr->r_ptr->kind_flags.has(MonsterKindType::ANIMAL)) {
        chg_virtue(player_ptr, Virtue::NATURE, 1);
    }
}

static void effect_monster_old_heal_recovery(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->m_ptr->get_remaining_stun()) {
        if (em_ptr->seen_msg) {
            msg_format(_("%s^は朦朧状態から立ち直った。", "%s^ is no longer stunned."), em_ptr->m_name);
        }

        (void)set_monster_stunned(player_ptr, em_ptr->g_ptr->m_idx, 0);
    }

    if (em_ptr->m_ptr->is_confused()) {
        if (em_ptr->seen_msg) {
            msg_format(_("%s^は混乱から立ち直った。", "%s^ is no longer confused."), em_ptr->m_name);
        }

        (void)set_monster_confused(player_ptr, em_ptr->g_ptr->m_idx, 0);
    }

    if (em_ptr->m_ptr->is_fearful()) {
        if (em_ptr->seen_msg) {
            msg_format(_("%s^は勇気を取り戻した。", "%s^ recovers %s courage."), em_ptr->m_name, em_ptr->m_poss);
        }

        (void)set_monster_monfear(player_ptr, em_ptr->g_ptr->m_idx, 0);
    }
}

ProcessResult effect_monster_old_heal(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    /* Wake up */
    (void)set_monster_csleep(player_ptr, em_ptr->g_ptr->m_idx, 0);
    effect_monster_old_heal_recovery(player_ptr, em_ptr);
    if (em_ptr->m_ptr->hp < MONSTER_MAXHP) {
        em_ptr->m_ptr->hp += em_ptr->dam;
    }
    if (em_ptr->m_ptr->hp > em_ptr->m_ptr->maxhp) {
        em_ptr->m_ptr->hp = em_ptr->m_ptr->maxhp;
    }

    effect_monster_old_heal_check_player(player_ptr, em_ptr);
    if (em_ptr->m_ptr->r_idx == MonraceId::LEPER) {
        em_ptr->heal_leper = true;
        if (em_ptr->is_player()) {
            chg_virtue(player_ptr, Virtue::COMPASSION, 5);
        }
    }

    HealthBarTracker::get_instance().set_flag_if_tracking(em_ptr->g_ptr->m_idx);
    if (em_ptr->m_ptr->is_riding()) {
        RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::UHEALTH);
    }

    em_ptr->note = _("は体力を回復したようだ。", " looks healthier.");
    em_ptr->dam = 0;
    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_old_speed(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (set_monster_fast(player_ptr, em_ptr->g_ptr->m_idx, em_ptr->m_ptr->get_remaining_acceleration() + 100)) {
        em_ptr->note = _("の動きが速くなった。", " starts moving faster.");
    }

    if (em_ptr->is_player()) {
        if (em_ptr->r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
            chg_virtue(player_ptr, Virtue::INDIVIDUALISM, 1);
        }
        if (em_ptr->m_ptr->is_friendly()) {
            chg_virtue(player_ptr, Virtue::HONOUR, 1);
        }
    }

    em_ptr->dam = 0;
    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_old_slow(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    bool has_resistance = em_ptr->r_ptr->kind_flags.has(MonsterKindType::UNIQUE);
    has_resistance |= (em_ptr->r_ptr->level > randint1(std::max(1, em_ptr->dam - 10)) + 10);

    /* Powerful monsters can resist */
    if (has_resistance) {
        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->obvious = false;
        em_ptr->dam = 0;
        return ProcessResult::PROCESS_CONTINUE;
    }

    if (set_monster_slow(player_ptr, em_ptr->g_ptr->m_idx, em_ptr->m_ptr->get_remaining_deceleration() + 50)) {
        em_ptr->note = _("の動きが遅くなった。", " starts moving slower.");
    }

    em_ptr->dam = 0;
    return ProcessResult::PROCESS_CONTINUE;
}

/*!
 * @todo 「ユニークは (魔法では)常に眠らない」はMonsterRaceDefinitionの趣旨に反すると思われる
 * 眠る確率を半分にするとかしておいた方が良さそう
 */
ProcessResult effect_monster_old_sleep(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    bool has_resistance = em_ptr->r_ptr->kind_flags.has(MonsterKindType::UNIQUE);
    has_resistance |= em_ptr->r_ptr->resistance_flags.has(MonsterResistanceType::NO_SLEEP);
    has_resistance |= (em_ptr->r_ptr->level > randint1(std::max(1, em_ptr->dam - 10)) + 10);

    if (has_resistance) {
        if (em_ptr->r_ptr->resistance_flags.has(MonsterResistanceType::NO_SLEEP)) {
            if (is_original_ap_and_seen(player_ptr, *em_ptr->m_ptr)) {
                em_ptr->r_ptr->resistance_flags.set(MonsterResistanceType::NO_SLEEP);
            }
        }

        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->obvious = false;
    } else {
        em_ptr->note = _("は眠り込んでしまった！", " falls asleep!");
        em_ptr->do_sleep = 500;
    }

    em_ptr->dam = 0;
    return ProcessResult::PROCESS_CONTINUE;
}

/*!
 * @todo 「ユニークは (魔法では)常に混乱しない」はMonsterRaceDefinitionの趣旨に反すると思われる
 * 眠る確率を半分にするとかしておいた方が良さそう
 */
ProcessResult effect_monster_old_conf(PlayerType *player_ptr, EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    em_ptr->do_conf = Dice::roll(3, (em_ptr->dam / 2)) + 1;

    bool has_resistance = em_ptr->r_ptr->kind_flags.has(MonsterKindType::UNIQUE);
    has_resistance |= em_ptr->r_ptr->resistance_flags.has(MonsterResistanceType::NO_CONF);
    has_resistance |= (em_ptr->r_ptr->level > randint1(std::max(1, em_ptr->dam - 10)) + 10);
    if (has_resistance) {
        if (em_ptr->r_ptr->resistance_flags.has(MonsterResistanceType::NO_CONF)) {
            if (is_original_ap_and_seen(player_ptr, *em_ptr->m_ptr)) {
                em_ptr->r_ptr->resistance_flags.set(MonsterResistanceType::NO_CONF);
            }
        }

        em_ptr->do_conf = 0;
        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->obvious = false;
    }

    em_ptr->dam = 0;
    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_stasis(EffectMonster *em_ptr, bool to_evil)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    int stasis_damage = (em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10);
    bool has_resistance = em_ptr->r_ptr->kind_flags.has(MonsterKindType::UNIQUE);
    has_resistance |= em_ptr->r_ptr->level > randint1(stasis_damage) + 10;
    if (to_evil) {
        has_resistance |= em_ptr->r_ptr->kind_flags.has_not(MonsterKindType::EVIL);
    }

    if (has_resistance) {
        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->obvious = false;
    } else {
        em_ptr->note = _("は動けなくなった！", " is suspended!");
        em_ptr->do_sleep = 500;
    }

    em_ptr->dam = 0;
    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_stun(EffectMonster *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    em_ptr->do_stun = Dice::roll((em_ptr->caster_lev / 20) + 3, (em_ptr->dam)) + 1;

    bool has_resistance = em_ptr->r_ptr->kind_flags.has(MonsterKindType::UNIQUE);
    has_resistance |= (em_ptr->r_ptr->level > randint1(std::max(1, em_ptr->dam - 10)) + 10);
    if (has_resistance) {
        em_ptr->do_stun = 0;
        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->obvious = false;
    }

    em_ptr->dam = 0;
    return ProcessResult::PROCESS_CONTINUE;
}
