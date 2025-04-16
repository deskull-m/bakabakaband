/*!
 * @brief モンスターが詠唱する魔法を選択する処理
 * @date 2020/07/23
 * @author Hourier
 * @details ba-no-ru/rupa-toの特殊処理はここで実施
 */

#include "mspell/mspell-selector.h"
#include "monster-race/race-ability-mask.h"
#include "monster/monster-status.h"
#include "mspell/mspell-attack-util.h"
#include "mspell/mspell-judgement.h"
#include "player/player-status.h"
#include "system/angband-system.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "util/enum-converter.h"
#include "world/world.h"

/*!
 * @brief 指定したID値が指定した範囲内のIDかどうかを返す
 *
 * enum値に対して範囲で判定するのはあまり好ましくないが、歴史的経緯により仕方がない
 *
 * @param spell 判定対象のID
 * @param start 範囲の開始ID
 * @param end 範囲の終了ID(このIDも含む)
 * @return IDが start <= spell <= end なら true、そうでなければ false
 */
static bool spell_in_between(MonsterAbilityType spell, MonsterAbilityType start, MonsterAbilityType end)
{
    auto spell_int = enum2i(spell);
    return enum2i(start) <= spell_int && spell_int <= enum2i(end);
}

/*!
 * @brief ID値が攻撃魔法のIDかどうかを返す /
 * Return TRUE if a spell is good for hurting the player (directly).
 * @param spell 判定対象のID
 * @return 正しいIDならばTRUEを返す。
 */
static bool spell_attack(MonsterAbilityType spell)
{
    /* All RF4 spells hurt (except for shriek and dispel) */
    if (spell_in_between(spell, MonsterAbilityType::ROCKET, MonsterAbilityType::BR_DISI)) {
        return true;
    }
    if (spell_in_between(spell, MonsterAbilityType::BR_VOID, MonsterAbilityType::BR_ABYSS)) {
        return true;
    }

    /* Various "ball" spells */
    if (spell_in_between(spell, MonsterAbilityType::BA_ACID, MonsterAbilityType::BA_DARK)) {
        return true;
    }
    if (spell_in_between(spell, MonsterAbilityType::BA_VOID, MonsterAbilityType::BA_ABYSS)) {
        return true;
    }

    /* "Cause wounds" and "bolt" spells */
    if (spell_in_between(spell, MonsterAbilityType::CAUSE_1, MonsterAbilityType::MISSILE)) {
        return true;
    }
    if (spell_in_between(spell, MonsterAbilityType::BO_VOID, MonsterAbilityType::BO_METEOR)) {
        return true;
    }

    /* Hand of Doom */
    if (spell == MonsterAbilityType::HAND_DOOM) {
        return true;
    }

    /* Psycho-Spear */
    if (spell == MonsterAbilityType::PSY_SPEAR) {
        return true;
    }

    /* Doesn't hurt */
    return false;
}

/*!
 * @brief ID値が退避目的に適したモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell is good for escaping.
 * @param spell 判定対象のID
 * @return 適した魔法のIDならばTRUEを返す。
 */
static bool spell_escape(MonsterAbilityType spell)
{
    /* Blink or Teleport */
    if (spell == MonsterAbilityType::BLINK || spell == MonsterAbilityType::TPORT) {
        return true;
    }

    /* Teleport the player away */
    if (spell == MonsterAbilityType::TELE_AWAY || spell == MonsterAbilityType::TELE_LEVEL) {
        return true;
    }

    /* Isn't good for escaping */
    return false;
}

/*!
 * @brief ID値が妨害目的に適したモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell is good for annoying the player.
 * @param spell 判定対象のID
 * @return 適した魔法のIDならばTRUEを返す。
 */
static bool spell_annoy(MonsterAbilityType spell)
{
    /* Shriek */
    if (spell == MonsterAbilityType::SHRIEK) {
        return true;
    }

    /* Brain smash, et al (added curses) */
    if (spell_in_between(spell, MonsterAbilityType::DRAIN_MANA, MonsterAbilityType::CAUSE_4)) {
        return true;
    }

    /* Scare, confuse, blind, slow, paralyze */
    if (spell_in_between(spell, MonsterAbilityType::SCARE, MonsterAbilityType::HOLD)) {
        return true;
    }

    /* Teleport to */
    if (spell == MonsterAbilityType::TELE_TO) {
        return true;
    }

    /* Teleport level */
    if (spell == MonsterAbilityType::TELE_LEVEL) {
        return true;
    }

    /* Darkness, make traps, cause amnesia */
    if (spell_in_between(spell, MonsterAbilityType::TRAPS, MonsterAbilityType::RAISE_DEAD)) {
        return true;
    }

    /* Doesn't annoy */
    return false;
}

/*!
 * @brief ID値が召喚型のモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell is good for annoying the player.
 * @param spell 判定対象のID
 * @return 召喚型魔法のIDならばTRUEを返す。
 */
static bool spell_summon(MonsterAbilityType spell)
{
    return spell_in_between(spell, MonsterAbilityType::S_KIN, MonsterAbilityType::S_DEAD_UNIQUE);
}

