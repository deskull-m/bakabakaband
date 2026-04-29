/*
 * @brief 薬を飲んだ時の効果処理
 * @date 2022/03/10
 * @author Hourier
 */

#include "object-use/quaff/quaff-effects.h"
#include "avatar/avatar.h"
#include "birth/birth-stat.h"
#include "game-option/birth-options.h"
#include "mutation/mutation-investor-remover.h"
#include "object/object-info.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/race-info.h"
#include "player-info/self-info.h"
#include "player/attack-defense-types.h"
#include "player/digestion-processor.h"
#include "player/eldritch-horror.h"
#include "player/player-damage.h"
#include "player/player-sex.h"
#include "player/player-skill.h"
#include "player/player-status-flags.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-teleport.h"
#include "spell/spells-status.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "status/experience.h"
#include "status/shape-changer.h"
#include "status/sight-setter.h"
#include "sv-definition/sv-potion-types.h"
#include "system/angband.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "system/redrawing-flags-updater.h"
#include "util/bit-flags-calculator.h"
#include "util/dice.h"
#include "view/display-messages.h"

QuaffEffects::QuaffEffects(CreatureEntity &creature)
    : creature(creature)
{
}

bool QuaffEffects::influence(const ItemEntity &item, const bool is_rectal)
{
    if (item.bi_key.tval() != ItemKindType::POTION) {
        return false;
    }

    switch (*item.bi_key.sval()) {
    case SV_POTION_WATER:
        if (!is_rectal) {
            msg_print(_("口の中がさっぱりした。", "That was refreshing."));
            msg_print(_("のどの渇きが少しおさまった。", "You feel less thirsty."));
            return true;
        }
        return false;
    case SV_POTION_APPLE_JUICE:
        if (!is_rectal) {
            msg_print(_("甘くてサッパリとしていて、とてもおいしい。", "It's sweet, refreshing and very tasty."));
            msg_print(_("のどの渇きが少しおさまった。", "You feel less thirsty."));
            return true;
        }
        return false;
    case SV_POTION_SLIME_MOLD:
        if (!is_rectal) {
            msg_print(_("なんとも不気味な味だ。", "That was strange."));
            msg_print(_("のどの渇きが少しおさまった。", "You feel less thirsty."));
            return true;
        }
        return false;
    case SV_POTION_SLOWNESS:
        return BadStatusSetter(this->creature).set_deceleration(randint1(25) + 15, false);
    case SV_POTION_SALT_WATER:
        if (!is_rectal) {
            return this->salt_water();
        } else {
            return false;
        }
    case SV_POTION_POISON:
        return this->poison();
    case SV_POTION_BLINDNESS:
        return this->blindness();
    case SV_POTION_BOOZE:
        return this->booze();
        break;
    case SV_POTION_SLEEP:
        return this->sleep();
    case SV_POTION_LOSE_MEMORIES:
        return this->lose_memories();
    case SV_POTION_RUINATION:
        return this->ruination();
    case SV_POTION_DEC_STR:
        return do_dec_stat(this->creature, A_STR);
    case SV_POTION_DEC_INT:
        return do_dec_stat(this->creature, A_INT);
    case SV_POTION_DEC_WIS:
        return do_dec_stat(this->creature, A_WIS);
    case SV_POTION_DEC_DEX:
        return do_dec_stat(this->creature, A_DEX);
    case SV_POTION_DEC_CON:
        return do_dec_stat(this->creature, A_CON);
    case SV_POTION_DEC_CHR:
        return do_dec_stat(this->creature, A_CHR);
    case SV_POTION_DETONATIONS:
        return this->detonation();
    case SV_POTION_DEATH:
        return this->death();
    case SV_POTION_INFRAVISION:
        return set_tim_infra(this->creature, this->creature.get_timed_effect(CreatureTimedEffect::TIM_INFRA) + 100 + randint1(100), false);
    case SV_POTION_DETECT_INVIS:
        return set_tim_invis(this->creature, this->creature.get_timed_effect(CreatureTimedEffect::TIM_INVIS) + 12 + randint1(12), false);
    case SV_POTION_SLOW_POISON:
        return BadStatusSetter(this->creature).set_poison(this->creature.get_remaining_poison() / 2);
    case SV_POTION_CURE_POISON:
        return BadStatusSetter(this->creature).set_poison(0);
    case SV_POTION_BOLDNESS:
        return BadStatusSetter(this->creature).set_fear(0);
    case SV_POTION_SPEED:
        return this->speed();
    case SV_POTION_RESIST_HEAT:
        return set_oppose_fire(this->creature, this->creature.get_timed_effect(CreatureTimedEffect::OPPOSE_FIRE) + randint1(10) + 10, false);
    case SV_POTION_RESIST_COLD:
        return set_oppose_cold(this->creature, this->creature.get_timed_effect(CreatureTimedEffect::OPPOSE_COLD) + randint1(10) + 10, false);
    case SV_POTION_HEROISM:
        return heroism(this->creature, 25);
    case SV_POTION_BESERK_STRENGTH:
        return berserk(this->creature, randint1(25) + 25);
    case SV_POTION_CURE_LIGHT:
        return cure_light_wounds(this->creature, Dice::roll(2, 8));
    case SV_POTION_CURE_SERIOUS:
        return cure_serious_wounds(this->creature, Dice::roll(4, 8));
    case SV_POTION_CURE_CRITICAL:
        return cure_critical_wounds(this->creature, Dice::roll(6, 8));
    case SV_POTION_HEALING:
        return cure_critical_wounds(this->creature, 300);
    case SV_POTION_STAR_HEALING:
        return cure_critical_wounds(this->creature, 1200);
    case SV_POTION_LIFE:
        return life_stream(this->creature, true, true);
    case SV_POTION_RESTORE_MANA:
        return restore_mana(this->creature, true);
    case SV_POTION_RESTORE_EXP:
        return restore_level(this->creature);
    case SV_POTION_RES_STR:
        return do_res_stat(this->creature, A_STR);
    case SV_POTION_RES_INT:
        return do_res_stat(this->creature, A_INT);
    case SV_POTION_RES_WIS:
        return do_res_stat(this->creature, A_WIS);
    case SV_POTION_RES_DEX:
        return do_res_stat(this->creature, A_DEX);
    case SV_POTION_RES_CON:
        return do_res_stat(this->creature, A_CON);
    case SV_POTION_RES_CHR:
        return do_res_stat(this->creature, A_CHR);
    case SV_POTION_INC_STR:
        return do_inc_stat(this->creature, A_STR);
    case SV_POTION_INC_INT:
        return do_inc_stat(this->creature, A_INT);
    case SV_POTION_INC_WIS:
        return do_inc_stat(this->creature, A_WIS);
    case SV_POTION_INC_DEX:
        return do_inc_stat(this->creature, A_DEX);
    case SV_POTION_INC_CON:
        return do_inc_stat(this->creature, A_CON);
    case SV_POTION_INC_CHR:
        return do_inc_stat(this->creature, A_CHR);
    case SV_POTION_POLY_SELF:
        do_poly_self(this->creature);
        return true;
    case SV_POTION_AUGMENTATION:
        return this->augmentation();
    case SV_POTION_ENLIGHTENMENT:
        return this->enlightenment();
    case SV_POTION_STAR_ENLIGHTENMENT:
        return this->star_enlightenment();
    case SV_POTION_SELF_KNOWLEDGE:
        msg_print(_("自分自身のことが少しは分かった気がする...", "You begin to know yourself a little better..."));
        msg_erase();
        self_knowledge(this->creature);
        return true;
    case SV_POTION_EXPERIENCE:
        return this->experience();
    case SV_POTION_RESISTANCE:
        return this->resistance();
    case SV_POTION_CURING:
        return true_healing(this->creature, 50);
    case SV_POTION_INVULNERABILITY:
        (void)set_invuln(this->creature, this->creature.get_timed_effect(CreatureTimedEffect::INVULNERABILITY) + randint1(4) + 4, false);
        return true;
    case SV_POTION_NEW_LIFE:
        return this->new_life();
    case SV_POSTION_MASARU_CHINISE_DYNAMIC:
        return this->neo_tsuyoshi();

    case SV_POTION_NEO_TSUYOSHI:
        msg_print(_("「新・オクレ兄さん！」", "NEW Brother OKURE!"));
        msg_erase();
        this->creature.set_timed_effect(CreatureTimedEffect::TSUYOSHI, 1);
        (void)set_tsuyoshi(this->creature, 0, true);
        if (!has_resist_chaos(this->creature)) {
            (void)BadStatusSetter(this->creature).mod_hallucination(50 + randint1(100));
        }
        return true;

    case SV_POTION_TSUYOSHI:
        return this->tsuyoshi();

    case SV_POTION_POLYMORPH:
        return this->polymorph();

    case SV_POTION_ICHIZIKU_ENEMA:
        if (is_rectal) {
            player_defecate(this->creature);
            return true;
        } else {
            msg_print(_("うぇ！思わず吐いてしまった。", "The potion makes you vomit!"));
            switch (CreatureRace(&this->creature).food()) {
            case PlayerRaceFoodType::RATION:
            case PlayerRaceFoodType::WATER:
            case PlayerRaceFoodType::BLOOD:
                (void)set_food(this->creature, PY_FOOD_STARVE - 1);
                break;
            default:
                break;
            }
            (void)BadStatusSetter(this->creature).set_poison(0);
            BadStatusSetter(this->creature).mod_paralysis(4);
            msg_print(_("この薬は直腸に注入するものらしい。", "The potion seems to be injected into the rectum"));
            return true;
        }
    case SV_POTION_MESUDACHI:
        return this->mesudachi();

    default:
        return false;
    }
}

