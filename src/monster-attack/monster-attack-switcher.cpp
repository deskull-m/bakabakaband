/*!
 * @brief モンスターからプレイヤーへの直接攻撃をその種別において振り分ける
 * @date 2020/05/31
 * @author Hourier
 * @details 長い処理はインクルード先の別ファイルにて行っている
 */

#include "monster-attack/monster-attack-switcher.h"
#include "dungeon/quest.h"
#include "inventory/inventory-slot-types.h"
#include "mind/drs-types.h"
#include "mind/mind-mirror-master.h"
#include "monster-attack/monster-attack-lose.h"
#include "monster-attack/monster-attack-player.h"
#include "monster-attack/monster-attack-status.h"
#include "monster-attack/monster-attack-table.h"
#include "monster-attack/monster-eating.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "mutation/mutation-investor-remover.h"
#include "player/eldritch-horror.h"
#include "player/player-damage.h"
#include "player/player-status-flags.h"
#include "player/player-status-resist.h"
#include "player/player-status.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/spells-equipment.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-teleport.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/element-resistance.h"
#include "status/experience.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/item-entity.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"

/*!
 * @brief 毒ダメージを計算する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 * @details 減衰の計算式がpoisではなくnukeなのは仕様 (1/3では減衰が強すぎると判断したため)
 */
static void calc_blow_poison(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr)
{
    if (monap_ptr->explode) {
        return;
    }

    if (!(has_resist_pois(creature) || is_oppose_pois(creature)) && !check_multishadow(creature) && BadStatusSetter(creature).mod_poison(randint1(monap_ptr->rlev) + 5)) {
        monap_ptr->obvious = true;
    }

    monap_ptr->damage = monap_ptr->damage * calc_nuke_damage_rate(creature) / 100;
    monap_ptr->get_damage += take_hit(creature, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, monap_ptr->m_ptr->r_idx);
    update_smart_learn(creature, monap_ptr->m_idx, DRS_POIS);
}

/*!
 * @brief 劣化ダメージを計算する (耐性があれば、(1d4 + 4) / 9になる)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 */
static void calc_blow_disenchant(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr)
{
    if (monap_ptr->explode) {
        return;
    }

    if (!has_resist_disen(creature) && !check_multishadow(creature) && apply_disenchant(creature, 0)) {
        update_creature(creature);
        monap_ptr->obvious = true;
    }

    if (has_resist_disen(creature)) {
        monap_ptr->damage = monap_ptr->damage * (randint1(4) + 4) / 9;
    }

    monap_ptr->get_damage += take_hit(creature, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, monap_ptr->m_ptr->r_idx);
    update_smart_learn(creature, monap_ptr->m_idx, DRS_DISEN);
}

/*!
 * @brief 魔道具吸収ダメージを計算する (消費魔力減少、呪文失敗率減少、魔道具使用能力向上があればそれぞれ-7.5%)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 * @detals 魔道具使用能力向上フラグがあれば、吸収対象のアイテムをスキャンされる回数が半分で済む
 */
static void calc_blow_un_power(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr)
{
    int damage_ratio = 1000;
    if (has_dec_mana(creature)) {
        damage_ratio -= 75;
    }

    if (has_easy_spell(creature)) {
        damage_ratio -= 75;
    }

    bool is_magic_mastery = has_magic_mastery(creature) != 0;
    if (is_magic_mastery) {
        damage_ratio -= 75;
    }

    monap_ptr->damage = monap_ptr->damage * damage_ratio / 1000;
    monap_ptr->get_damage += take_hit(creature, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, monap_ptr->m_ptr->r_idx);
    if (creature.is_dead() || check_multishadow(creature)) {
        return;
    }

    int max_draining_item = is_magic_mastery ? 5 : 10;
    for (int i = 0; i < max_draining_item; i++) {
        auto i_idx = randnum0<short>(INVEN_PACK);
        monap_ptr->o_ptr = creature.inventory[i_idx].get();
        if (!monap_ptr->o_ptr->is_valid()) {
            continue;
        }

        if (process_un_power(creature, monap_ptr)) {
            break;
        }
    }
}

/*!
 * @brief 盲目ダメージを計算する (耐性があれば、(1d4 + 3) / 8になる)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 */