/*!
 * @brief ID値が死者復活処理かどうかを返す /
 * Return TRUE if a spell is good for annoying the player.
 * @param spell 判定対象のID
 * @return 死者復活の処理ならばTRUEを返す。
 */
static bool spell_raise(MonsterAbilityType spell)
{
    return spell == MonsterAbilityType::RAISE_DEAD;
}

/*!
 * @brief ID値が戦術的なモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell is good in a tactical situation.
 * @param spell 判定対象のID
 * @return 戦術的な魔法のIDならばTRUEを返す。
 */
static bool spell_tactic(MonsterAbilityType spell)
{
    return spell == MonsterAbilityType::BLINK;
}

/*!
 * @brief ID値が無敵化するモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell makes invulnerable.
 * @param spell 判定対象のID
 * @return 召喚型魔法のIDならばTRUEを返す。
 */
static bool spell_invulner(MonsterAbilityType spell)
{
    return spell == MonsterAbilityType::INVULNER;
}

/*!
 * @brief ID値が加速するモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell hastes.
 * @param spell 判定対象のID
 * @return 召喚型魔法のIDならばTRUEを返す。
 */
static bool spell_haste(MonsterAbilityType spell)
{
    return spell == MonsterAbilityType::HASTE;
}

/*!
 * @brief ID値が時間停止を行うモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell world.
 * @param spell 判定対象のID
 * @return 時間停止魔法のIDならばTRUEを返す。
 */
static bool spell_world(MonsterAbilityType spell)
{
    return spell == MonsterAbilityType::WORLD;
}

/*!
 * @brief ID値が特別効果のモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell special.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param spell 判定対象のID
 * @return 特別効果魔法のIDならばTRUEを返す。
 */
static bool spell_special(MonsterAbilityType spell)
{
    if (AngbandSystem::get_instance().is_phase_out()) {
        return false;
    }

    return spell == MonsterAbilityType::SPECIAL;
}

/*!
 * @brief ID値が光の剣のモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell psycho-spear.
 * @param spell 判定対象のID
 * @return 光の剣のIDならばTRUEを返す。
 */
static bool spell_psy_spe(MonsterAbilityType spell)
{
    return spell == MonsterAbilityType::PSY_SPEAR;
}

/*!
 * @brief ID値が治癒魔法かどうかを返す /
 * Return TRUE if a spell is good for healing.
 * @param spell 判定対象のID
 * @return 治癒魔法のIDならばTRUEを返す。
 */
static bool spell_heal(MonsterAbilityType spell)
{
    return spell == MonsterAbilityType::HEAL;
}

/*!
 * @brief ID値が魔力消去かどうかを返す /
 * Return TRUE if a spell is good for dispel.
 * @param spell 判定対象のID
 * @return 魔力消去のIDならばTRUEを返す。
 */
static bool spell_dispel(MonsterAbilityType spell)
{
    return spell == MonsterAbilityType::DISPEL;
}

/*!
 * @brief 特殊魔法を使用する確率の判定
 * @param r_idx モンスター種族ID
 * @return 特殊魔法を使用するか否か
 * @details 分離/合体ユニークは常に使用しない (その前に判定済のため).
 */
static bool decide_select_special(MonraceId r_idx)
{
    if (MonraceList::get_instance().is_separated(r_idx)) {
        return false;
    }

    switch (r_idx) {
    case MonraceId::OHMU:
        return false;
    case MonraceId::ROLENTO:
        return evaluate_percent(40);
    default:
        return one_in_(2);
    }
}

/*!
 * @brief モンスターの魔法選択ルーチン
 * Have a monster choose a spell from a list of "useful" spells.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターの構造体配列ID
 * @param spells 候補魔法IDをまとめた配列
 * @param num spellsの長さ
 * @return 選択したモンスター魔法のID
 * @details
 * Note that this list does NOT include spells that will just hit\n
 * other monsters, and the list is restricted when the monster is\n
 * "desperate".  Should that be the job of this function instead?\n
 *\n
 * Stupid monsters will just pick a spell randomly.  Smart monsters\n
 * will choose more "intelligently".\n
 *\n
 * Use the helper functions above to put spells into categories.\n
 *\n
 * This function may well be an efficiency bottleneck.\n
 * @todo 長過ぎる。切り分けが必要
 */