/*!
 * @brief 塩水の薬
 * @return 常にtrue
 */
bool QuaffEffects::salt_water()
{
    msg_print(_("うぇ！思わず吐いてしまった。", "The potion makes you vomit!"));
    switch (CreatureRace(&this->creature).food()) {
    case PlayerRaceFoodType::RATION:
    case PlayerRaceFoodType::WATER:
    case PlayerRaceFoodType::BLOOD:
        (void)set_food(this->creature, PY_FOOD_STARVE - 1);
        break;
    default:
        break;
    }

    BadStatusSetter bss(this->creature);
    (void)bss.set_poison(0);
    (void)bss.mod_paralysis(4);
    return true;
}

/*!
 * @brief 毒の薬
 * @return 毒の効果を受けたらtrue
 */
bool QuaffEffects::poison()
{
    if (has_resist_pois(this->creature) || is_oppose_pois(this->creature)) {
        return false;
    }

    return BadStatusSetter(this->creature).mod_poison(randint0(15) + 10);
}

/*!
 * @brief 盲目の薬
 * @return 盲目になったらtrue
 */
bool QuaffEffects::blindness()
{
    if (has_resist_blind(this->creature)) {
        return false;
    }

    return BadStatusSetter(this->creature).mod_blindness(randint0(100) + 100);
}