static void calc_blow_blind(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr)
{
    if (has_resist_blind(creature)) {
        monap_ptr->damage = monap_ptr->damage * (randint1(4) + 3) / 8;
    }

    monap_ptr->get_damage += take_hit(creature, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, monap_ptr->m_ptr->r_idx);
    if (creature.is_dead()) {
        return;
    }

    process_blind_attack(creature, monap_ptr);
    update_smart_learn(creature, monap_ptr->m_idx, DRS_BLIND);
}

/*!
 * @brief 混乱ダメージを計算する (耐性があれば、(1d4 + 3) / 8になる)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 */
static void calc_blow_confusion(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr)
{
    if (monap_ptr->explode) {
        return;
    }

    if (has_resist_conf(creature)) {
        monap_ptr->damage = monap_ptr->damage * (randint1(4) + 3) / 8;
    }

    monap_ptr->get_damage += take_hit(creature, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, monap_ptr->m_ptr->r_idx);
    if (creature.is_dead()) {
        return;
    }

    if (!has_resist_conf(creature) && !check_multishadow(creature) && BadStatusSetter(creature).mod_confusion(3 + randint1(monap_ptr->rlev))) {
        monap_ptr->obvious = true;
    }

    update_smart_learn(creature, monap_ptr->m_idx, DRS_CONF);
}

/*!
 * @brief 恐怖ダメージを計算する (耐性があれば、(1d4 + 3) / 8になる)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 */
static void calc_blow_fear(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr)
{
    if (has_resist_fear(creature)) {
        monap_ptr->damage = monap_ptr->damage * (randint1(4) + 3) / 8;
    }

    monap_ptr->get_damage += take_hit(creature, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, monap_ptr->m_ptr->r_idx);
    if (creature.is_dead()) {
        return;
    }

    process_terrify_attack(creature, monap_ptr);
    update_smart_learn(creature, monap_ptr->m_idx, DRS_FEAR);
}

/*!
 * @brief 麻痺ダメージを計算する (耐性があれば、(1d4 + 3) / 8になる)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 */
static void calc_blow_paralysis(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr)
{
    if (has_free_act(creature)) {
        monap_ptr->damage = monap_ptr->damage * (randint1(4) + 3) / 8;
    }

    monap_ptr->get_damage += take_hit(creature, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, monap_ptr->m_ptr->r_idx);
    if (creature.is_dead()) {
        return;
    }

    process_paralyze_attack(creature, monap_ptr);
    update_smart_learn(creature, monap_ptr->m_idx, DRS_FREE);
}

/*!
 * @brief 経験値吸収ダメージを計算する (経験値保持と地獄耐性があれば、それぞれ-7.5%)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 */
static void calc_blow_drain_exp(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr, const int drain_value, const int hold_exp_prob)
{
    int32_t d = Dice::roll(drain_value, 6) + (creature.exp / 100) * MON_DRAIN_LIFE;
    monap_ptr->obvious = true;
    int damage_ratio = 1000;
    if (has_hold_exp(creature)) {
        damage_ratio -= 75;
    }

    if (has_resist_neth(creature)) {
        damage_ratio -= 75;
    }

    monap_ptr->damage = monap_ptr->damage * damage_ratio / 1000;
    monap_ptr->get_damage += take_hit(creature, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, monap_ptr->m_ptr->r_idx);
    if (creature.is_dead() || check_multishadow(creature)) {
        return;
    }

    (void)drain_exp(creature, d, d / 10, hold_exp_prob);
}

/*!
 * @brief 時間逆転ダメージを計算する (耐性があれば、(1d4 + 4) / 9になる)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 */
static void calc_blow_time(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr)
{
    if (monap_ptr->explode) {
        return;
    }

    process_monster_attack_time(creature);
    if (has_resist_time(creature)) {
        monap_ptr->damage = monap_ptr->damage * (randint1(4) + 4) / 9;
    }

    monap_ptr->get_damage += take_hit(creature, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, monap_ptr->m_ptr->r_idx);
}

/*!
 * @brief 生命力吸収ダメージを計算する (経験値維持があれば9/10になる)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 */
