#include "status/shape-changer.h"
#include "autopick/autopick-reader-writer.h"
#include "avatar/avatar.h"
#include "birth/birth-body-spec.h"
#include "birth/birth-stat.h"
#include "core/disturbance.h"
#include "core/stuff-handler.h"
#include "game-option/disturbance-options.h"
#include "grid/grid.h"
#include "hpmp/hp-mp-processor.h"
#include "locale/english.h"
#include "mutation/mutation-investor-remover.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/class-info.h"
#include "player/player-damage.h"
#include "player/player-personality.h"
#include "player/player-sex.h"
#include "player/player-status.h"
#include "player/race-info-table.h"
#include "spell/spells-status.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "system/creature-entity.h"
#include "system/redrawing-flags-updater.h"
#include "timed-effect/timed-effects.h"
#include "util/enum-converter.h"
#include "view/display-messages.h"

void do_poly_wounds(CreatureEntity &creature)
{
    int16_t hit_p = (creature.maxhp - creature.hp);
    auto change = static_cast<TIME_EFFECT>(Dice::roll(creature.level, 5));
    auto nasty_effect = one_in_(5);
    const auto &player_cut = creature.effects()->cut();
    if (!player_cut.is_cut() && (hit_p == 0) && !nasty_effect) {
        return;
    }

    msg_print(_("傷がより軽いものに変化した。", "Your wounds are polymorphed into less serious ones."));
    hp_player(creature, change);
    BadStatusSetter bss(creature);
    if (!nasty_effect) {
        (void)bss.mod_cut(change / 2);
        return;
    }

    msg_print(_("新たな傷ができた！", "A new wound was created!"));
    take_hit(creature, DAMAGE_LOSELIFE, change / 2, _("変化した傷", "a polymorphed wound"));
    (void)bss.set_cut(change);
}

/*
 * Change player race
 */
void change_race(CreatureEntity &creature, PlayerRaceType new_race, concptr effect_msg)
{
    auto title = race_info[enum2i(new_race)].title.data();
    PlayerRaceType old_race = creature.prace;
#ifdef JP
    msg_format("あなたは%s%sに変化した！", effect_msg, title);
#else
    msg_format("You turn into %s %s%s!", (is_a_vowel((effect_msg[0]) ? effect_msg[0] : title[0]) ? "an" : "a"), effect_msg, title);
#endif

    chg_virtue(creature, Virtue::CHANCE, 2);
    if (enum2i(creature.prace) < 32) {
        creature.old_race1 |= 1UL << enum2i(creature.prace);
    } else {
        creature.old_race2 |= 1UL << (enum2i(creature.prace) - 32);
    }

    creature.prace = new_race;
    creature.race = &race_info[enum2i(creature.prace)];
    creature.expfact = creature.get_race_info()->r_exp + (*creature.get_class_info()).c_exp;

    CreatureClass pc(creature);
    bool is_special_class = pc.equals(PlayerClassType::MONK);
    is_special_class |= pc.equals(PlayerClassType::FORCETRAINER);
    is_special_class |= pc.equals(PlayerClassType::NINJA);
    CreatureRace pr(&creature);
    bool is_special_race = pr.equals(PlayerRaceType::KLACKON);
    is_special_race |= pr.equals(PlayerRaceType::SPRITE);
    if (is_special_class && is_special_race) {
        creature.expfact -= 15;
    }

    get_height_weight(creature);

    const auto r_mhp = pc.equals(PlayerClassType::SORCERER) ? creature.get_race_info()->r_mhp / 2 : creature.get_race_info()->r_mhp;
    creature.hit_dice = Dice(1, r_mhp + (*creature.get_class_info()).c_mhp + (*creature.get_personality_info()).a_mhp);

    roll_hitdice(creature, SPOP_NONE);
    check_experience(creature);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::BASIC);
    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(creature);

    if (old_race != creature.prace) {
        autopick_load_pref(creature, false);
    }

    lite_spot(creature, creature.get_position());
}

void do_poly_self(CreatureEntity &creature)
{
    int power = creature.level;

    msg_print(_("あなたは変化の訪れを感じた...", "You feel a change coming over you..."));
    chg_virtue(creature, Virtue::CHANCE, 1);

    CreatureRace pr(&creature);
    if ((power > randint0(20)) && one_in_(3) && !pr.equals(PlayerRaceType::ANDROID)) {
        char effect_msg[80] = "";
        char sex_msg[32] = "";
        PlayerRaceType new_race;

        power -= 10;
        if ((power > randint0(5)) && one_in_(4)) {
            power -= 2;
            if (creature.psex == SEX_MALE) {
                creature.psex = SEX_FEMALE;
                sp_ptr = &sex_info[creature.psex];
                sprintf(sex_msg, _("女性の", "female"));
            } else {
                creature.psex = SEX_MALE;
                sp_ptr = &sex_info[creature.psex];
                sprintf(sex_msg, _("男性の", "male"));
            }
        }

        if ((power > randint0(30)) && one_in_(5)) {
            int tmp = 0;
            power -= 15;
            while (tmp < A_MAX) {
                if (one_in_(2)) {
                    (void)dec_stat(creature, tmp, randint1(6) + 6, one_in_(3));
                    power -= 1;
                }
                tmp++;
            }

            (void)dec_stat(creature, A_CHR, randint1(6), true);

            if (sex_msg[0]) {
                sprintf(effect_msg, _("奇形の%s", "deformed %s "), sex_msg);
            } else {
                sprintf(effect_msg, _("奇形の", "deformed "));
            }
        }

        while ((power > randint0(20)) && one_in_(10)) {
            power -= 10;

            if (!lose_mutation(creature, 0)) {
                msg_print(_("奇妙なくらい普通になった気がする。", "You feel oddly normal."));
            }
        }

        do {
            new_race = randnum0<PlayerRaceType>(MAX_RACES);
        } while (pr.equals(new_race) || (new_race == PlayerRaceType::ANDROID));

        change_race(creature, new_race, effect_msg);
    }

    if ((power > randint0(30)) && one_in_(6)) {
        int tmp = 0;
        power -= 20;
        msg_format(_("%sの構成が変化した！", "Your internal organs are rearranged!"), pr.equals(PlayerRaceType::ANDROID) ? "機械" : "内臓");

        while (tmp < A_MAX) {
            (void)dec_stat(creature, tmp, randint1(6) + 6, one_in_(3));
            tmp++;
        }
        if (one_in_(6)) {
            msg_print(_("現在の姿で生きていくのは困難なようだ！", "You find living difficult in your present form!"));
            take_hit(creature, DAMAGE_LOSELIFE, Dice::roll(randint1(10), creature.level), _("致命的な突然変異", "a lethal mutation"));

            power -= 10;
        }
    }

    if ((power > randint0(20)) && one_in_(4)) {
        power -= 10;

        get_max_stats(creature);
        roll_hitdice(creature, SPOP_NONE);
    }

    while ((power > randint0(15)) && one_in_(3)) {
        power -= 7;
        (void)gain_mutation(creature, 0);
    }

    if (power > randint0(5)) {
        power -= 5;
        do_poly_wounds(creature);
    }

    while (power > 0) {
        status_shuffle(creature);
        power--;
    }
}