/*!
 * @brief 酔っ払いの薬
 * @return カオス耐性があるかその他の一部確率でFALSE、それ以外はTRUE
 */
bool QuaffEffects::booze()
{
    auto ident = false;
    auto is_monk = CreatureClass(this->creature).equals(PlayerClassType::MONK);
    if (!is_monk) {
        chg_virtue(this->creature, Virtue::HARMONY, -1);
    } else if (!has_resist_conf(this->creature)) {
        set_bits(this->creature.special_attack, ATTACK_SUIKEN);
    }

    BadStatusSetter bss(this->creature);
    if (!has_resist_conf(this->creature) && bss.set_confusion(randint0(20) + 15)) {
        ident = true;
    }

    if (has_resist_chaos(this->creature)) {
        return ident;
    }

    if (one_in_(2) && bss.mod_hallucination(randint0(150) + 150)) {
        ident = true;
    }

    if (is_monk || !one_in_(13)) {
        return ident;
    }

    ident = true;
    if (one_in_(3)) {
        lose_all_info(this->creature);
    } else {
        wiz_dark(this->creature);
    }

    (void)teleport_player_aux(this->creature, 100, false, i2enum<teleport_flags>(TELEPORT_NONMAGICAL | TELEPORT_PASSIVE));
    wiz_dark(this->creature);
    msg_print(_("知らない場所で目が醒めた。頭痛がする。", "You wake up somewhere with a sore head..."));
    msg_print(_("何も思い出せない。どうやってここへ来たのかも分からない！", "You can't remember a thing or how you got here!"));
    return ident;
}