static void calc_blow_drain_life(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr)
{
    int32_t d = Dice::roll(60, 6) + (creature.exp / 100) * MON_DRAIN_LIFE;
    monap_ptr->obvious = true;
    if (creature.hold_exp) {
        monap_ptr->damage = monap_ptr->damage * 9 / 10;
    }

    monap_ptr->get_damage += take_hit(creature, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, monap_ptr->m_ptr->r_idx);
    if (creature.is_dead() || check_multishadow(creature)) {
        return;
    }

    bool resist_drain = check_drain_hp(creature, d);
    process_drain_life(monap_ptr, resist_drain);
}

/*!
 * @brief MPダメージを計算する (消費魔力減少、呪文失敗率減少、魔道具使用能力向上があればそれぞれ-5%)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 */
static void calc_blow_drain_mana(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr)
{
    monap_ptr->obvious = true;
    int damage_ratio = 100;
    if (has_dec_mana(creature)) {
        damage_ratio -= 5;
    }

    if (has_easy_spell(creature)) {
        damage_ratio -= 5;
    }

    if (has_magic_mastery(creature)) {
        damage_ratio -= 5;
    }

    monap_ptr->damage = monap_ptr->damage * damage_ratio / 100;
    process_drain_mana(creature, monap_ptr);
    update_smart_learn(creature, monap_ptr->m_idx, DRS_MANA);
}

static void calc_blow_inertia(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr)
{
    if (creature.effects()->acceleration().is_fast() || (static_cast<CreatureEntity &>(creature).get_speed() >= 130)) {
        monap_ptr->damage = monap_ptr->damage * (randint1(4) + 4) / 9;
    }

    monap_ptr->get_damage += take_hit(creature, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, monap_ptr->m_ptr->r_idx);
    if (creature.is_dead() || check_multishadow(creature)) {
        return;
    }

    if (BadStatusSetter(creature).mod_deceleration(4 + randint0(monap_ptr->rlev / 10), false)) {
        monap_ptr->obvious = true;
    }
}

/*!
 * @brief 空腹進行度を計算する (急速回復があれば+100%、遅消化があれば-50%)
 */
static void calc_blow_hungry(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr)
{
    if (creature.regenerate) {
        monap_ptr->damage = monap_ptr->damage * 2;
    }
    if (creature.slow_digest) {
        monap_ptr->damage = monap_ptr->damage / 2;
    }

    process_monster_attack_hungry(creature, monap_ptr);
}