MonsterAbilityType choose_attack_spell(PlayerType *player_ptr, msa_type *msa_ptr)
{
    std::vector<MonsterAbilityType> escape;
    std::vector<MonsterAbilityType> attack;
    std::vector<MonsterAbilityType> summon;
    std::vector<MonsterAbilityType> tactic;
    std::vector<MonsterAbilityType> annoy;
    std::vector<MonsterAbilityType> invul;
    std::vector<MonsterAbilityType> haste;
    std::vector<MonsterAbilityType> world_spells;
    std::vector<MonsterAbilityType> special;
    std::vector<MonsterAbilityType> psy_spe;
    std::vector<MonsterAbilityType> raise;
    std::vector<MonsterAbilityType> heal;
    std::vector<MonsterAbilityType> dispel;

    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[msa_ptr->m_idx];
    auto *r_ptr = &m_ptr->get_monrace();
    if (r_ptr->behavior_flags.has(MonsterBehaviorType::STUPID)) {
        return rand_choice(msa_ptr->mspells);
    }

    for (size_t i = 0; i < msa_ptr->mspells.size(); i++) {
        if (spell_escape(msa_ptr->mspells[i])) {
            escape.push_back(msa_ptr->mspells[i]);
        }

        if (spell_attack(msa_ptr->mspells[i])) {
            attack.push_back(msa_ptr->mspells[i]);
        }

        if (spell_summon(msa_ptr->mspells[i])) {
            summon.push_back(msa_ptr->mspells[i]);
        }

        if (spell_tactic(msa_ptr->mspells[i])) {
            tactic.push_back(msa_ptr->mspells[i]);
        }

        if (spell_annoy(msa_ptr->mspells[i])) {
            annoy.push_back(msa_ptr->mspells[i]);
        }

        if (spell_invulner(msa_ptr->mspells[i])) {
            invul.push_back(msa_ptr->mspells[i]);
        }

        if (spell_haste(msa_ptr->mspells[i])) {
            haste.push_back(msa_ptr->mspells[i]);
        }

        if (spell_world(msa_ptr->mspells[i])) {
            world_spells.push_back(msa_ptr->mspells[i]);
        }

        if (spell_special(msa_ptr->mspells[i])) {
            special.push_back(msa_ptr->mspells[i]);
        }

        if (spell_psy_spe(msa_ptr->mspells[i])) {
            psy_spe.push_back(msa_ptr->mspells[i]);
        }

        if (spell_raise(msa_ptr->mspells[i])) {
            raise.push_back(msa_ptr->mspells[i]);
        }

        if (spell_heal(msa_ptr->mspells[i])) {
            heal.push_back(msa_ptr->mspells[i]);
        }

        if (spell_dispel(msa_ptr->mspells[i])) {
            dispel.push_back(msa_ptr->mspells[i]);
        }
    }

    const auto &world = AngbandWorld::get_instance();
    if (!world_spells.empty() && evaluate_percent(15) && !world.timewalk_m_idx) {
        return rand_choice(world_spells);
    }

    const auto &monrace_list = MonraceList::get_instance();
    if (!special.empty() && monrace_list.can_select_separate(m_ptr->r_idx, m_ptr->hp, m_ptr->maxhp)) {
        return rand_choice(special);
    }

    if (m_ptr->hp < m_ptr->maxhp / 3 && one_in_(2)) {
        if (!heal.empty()) {
            return rand_choice(heal);
        }
    }

    if (((m_ptr->hp < m_ptr->maxhp / 3) || m_ptr->is_fearful()) && one_in_(2)) {
        if (!escape.empty()) {
            return rand_choice(escape);
        }
    }

    if (!special.empty()) {
        const auto r_idx = m_ptr->r_idx;
        auto should_select_special = monrace_list.is_unified(r_idx) && evaluate_percent(70);
        should_select_special |= decide_select_special(r_idx);
        if (should_select_special) {
            return rand_choice(special);
        }
    }

    auto should_select_tactic = Grid::calc_distance(player_ptr->get_position(), m_ptr->get_position()) < 4;
    should_select_tactic &= !attack.empty() || r_ptr->ability_flags.has(MonsterAbilityType::TRAPS);
    should_select_tactic &= evaluate_percent(75);
    should_select_tactic &= world.timewalk_m_idx == 0;
    should_select_tactic &= !tactic.empty();
    if (should_select_tactic) {
        return rand_choice(tactic);
    }

    if (!summon.empty() && evaluate_percent(40)) {
        return rand_choice(summon);
    }

    if (!dispel.empty() && one_in_(2)) {
        if (dispel_check(player_ptr, msa_ptr->m_idx)) {
            return rand_choice(dispel);
        }
    }

    if (!raise.empty() && evaluate_percent(40)) {
        return rand_choice(raise);
    }

    if (is_invuln(player_ptr)) {
        if (!psy_spe.empty() && one_in_(2)) {
            return rand_choice(psy_spe);
        } else if (!attack.empty() && evaluate_percent(40)) {
            return rand_choice(attack);
        }
    } else if (!attack.empty() && evaluate_percent(85)) {
        return rand_choice(attack);
    }

    if (!tactic.empty() && one_in_(2) && !world.timewalk_m_idx) {
        return rand_choice(tactic);
    }

    if (!invul.empty() && !m_ptr->mtimed[MonsterTimedEffect::INVULNERABILITY] && one_in_(2)) {
        return rand_choice(invul);
    }

    if ((m_ptr->hp < m_ptr->maxhp * 3 / 4) && one_in_(4)) {
        if (!heal.empty()) {
            return rand_choice(heal);
        }
    }

    if (!haste.empty() && one_in_(5) && !m_ptr->is_accelerated()) {
        return rand_choice(haste);
    }

    if (!annoy.empty() && evaluate_percent(80)) {
        return rand_choice(annoy);
    }

    return MonsterAbilityType::MAX;
}