/*!
 * @brief 眠りの薬
 * @return 麻痺したか否か
 */
bool QuaffEffects::sleep()
{
    if (this->creature.has_free_act()) {
        return false;
    }

    msg_print(_("あなたは眠ってしまった。", "You fall asleep."));
    if (ironman_nightmare) {
        msg_print(_("恐ろしい光景が頭に浮かんできた。", "A horrible vision enters your mind."));
        sanity_blast(this->creature);
    }

    return BadStatusSetter(this->creature).mod_paralysis(randint0(4) + 4);
}

/*!
 * @brief 記憶喪失の薬
 * @return 経験値が下がったか
 */
bool QuaffEffects::lose_memories()
{
    if (this->creature.has_hold_exp() || (this->creature.exp <= 0)) {
        return false;
    }

    msg_print(_("過去の記憶が薄れていく気がする。", "You feel your memories fade."));
    chg_virtue(this->creature, Virtue::KNOWLEDGE, -5);
    lose_exp(this->creature, this->creature.exp / 4);
    return true;
}

/*!
 * @brief 破滅の薬
 * @return 常にtrue
 */
bool QuaffEffects::ruination()
{
    msg_print(_("身も心も弱ってきて、精気が抜けていくようだ。", "Your nerves and muscles feel weak and lifeless!"));
    take_hit(this->creature, DAMAGE_LOSELIFE, Dice::roll(10, 10), _("破滅の薬", "a potion of Ruination"));
    (void)dec_stat(this->creature, A_DEX, 25, true);
    (void)dec_stat(this->creature, A_WIS, 25, true);
    (void)dec_stat(this->creature, A_CON, 25, true);
    (void)dec_stat(this->creature, A_STR, 25, true);
    (void)dec_stat(this->creature, A_CHR, 25, true);
    (void)dec_stat(this->creature, A_INT, 25, true);
    return true;
}

/*!
 * @brief 爆発の薬 / Fumble ramble
 * @return 常にtrue
 */
bool QuaffEffects::detonation()
{
    msg_print(_("体の中で激しい爆発が起きた！", "Massive explosions rupture your body!"));
    take_hit(this->creature, DAMAGE_NOESCAPE, Dice::roll(50, 20), _("爆発の薬", "a potion of Detonation"));
    BadStatusSetter bss(this->creature);
    (void)bss.mod_stun(75);
    (void)bss.mod_cut(5000);
    return true;
}

/*!
 * @brief 死の薬
 * @return 常にtrue
 */
bool QuaffEffects::death()
{
    chg_virtue(this->creature, Virtue::VITALITY, -1);
    chg_virtue(this->creature, Virtue::UNLIFE, 5);
    msg_print(_("死の予感が体中を駆けめぐった。", "A feeling of Death flows through your body."));
    take_hit(this->creature, DAMAGE_LOSELIFE, 5000, _("死の薬", "a potion of Death"));
    return true;
}

/*!
 * @brief スピードの薬
 * @return 加速したらtrue、加速効果が切れていない状態で重ね飲みしたらfalse
 */
