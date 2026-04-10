#include "monster-attack/monster-attack-lose.h"
#include "mind/mind-mirror-master.h"
#include "monster-attack/monster-attack-player.h"
#include "monster-attack/monster-attack-status.h"
#include "player-base/player-race.h"
#include "player/player-damage.h"
#include "player/player-status-flags.h"
#include "player/player-status-resist.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/element-resistance.h"
#include "system/creature-entity.h"
#include "view/display-messages.h"

/*!
 * @brief 病気ダメージを計算する (毒耐性があれば、(1d4 + 4) / 9になる。二重耐性なら更に(1d4 + 4) / 9)
 * @param creature クリーチャーへの参照
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 * @details 10% (毒の一次耐性があれば4%、二重耐性ならば1.6%)の確率で耐久が低下し、更に1/10の確率で永久低下する
 */
void calc_blow_disease(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr)
{
    if (has_resist_pois(creature)) {
        monap_ptr->damage = monap_ptr->damage * (randint1(4) + 4) / 9;
    }

    if (is_oppose_pois(creature)) {
        monap_ptr->damage = monap_ptr->damage * (randint1(4) + 4) / 9;
    }

    monap_ptr->get_damage += take_hit(creature, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
    if (creature.is_dead() || check_multishadow(creature)) {
        return;
    }

    if (!(has_resist_pois(creature) || is_oppose_pois(creature)) && BadStatusSetter(creature).mod_poison(randint1(monap_ptr->rlev) + 5)) {
        monap_ptr->obvious = true;
    }

    bool disease_possibility = randint1(100) > calc_nuke_damage_rate(creature);
    if (disease_possibility || (randint1(100) > 10) || CreatureRace(&creature).equals(PlayerRaceType::ANDROID)) {
        return;
    }

    bool perm = one_in_(10);
    if (dec_stat(creature, A_CON, randint1(10), perm)) {
        msg_print(_("病があなたを蝕んでいる気がする。", "You feel sickly."));
        monap_ptr->obvious = true;
    }
}

/*!
 * @brief 腕力低下ダメージを計算する (維持があれば、(1d4 + 4) / 9になる)
 * @param creature クリーチャーへの参照
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 */
void calc_blow_lose_strength(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr)
{
    if (has_sustain_str(creature)) {
        monap_ptr->get_damage = monap_ptr->get_damage * (randint1(4) + 4) / 9;
    }

    monap_ptr->get_damage += take_hit(creature, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
    if (creature.is_dead() || check_multishadow(creature)) {
        return;
    }

    if (do_dec_stat(creature, A_STR)) {
        monap_ptr->obvious = true;
    }
}

/*!
 * @brief 知能低下ダメージを計算する (維持があれば、(1d4 + 4) / 9になる)
 * @param creature クリーチャーへの参照
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 */
void calc_blow_lose_intelligence(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr)
{
    if (has_sustain_int(creature)) {
        monap_ptr->get_damage = monap_ptr->get_damage * (randint1(4) + 4) / 9;
    }

    monap_ptr->get_damage += take_hit(creature, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
    if (creature.is_dead() || check_multishadow(creature)) {
        return;
    }

    if (do_dec_stat(creature, A_INT)) {
        monap_ptr->obvious = true;
    }
}

/*!
 * @brief 賢さ低下ダメージを計算する (維持があれば、(1d4 + 4) / 9になる)
 * @param creature クリーチャーへの参照
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 */
void calc_blow_lose_wisdom(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr)
{
    if (has_sustain_wis(creature)) {
        monap_ptr->get_damage = monap_ptr->get_damage * (randint1(4) + 4) / 9;
    }

    monap_ptr->get_damage += take_hit(creature, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
    if (creature.is_dead() || check_multishadow(creature)) {
        return;
    }

    if (do_dec_stat(creature, A_WIS)) {
        monap_ptr->obvious = true;
    }
}

/*!
 * @brief 器用低下ダメージを計算する (維持があれば、(1d4 + 4) / 9になる)
 * @param creature クリーチャーへの参照
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 */
void calc_blow_lose_dexterity(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr)
{
    if (has_sustain_dex(creature)) {
        monap_ptr->get_damage = monap_ptr->get_damage * (randint1(4) + 4) / 9;
    }

    monap_ptr->get_damage += take_hit(creature, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
    if (creature.is_dead() || check_multishadow(creature)) {
        return;
    }

    if (do_dec_stat(creature, A_DEX)) {
        monap_ptr->obvious = true;
    }
}

/*!
 * @brief 耐久低下ダメージを計算する (維持があれば、(1d4 + 4) / 9になる)
 * @param creature クリーチャーへの参照
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 */
void calc_blow_lose_constitution(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr)
{
    if (has_sustain_con(creature)) {
        monap_ptr->get_damage = monap_ptr->get_damage * (randint1(4) + 4) / 9;
    }

    monap_ptr->get_damage += take_hit(creature, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
    if (creature.is_dead() || check_multishadow(creature)) {
        return;
    }

    if (do_dec_stat(creature, A_CON)) {
        monap_ptr->obvious = true;
    }
}

/*!
 * @brief 魅力低下ダメージを計算する (維持があれば、(1d4 + 4) / 9になる)
 * @param creature クリーチャーへの参照
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 */
void calc_blow_lose_charisma(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr)
{
    if (has_sustain_chr(creature)) {
        monap_ptr->get_damage = monap_ptr->get_damage * (randint1(4) + 4) / 9;
    }

    monap_ptr->get_damage += take_hit(creature, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
    if (creature.is_dead() || check_multishadow(creature)) {
        return;
    }

    if (do_dec_stat(creature, A_CHR)) {
        monap_ptr->obvious = true;
    }
}

/*!
 * @brief 全能力低下ダメージを計算する (維持があれば、1つに付き-3%軽減する)
 * @param creature クリーチャーへの参照
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 */
void calc_blow_lose_all(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr)
{
    int damage_ratio = 100;
    if (has_sustain_str(creature)) {
        damage_ratio -= 3;
    }

    if (has_sustain_int(creature)) {
        damage_ratio -= 3;
    }

    if (has_sustain_wis(creature)) {
        damage_ratio -= 3;
    }

    if (has_sustain_dex(creature)) {
        damage_ratio -= 3;
    }

    if (has_sustain_con(creature)) {
        damage_ratio -= 3;
    }

    if (has_sustain_chr(creature)) {
        damage_ratio -= 3;
    }

    monap_ptr->damage = monap_ptr->damage * damage_ratio / 100;
    monap_ptr->get_damage += take_hit(creature, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
    if (creature.is_dead() || check_multishadow(creature)) {
        return;
    }

    process_lose_all_attack(creature, monap_ptr);
}
