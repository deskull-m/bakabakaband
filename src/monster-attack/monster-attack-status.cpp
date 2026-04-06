/*!
 * @brief プレイヤーのステータス (麻痺等)に影響を与えるモンスターの打撃処理
 * @date 2020/05/31
 * @author Hourier
 */

#include "monster-attack/monster-attack-status.h"
#include "mind/mind-mirror-master.h"
#include "monster-attack/monster-attack-player.h"
#include "player-base/player-race.h"
#include "player-info/race-types.h"
#include "player/player-sex.h"
#include "player/player-status-flags.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/experience.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"

void process_blind_attack(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr)
{
    if (has_resist_blind(creature) || check_multishadow(creature)) {
        return;
    }

    auto is_dio = monap_ptr->m_ptr->r_idx == MonraceId::DIO;
    auto dio_msg = _("どうだッ！この血の目潰しはッ！", "How is it! This blood-blinding!");
    if (is_dio && CreatureRace(&creature).equals(PlayerRaceType::SKELETON)) {
        msg_print(dio_msg);
        msg_print(_("しかし、あなたには元々目はなかった！", "However, you don't have eyes!"));
        return;
    }

    if (!BadStatusSetter(creature).mod_blindness(10 + randint1(monap_ptr->rlev))) {
        return;
    }

    if (is_dio) {
        msg_print(dio_msg);
    }

    monap_ptr->obvious = true;
}

void process_terrify_attack(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr)
{
    if (check_multishadow(creature)) {
        return;
    }

    const auto &monrace = monap_ptr->m_ptr->get_monrace();
    if (has_resist_fear(creature)) {
        msg_print(_("しかし恐怖に侵されなかった！", "You stand your ground!"));
        monap_ptr->obvious = true;
        return;
    }

    if (randint0(100 + monrace.level / 2) < creature.skill_sav) {
        msg_print(_("しかし恐怖に侵されなかった！", "You stand your ground!"));
        monap_ptr->obvious = true;
        return;
    }

    if (BadStatusSetter(creature).mod_fear(3 + randint1(monap_ptr->rlev))) {
        monap_ptr->obvious = true;
    }
}

void process_paralyze_attack(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr)
{
    if (check_multishadow(creature)) {
        return;
    }

    const auto &monrace = monap_ptr->m_ptr->get_monrace();
    if (creature.free_act) {
        msg_print(_("しかし効果がなかった！", "You are unaffected!"));
        monap_ptr->obvious = true;
        return;
    }

    if (randint0(100 + monrace.level / 2) < creature.skill_sav) {
        msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
        monap_ptr->obvious = true;
        return;
    }

    const auto is_paralyzed = creature.is_paralyzed();
    if (!is_paralyzed && BadStatusSetter(creature).set_paralysis(3 + randint1(monap_ptr->rlev))) {
        monap_ptr->obvious = true;
    }
}

void process_lose_all_attack(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr)
{
    if (do_dec_stat(creature, A_STR)) {
        monap_ptr->obvious = true;
    }

    if (do_dec_stat(creature, A_DEX)) {
        monap_ptr->obvious = true;
    }

    if (do_dec_stat(creature, A_CON)) {
        monap_ptr->obvious = true;
    }

    if (do_dec_stat(creature, A_INT)) {
        monap_ptr->obvious = true;
    }

    if (do_dec_stat(creature, A_WIS)) {
        monap_ptr->obvious = true;
    }

    if (do_dec_stat(creature, A_CHR)) {
        monap_ptr->obvious = true;
    }
}

void process_stun_attack(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr)
{
    if (has_resist_sound(creature) || check_multishadow(creature)) {
        return;
    }

    const auto &monrace = monap_ptr->m_ptr->get_monrace();
    if (BadStatusSetter(creature).mod_stun(10 + randint1(monrace.level / 4))) {
        monap_ptr->obvious = true;
    }
}

void process_groin_attack(CreatureEntity &creature, MonsterAttackPlayer *monap_ptr)
{
    if (check_multishadow(creature)) {
        return;
    }

    // 男性または両性の場合のみ追加効果
    const bool is_vulnerable = (creature.psex == SEX_MALE) || (creature.psex == SEX_BISEXUAL);

    if (is_vulnerable) {
        // 追加ダメージを与える（元のダメージの50%追加）
        const int extra_damage = monap_ptr->damage / 2;
        monap_ptr->damage += extra_damage;

        // 朦朧状態を付与
        const auto &monrace = monap_ptr->m_ptr->get_monrace();
        if (BadStatusSetter(creature).mod_stun(15 + randint1(monrace.level / 3))) {
            monap_ptr->obvious = true;
        }

        msg_print(_("痛っ！", "Ouch!"));
    }

    monap_ptr->obvious = true;
}

void process_monster_attack_time(CreatureEntity &creature)
{
    if (has_resist_time(creature) || check_multishadow(creature)) {
        return;
    }

    switch (randint1(10)) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        if (CreatureRace(&creature).equals(PlayerRaceType::ANDROID)) {
            break;
        }

        msg_print(_("人生が逆戻りした気がする。", "You feel like a chunk of the past has been ripped away."));
        lose_exp(creature, 100 + (creature.exp / 100) * MON_DRAIN_LIFE);
        break;
    case 6:
    case 7:
    case 8:
    case 9:
        msg_print(creature.decrease_ability_random());
        break;
    case 10:
        msg_print(creature.decrease_ability_all());
        break;
    }
}