bool QuaffEffects::speed()
{
    if (this->creature.is_accelerated()) {
        (void)mod_acceleration(this->creature, 5, false);
        return false;
    }

    return set_acceleration(this->creature, randint1(25) + 15, false);
}

/*!
 * @brief 増強の薬
 * @return アビリティスコアのどれか1つでも向上したらtrue
 */
bool QuaffEffects::augmentation()
{
    auto ident = false;
    if (do_inc_stat(this->creature, A_STR)) {
        ident = true;
    }

    if (do_inc_stat(this->creature, A_INT)) {
        ident = true;
    }

    if (do_inc_stat(this->creature, A_WIS)) {
        ident = true;
    }

    if (do_inc_stat(this->creature, A_DEX)) {
        ident = true;
    }

    if (do_inc_stat(this->creature, A_CON)) {
        ident = true;
    }

    if (do_inc_stat(this->creature, A_CHR)) {
        ident = true;
    }

    return ident;
}

/*!
 * @brief 啓蒙の薬
 * @return 常にtrue
 */
bool QuaffEffects::enlightenment()
{
    msg_print(_("自分の置かれている状況が脳裏に浮かんできた...", "An image of your surroundings forms in your mind..."));
    chg_virtue(this->creature, Virtue::KNOWLEDGE, 1);
    chg_virtue(this->creature, Virtue::ENLIGHTEN, 1);
    wiz_lite(this->creature, false);
    return true;
}

/*!
 * @brief *啓蒙*の薬
 * @return 常にtrue
 */
bool QuaffEffects::star_enlightenment()
{
    msg_print(_("更なる啓蒙を感じた...", "You begin to feel more enlightened..."));
    chg_virtue(this->creature, Virtue::KNOWLEDGE, 1);
    chg_virtue(this->creature, Virtue::ENLIGHTEN, 2);
    msg_erase();
    wiz_lite(this->creature, false);
    (void)do_inc_stat(this->creature, A_INT);
    (void)do_inc_stat(this->creature, A_WIS);
    (void)detect_traps(this->creature, DETECT_RAD_DEFAULT, true);
    (void)detect_doors(this->creature, DETECT_RAD_DEFAULT);
    (void)detect_stairs(this->creature, DETECT_RAD_DEFAULT);
    (void)detect_treasure(this->creature, DETECT_RAD_DEFAULT);
    (void)detect_objects_gold(this->creature, DETECT_RAD_DEFAULT);
    (void)detect_objects_normal(this->creature, DETECT_RAD_DEFAULT);
    identify_pack(this->creature);
    self_knowledge(this->creature);
    return true;
}

/*!
 * @brief 経験の薬
 * @return 経験値が増えたらtrue
 */
bool QuaffEffects::experience()
{
    if (CreatureRace(&this->creature).equals(PlayerRaceType::ANDROID)) {
        return false;
    }

    chg_virtue(this->creature, Virtue::ENLIGHTEN, 1);
    if (this->creature.exp >= PY_MAX_EXP) {
        return false;
    }

    auto ee = (this->creature.exp / 2) + 10;
    constexpr int max_exp = 100000;
    if (ee > max_exp) {
        ee = max_exp;
    }

    msg_print(_("更に経験を積んだような気がする。", "You feel more experienced."));
    gain_exp(this->creature, ee);
    return true;
}

/*!
 * @brief 耐性の薬
 * @return 経験値が増えたらtrue
 */
bool QuaffEffects::resistance()
{
    (void)set_oppose_acid(this->creature, this->creature.get_timed_effect(CreatureTimedEffect::OPPOSE_ACID) + randint1(20) + 20, false);
    (void)set_oppose_elec(this->creature, this->creature.get_timed_effect(CreatureTimedEffect::OPPOSE_ELEC) + randint1(20) + 20, false);
    (void)set_oppose_fire(this->creature, this->creature.get_timed_effect(CreatureTimedEffect::OPPOSE_FIRE) + randint1(20) + 20, false);
    (void)set_oppose_cold(this->creature, this->creature.get_timed_effect(CreatureTimedEffect::OPPOSE_COLD) + randint1(20) + 20, false);
    (void)set_oppose_pois(this->creature, this->creature.get_timed_effect(CreatureTimedEffect::OPPOSE_POIS) + randint1(20) + 20, false);
    return true;
}