void switch_monster_blow_to_player(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr)
{
    switch (monap_ptr->effect) {
    case RaceBlowEffectType::NONE:
        // ここには来ないはずだが、何らかのバグで来た場合はプレイヤーの不利益に
        // ならないようダメージを 0 にしておく。
        monap_ptr->damage = 0;
        break;
    case RaceBlowEffectType::SUPERHURT: { /* AC軽減あり / Player armor reduces total damage */
        if (((randint1(monap_ptr->rlev * 2 + 300) > (monap_ptr->ac + 200)) || one_in_(13)) && !check_multishadow(creature)) {
            monap_ptr->damage -= (monap_ptr->damage * ((monap_ptr->ac < 150) ? monap_ptr->ac : 150) / 250);
            msg_print(_("痛恨の一撃！", "It was a critical hit!"));
            monap_ptr->damage = std::max(monap_ptr->damage, monap_ptr->damage * 2);
            monap_ptr->get_damage += take_hit(creature, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, monap_ptr->m_ptr->r_idx);
            break;
        }
    }
        [[fallthrough]];
    case RaceBlowEffectType::HURT: { /* AC軽減あり / Player armor reduces total damage */
        monap_ptr->obvious = true;
        monap_ptr->damage -= (monap_ptr->damage * ((monap_ptr->ac < 150) ? monap_ptr->ac : 150) / 250);
        monap_ptr->get_damage += take_hit(creature, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, monap_ptr->m_ptr->r_idx);
        break;
    }
    case RaceBlowEffectType::POISON:
        calc_blow_poison(creature, monap_ptr);
        break;
    case RaceBlowEffectType::UN_BONUS:
        calc_blow_disenchant(creature, monap_ptr);
        break;
    case RaceBlowEffectType::UN_POWER:
        calc_blow_un_power(creature, monap_ptr);
        break;
    case RaceBlowEffectType::EAT_GOLD:
        monap_ptr->get_damage += take_hit(creature, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, monap_ptr->m_ptr->r_idx);
        if (monap_ptr->m_ptr->is_confused() || creature.is_dead() || check_multishadow(creature)) {
            break;
        }

        monap_ptr->obvious = true;
        process_eat_gold(creature, monap_ptr);
        break;
    case RaceBlowEffectType::EAT_ITEM: {
        monap_ptr->get_damage += take_hit(creature, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, monap_ptr->m_ptr->r_idx);
        if (!check_eat_item(creature, monap_ptr)) {
            break;
        }

        process_eat_item(creature, monap_ptr);
        break;
    }

    case RaceBlowEffectType::EAT_FOOD: {
        monap_ptr->get_damage += take_hit(creature, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, monap_ptr->m_ptr->r_idx);
        if (creature.is_dead() || check_multishadow(creature)) {
            break;
        }

        process_eat_food(creature, monap_ptr);
        break;
    }
    case RaceBlowEffectType::EAT_LITE: {
        monap_ptr->o_ptr = creature.inventory[INVEN_LITE].get();
        monap_ptr->get_damage += take_hit(creature, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, monap_ptr->m_ptr->r_idx);
        if (creature.is_dead() || check_multishadow(creature)) {
            break;
        }

        process_eat_lite(creature, monap_ptr);
        break;
    }
    case RaceBlowEffectType::ACID: {
        if (monap_ptr->explode) {
            break;
        }

        monap_ptr->obvious = true;
        msg_print(_("酸を浴びせられた！", "You are covered in acid!"));
        monap_ptr->get_damage += acid_dam(creature, monap_ptr->damage, monap_ptr->ddesc, false);
        update_creature(creature);
        update_smart_learn(creature, monap_ptr->m_idx, DRS_ACID);
        break;
    }
    case RaceBlowEffectType::ELEC: {
        if (monap_ptr->explode) {
            break;
        }

        monap_ptr->obvious = true;
        msg_print(_("電撃を浴びせられた！", "You are struck by electricity!"));
        monap_ptr->get_damage += elec_dam(creature, monap_ptr->damage, monap_ptr->ddesc, false);
        update_smart_learn(creature, monap_ptr->m_idx, DRS_ELEC);
        break;
    }
    case RaceBlowEffectType::FIRE: {
        if (monap_ptr->explode) {
            break;
        }

        monap_ptr->obvious = true;
        msg_print(_("全身が炎に包まれた！", "You are enveloped in flames!"));
        monap_ptr->get_damage += fire_dam(creature, monap_ptr->damage, monap_ptr->ddesc, false);
        update_smart_learn(creature, monap_ptr->m_idx, DRS_FIRE);
        break;
    }
    case RaceBlowEffectType::COLD: {
        if (monap_ptr->explode) {
            break;
        }

        monap_ptr->obvious = true;
        msg_print(_("全身が冷気で覆われた！", "You are covered with frost!"));
        monap_ptr->get_damage += cold_dam(creature, monap_ptr->damage, monap_ptr->ddesc, false);
        update_smart_learn(creature, monap_ptr->m_idx, DRS_COLD);
        break;
    }
    case RaceBlowEffectType::BLIND:
        calc_blow_blind(creature, monap_ptr);
        break;
    case RaceBlowEffectType::CONFUSE:
        calc_blow_confusion(creature, monap_ptr);
        break;
    case RaceBlowEffectType::TERRIFY:
        calc_blow_fear(creature, monap_ptr);
        break;
    case RaceBlowEffectType::PARALYZE:
        calc_blow_paralysis(creature, monap_ptr);
        break;
    case RaceBlowEffectType::LOSE_STR:
        calc_blow_lose_strength(creature, monap_ptr);
        break;
    case RaceBlowEffectType::LOSE_INT:
        calc_blow_lose_intelligence(creature, monap_ptr);
        break;
    case RaceBlowEffectType::LOSE_WIS:
        calc_blow_lose_wisdom(creature, monap_ptr);
        break;
    case RaceBlowEffectType::LOSE_DEX:
        calc_blow_lose_dexterity(creature, monap_ptr);
        break;
    case RaceBlowEffectType::LOSE_CON:
        calc_blow_lose_constitution(creature, monap_ptr);
        break;
    case RaceBlowEffectType::LOSE_CHR:
        calc_blow_lose_charisma(creature, monap_ptr);
        break;
    case RaceBlowEffectType::LOSE_ALL:
        calc_blow_lose_all(creature, monap_ptr);
        break;
    case RaceBlowEffectType::SHATTER: { /* AC軽減あり / Player armor reduces total damage */
        monap_ptr->obvious = true;
        monap_ptr->damage -= (monap_ptr->damage * ((monap_ptr->ac < 150) ? monap_ptr->ac : 150) / 250);
        monap_ptr->get_damage += take_hit(creature, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, monap_ptr->m_ptr->r_idx);
        if (monap_ptr->damage > 23 || monap_ptr->explode) {
            earthquake(creature, monap_ptr->m_ptr->get_position(), 8, monap_ptr->m_idx);
        }

        break;
    }
    case RaceBlowEffectType::EXP_10:
        calc_blow_drain_exp(creature, monap_ptr, 10, 95);
        break;
    case RaceBlowEffectType::EXP_20:
        calc_blow_drain_exp(creature, monap_ptr, 20, 90);
        break;
    case RaceBlowEffectType::EXP_40:
        calc_blow_drain_exp(creature, monap_ptr, 40, 75);
        break;
    case RaceBlowEffectType::EXP_80:
        calc_blow_drain_exp(creature, monap_ptr, 80, 50);
        break;
    case RaceBlowEffectType::DISEASE:
        calc_blow_disease(creature, monap_ptr);
        break;
    case RaceBlowEffectType::TIME:
        calc_blow_time(creature, monap_ptr);
        break;
    case RaceBlowEffectType::DR_LIFE:
        calc_blow_drain_life(creature, monap_ptr);
        break;
    case RaceBlowEffectType::DR_MANA:
        calc_blow_drain_mana(creature, monap_ptr);
        break;
    case RaceBlowEffectType::INERTIA:
        calc_blow_inertia(creature, monap_ptr);
        break;
    case RaceBlowEffectType::STUN:
        monap_ptr->get_damage += take_hit(creature, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, monap_ptr->m_ptr->r_idx);
        if (creature.is_dead()) {
            break;
        }
        process_stun_attack(creature, monap_ptr);
        break;
    case RaceBlowEffectType::FLAVOR:
        // フレーバー打撃は自明かつダメージ 0。
        monap_ptr->obvious = true;
        monap_ptr->damage = 0;
        break;
    case RaceBlowEffectType::HUNGRY:
        calc_blow_hungry(creature, monap_ptr);
        break;
    case RaceBlowEffectType::CHAOS: {
        update_smart_learn(creature, monap_ptr->m_idx, DRS_CHAOS);
        monap_ptr->damage = monap_ptr->damage * calc_chaos_damage_rate(creature, CALC_RAND) / 100;
        monap_ptr->get_damage += take_hit(creature, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, monap_ptr->m_ptr->r_idx);

        const auto has_chaos_resist = has_resist_chaos(creature);

        if (!has_chaos_resist) {
            monap_ptr->obvious = true;
        }
        if (randint1(5) < 3) {
            monap_ptr->obvious = true;
            if (!has_chaos_resist) {
                if (creature.is_dead() || check_multishadow(creature)) {
                    return;
                }

                int32_t d = Dice::roll(60, 6) + (creature.exp / 100) * MON_DRAIN_LIFE;

                bool resist_drain = check_drain_hp(creature, d);
                process_drain_life(monap_ptr, resist_drain);
            }
            break;
        }
        if (one_in_(250)) {
            monap_ptr->obvious = true;
            const auto &floor = *creature.current_floor_ptr;
            if (floor.is_underground() && (!floor.is_in_quest() || !QuestType::is_fixed(floor.quest_number))) {
                if (monap_ptr->damage > 23 || monap_ptr->explode) {
                    msg_print(_("カオスの力でダンジョンが崩れ始める！", "The dungeon tumbles by the chaotic power!"));
                    earthquake(creature, monap_ptr->m_ptr->get_position(), 8, monap_ptr->m_idx);
                    break;
                }
            }
        }
        if (!one_in_(10)) {
            if (creature.is_dead()) {
                return;
            }
            monap_ptr->obvious = true;

            if (!has_chaos_resist && !has_resist_conf(creature) && !check_multishadow(creature) && BadStatusSetter(creature).mod_confusion(3 + randint1(monap_ptr->rlev))) {
                monap_ptr->obvious = true;
            }
            break;
        }

        if (one_in_(2)) {
            if (creature.is_dead()) {
                return;
            }
            monap_ptr->obvious = true;

            if (!has_chaos_resist && creature.anti_tele == 0) {
                msg_print(_("突然体が浮きだした！", "Your body floats suddenly!"));
                teleport_player(creature, 50, TELEPORT_PASSIVE);
            }
        } else if (!has_chaos_resist) {
            if (creature.is_dead()) {
                return;
            }
            monap_ptr->obvious = true;

            msg_print(_("あなたの身体はカオスの力で捻じ曲げられた！", "Your body is twisted by chaos!"));
            (void)gain_mutation(creature, 0);
        }
    } break;

    case RaceBlowEffectType::DEFECATE: { /* AC軽減あり / Player armor reduces total damage */
        monap_ptr->obvious = true;
        monap_ptr->damage -= (monap_ptr->damage * ((monap_ptr->ac < 150) ? monap_ptr->ac : 150) / 250);
        monap_ptr->get_damage += take_hit(creature, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, monap_ptr->m_ptr->r_idx);
        if (monap_ptr->damage * 2 > randint1(creature.hp)) {
            player_defecate(creature);
        }
        break;
    }

    case RaceBlowEffectType::SANITY_BLAST: {
        monap_ptr->get_damage += take_hit(creature, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, monap_ptr->m_ptr->r_idx);
        if (creature.is_dead()) {
            break;
        }
        sanity_blast(creature);
        break;
    }

    case RaceBlowEffectType::GROIN_ATTACK: { /* AC軽減あり / Player armor reduces total damage */
        monap_ptr->damage -= (monap_ptr->damage * ((monap_ptr->ac < 150) ? monap_ptr->ac : 150) / 250);
        monap_ptr->get_damage += take_hit(creature, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, monap_ptr->m_ptr->r_idx);
        if (creature.is_dead()) {
            break;
        }
        process_groin_attack(creature, monap_ptr);
        break;
    }

    case RaceBlowEffectType::LOCKUP: { /* AC軽減あり / Player armor reduces total damage */
        if (creature.anti_tele == 0) {
            teleport_player(creature, 50, TELEPORT_PASSIVE);
            wall_creation(creature, creature.y, creature.x);
        }
        break;
    }

    case RaceBlowEffectType::DESTROY_ASSHOLE: { /* AC軽減あり / Player armor reduces total damage */
        monap_ptr->obvious = true;
        monap_ptr->damage -= (monap_ptr->damage * ((monap_ptr->ac < 150) ? monap_ptr->ac : 150) / 250);
        monap_ptr->get_damage += take_hit(creature, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, monap_ptr->m_ptr->r_idx);

        // ダメージ量の最大HPに対する比率を計算
        int damage_ratio = (monap_ptr->damage * 100) / creature.maxhp;

        // 20%以上のダメージで肛門破壊の変異発生判定
        if (damage_ratio >= 20) {
            int chance = damage_ratio - 15; // 20%で5%、50%で35%、100%で85%の確率
            if (randint1(100) <= chance) {
                msg_print(_("あなたの肛門が完全に破壊された！", "Your asshole has been completely destroyed!"));
                (void)gain_mutation(creature, static_cast<int>(PlayerMutationType::DESTROYED_ASSHOLE));
            } else {
                msg_print(_("肛門に激痛が走った！", "Your asshole is in severe pain!"));
            }
        } else if (damage_ratio >= 10) {
            msg_print(_("肛門に痛みを感じた...", "You feel pain in your asshole..."));
        }
        break;
    }

    case RaceBlowEffectType::MAX:
        break;
    }
}