/*!
 * @brief 新生の薬
 * @return 常にtrue
 */
bool QuaffEffects::new_life()
{
    roll_hitdice(this->creature, SPOP_NONE);
    get_max_stats(this->creature);
    RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
    lose_all_mutations(this->creature);
    return true;
}

/*!
 * @brief ネオ・つよしスペシャルの薬
 * @return 常にtrue
 */
bool QuaffEffects::neo_tsuyoshi()
{
    (void)BadStatusSetter(this->creature).hallucination(0);
    (void)set_tsuyoshi(this->creature, this->creature.get_timed_effect(CreatureTimedEffect::TSUYOSHI) + randint1(100) + 100, false);
    return true;
}

/*!
 * @brief つよしスペシャルの薬
 * @return 常にtrue
 */
bool QuaffEffects::tsuyoshi()
{
    msg_print(_("「オクレ兄さん！」", "Brother OKURE!"));
    msg_erase();
    this->creature.set_timed_effect(CreatureTimedEffect::TSUYOSHI, 1);
    (void)set_tsuyoshi(this->creature, 0, true);
    if (!has_resist_chaos(this->creature)) {
        (void)BadStatusSetter(this->creature).hallucination(50 + randint1(50));
    }

    return true;
}

/*!
 * @brief 自己変容の薬
 * @return 突然変異を得たか失ったらtrue、そのままならfalse
 */
bool QuaffEffects::polymorph()
{
    if (this->creature.muta.any() && one_in_(23)) {
        lose_all_mutations(this->creature);
        return false;
    }

    auto ident = false;
    do {
        if (one_in_(2)) {
            if (gain_mutation(this->creature, 0)) {
                ident = true;
            }
        } else if (lose_mutation(this->creature, 0)) {
            ident = true;
        }
    } while (!ident || one_in_(2));
    return ident;
}

/*!
 * @brief メス堕ちの薬
 * @return 常にtrue
 */
bool QuaffEffects::mesudachi()
{
    msg_print(_("「女の子になっちゃう！」", "You become a girl!"));

    // 尻の穴技能が10d10増加
    int skill_gain = 0;
    for (int i = 0; i < 10; i++) {
        skill_gain += randint1(10);
    }
    this->creature.skill_exp[PlayerSkillKindType::ASSHOLE] += skill_gain;
    msg_format(_("尻の穴技能が%d向上した！", "Your asshole skill increased by %d!"), skill_gain);

    // 1/36の確率で性別が女になる（両性の場合は無効）
    if (this->creature.psex != SEX_BISEXUAL && one_in_(36)) {
        this->creature.psex = SEX_FEMALE;
        sp_ptr = &sex_info[this->creature.psex];
        msg_print(_("あなたは本当に女の子になってしまった！", "You have really become a girl!"));

        static constexpr auto flags = { StatusRecalculatingFlag::BONUS, StatusRecalculatingFlag::HP, StatusRecalculatingFlag::MP, StatusRecalculatingFlag::SPELLS };
        RedrawingFlagsUpdater::get_instance().set_flags(flags);
        static constexpr auto flags2 = {
            MainWindowRedrawingFlag::BASIC,
            MainWindowRedrawingFlag::HP,
            MainWindowRedrawingFlag::MP,
            MainWindowRedrawingFlag::ABILITY_SCORE,
        };
        RedrawingFlagsUpdater::get_instance().set_flags(flags2);
        static constexpr auto flags3 = { SubWindowRedrawingFlag::PLAYER };
        RedrawingFlagsUpdater::get_instance().set_flags(flags3);
    }

    return true;
}
