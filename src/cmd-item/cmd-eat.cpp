﻿/*!
 * @brief プレイヤーの食べるコマンド実装
 * @date 2018/09/07
 @ @author deskull
 */

#include "cmd-item/cmd-eat.h"
#include "avatar/avatar.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "hpmp/hp-mp-processor.h"
#include "inventory/inventory-object.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags9.h"
#include "object-enchant/special-object-flags.h"
#include "object-hook/hook-expendable.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-info.h"
#include "object/object-kind-hook.h"
#include "object/object-kind.h"
#include "perception/object-perception.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/digestion-processor.h"
#include "player/eldritch-horror.h"
#include "player/mimic-info-table.h"
#include "player/player-class.h"
#include "player/player-damage.h"
#include "player/player-race-types.h"
#include "player/player-status-flags.h"
#include "player/special-defense-types.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-song.h"
#include "spell/spells-status.h"
#include "spell/spell-types.h"
#include "spell-kind/spells-launcher.h"
#include "status/action-setter.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "status/experience.h"
#include "status/shape-changer.h"
#include "sv-definition/sv-food-types.h"
#include "sv-definition/sv-junk-types.h"
#include "sv-definition/sv-other-types.h"
#include "system/monster-race-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "view/object-describer.h"

/*!
 * @brief ゴミみてえなものを食べたときの効果を発動
 * @param creature_ptr プレイヤー情報への参照ポインタ
 * @param o_ptr 食べるオブジェクト
 * @return 鑑定されるならTRUE、されないならFALSE
 */
static bool exe_eat_junk_type_object(player_type *creature_ptr, object_type *o_ptr)
{
    if (o_ptr->tval != TV_JUNK)
        return false;

    switch (o_ptr->sval) {
    case SV_JUNK_FECES:
        msg_print("ワーォ！貴方は糞を喰った！");
        msg_print("『涙が出るほどうめぇ……』");
        if (!(has_resist_pois(creature_ptr) || is_oppose_pois(creature_ptr))) {
            set_poisoned(creature_ptr, creature_ptr->poisoned + randint0(10) + 10);
        }
        if (creature_ptr->incident.count(INCIDENT::EAT_FECES) == 0) {
            creature_ptr->incident[INCIDENT::EAT_FECES] = 0;
        }
        creature_ptr->incident[INCIDENT::EAT_FECES]++;
        return true;
        break;
    case SV_JUNK_VOMITTING:
        msg_print("ワーォ！貴方はゲロを喰った！");
        msg_print("『涙が出るほどうめぇ……』");
        if (!(has_resist_pois(creature_ptr) || is_oppose_pois(creature_ptr))) {
            set_poisoned(creature_ptr, creature_ptr->poisoned + randint0(10) + 10);
        }
        if (creature_ptr->incident.count(INCIDENT::EAT_FECES) == 0) {
            creature_ptr->incident[INCIDENT::EAT_FECES] = 0;
        }
        creature_ptr->incident[INCIDENT::EAT_FECES]++;
        return true;
        break;
    }
    return false;
}

/*!
 * @brief ソウルを食べたときの効果を発動
 * @param creature_ptr プレイヤー情報への参照ポインタ
 * @param o_ptr 食べるオブジェクト
 * @return 鑑定されるならTRUE、されないならFALSE
 */
static bool exe_eat_soul(player_type *creature_ptr, object_type *o_ptr)
{
    if (!(o_ptr->tval == TV_CORPSE && o_ptr->sval == SV_SOUL))
        return false;

    if (creature_ptr->prace == player_race_type::ANDROID)
        return false;

    monster_race *r_ptr = &r_info[o_ptr->pval];
    EXP max_exp = r_ptr->level * r_ptr->level * 10;

    chg_virtue(creature_ptr, V_ENLIGHTEN, 1);
    if (creature_ptr->exp < PY_MAX_EXP) {
        EXP ee = (creature_ptr->exp / 2) + 10;
        if (ee > max_exp)
            ee = max_exp;
        msg_print(_("更に経験を積んだような気がする。", "You feel more experienced."));
        gain_exp(creature_ptr, ee);
    }
    return true;
}

/*!
 * @brief 死体を食べたときの効果を発動
 * @param creature_ptr プレイヤー情報への参照ポインタ
 * @param o_ptr 食べるオブジェクト
 * @return 鑑定されるならTRUE、されないならFALSE
 */
static bool exe_eat_corpse_type_object(player_type *creature_ptr, object_type *o_ptr)
{
    if (!(o_ptr->tval == TV_CORPSE && o_ptr->sval == SV_CORPSE))
        return false;

    monster_race *r_ptr = &r_info[o_ptr->pval];

    if (r_ptr->flags9 & RF9_EAT_BLIND) {
        set_blind(creature_ptr, creature_ptr->blind + 200 + randint1(200));
    }

    if (r_ptr->flags9 & RF9_EAT_CONF) {
        set_confused(creature_ptr, creature_ptr->confused + 200 + randint1(200));
    }

    if (r_ptr->flags9 & RF9_EAT_MANA) {
        restore_mana(creature_ptr, false);
    }

    if (r_ptr->flags9 & RF9_EAT_NEXUS) {
        do_poly_self(creature_ptr);
    }

    if (r_ptr->flags9 & RF9_EAT_SLEEP) {
        if (!creature_ptr->free_act)
            set_paralyzed(creature_ptr, creature_ptr->paralyzed + randint0(10) + 10);
    }

    if (r_ptr->flags9 & RF9_EAT_BERSERKER) {
        set_shero(creature_ptr, creature_ptr->shero + randint1(10) + 10, false);
    }

    if (r_ptr->flags9 & RF9_EAT_ACIDIC) {
    }

    if (r_ptr->flags9 & RF9_EAT_SPEED) {
        (void)set_fast(creature_ptr, randint1(20) + 20, false);
    }

    if (r_ptr->flags9 & RF9_EAT_CURE) {
        true_healing(creature_ptr, 50);
    }

    if (r_ptr->flags9 & RF9_EAT_FIRE_RES) {
        set_oppose_fire(creature_ptr, randint1(20) + 20, false);
    }

    if (r_ptr->flags9 & RF9_EAT_COLD_RES) {
        set_oppose_cold(creature_ptr, randint1(20) + 20, false);
    }

    if (r_ptr->flags9 & RF9_EAT_ELEC_RES) {
        set_oppose_elec(creature_ptr, randint1(20) + 20, false);
    }

    if (r_ptr->flags9 & RF9_EAT_POIS_RES) {
        set_oppose_pois(creature_ptr, randint1(20) + 20, false);
    }

    if (r_ptr->flags9 & RF9_EAT_INSANITY) {
        sanity_blast(creature_ptr, NULL, false);
    }

    if (r_ptr->flags9 & RF9_EAT_DRAIN_EXP) {
    }

    if (r_ptr->flags9 & RF9_EAT_POISONOUS) {
        if (!(has_resist_pois(creature_ptr) || is_oppose_pois(creature_ptr))) {
            set_poisoned(creature_ptr, creature_ptr->poisoned + randint0(15) + 10);
        }
    }

    if (r_ptr->flags9 & RF9_EAT_GIVE_STR) {
        do_inc_stat(creature_ptr, A_STR);
    }

    if (r_ptr->flags9 & RF9_EAT_GIVE_INT) {
        do_inc_stat(creature_ptr, A_INT);
    }

    if (r_ptr->flags9 & RF9_EAT_GIVE_WIS) {
        do_inc_stat(creature_ptr, A_WIS);
    }

    if (r_ptr->flags9 & RF9_EAT_GIVE_DEX) {
        do_inc_stat(creature_ptr, A_DEX);
    }

    if (r_ptr->flags9 & RF9_EAT_GIVE_CON) {
        do_inc_stat(creature_ptr, A_CON);
    }

    if (r_ptr->flags9 & RF9_EAT_GIVE_CHR) {
        do_inc_stat(creature_ptr, A_CHR);
    }

    if (r_ptr->flags9 & RF9_EAT_LOSE_STR) {
        do_dec_stat(creature_ptr, A_STR);
    }

    if (r_ptr->flags9 & RF9_EAT_LOSE_INT) {
        do_dec_stat(creature_ptr, A_INT);
    }

    if (r_ptr->flags9 & RF9_EAT_LOSE_WIS) {
        do_dec_stat(creature_ptr, A_WIS);
    }

    if (r_ptr->flags9 & RF9_EAT_LOSE_DEX) {
        do_dec_stat(creature_ptr, A_DEX);
    }

    if (r_ptr->flags9 & RF9_EAT_LOSE_CON) {
        do_dec_stat(creature_ptr, A_CON);
    }

    if (r_ptr->flags9 & RF9_EAT_LOSE_CHR) {
        do_dec_stat(creature_ptr, A_CHR);
    }

    if (r_ptr->flags9 & RF9_EAT_DRAIN_MANA) {
        creature_ptr->csp -= 30;
        if (creature_ptr->csp < 0) {
            creature_ptr->csp = 0;
            creature_ptr->csp_frac = 0;
        }
    }

    return true;
}

/*!
 * @brief 食料タイプの食料を食べたときの効果を発動
 * @param creature_ptr プレイヤー情報への参照ポインタ
 * @param o_ptr 食べるオブジェクト
 * @return 鑑定されるならTRUE、されないならFALSE
 */
bool exe_eat_food_type_object(player_type *creature_ptr, object_type *o_ptr)
{

    if (o_ptr->tval != TV_FOOD)
        return false;

    switch (o_ptr->sval) {
    case SV_FOOD_POISON:
        if (!(has_resist_pois(creature_ptr) || is_oppose_pois(creature_ptr))) {
            if (set_poisoned(creature_ptr, creature_ptr->poisoned + randint0(10) + 10)) {
                if (creature_ptr->incident.count(INCIDENT::EAT_POISON) == 0) {
                    creature_ptr->incident[INCIDENT::EAT_POISON] = 0;
                }
                creature_ptr->incident[INCIDENT::EAT_POISON]++;
                return true;
            }
        }
        break;
    case SV_FOOD_BLINDNESS:
        if (!has_resist_blind(creature_ptr)) {
            if (set_blind(creature_ptr, creature_ptr->blind + randint0(200) + 200)) {
                if (creature_ptr->incident.count(INCIDENT::EAT_POISON) == 0) {
                    creature_ptr->incident[INCIDENT::EAT_POISON] = 0;
                }
                creature_ptr->incident[INCIDENT::EAT_POISON]++;
                return true;
            }
        }
        break;
    case SV_FOOD_PARANOIA:
        if (!has_resist_fear(creature_ptr)) {
            if (set_afraid(creature_ptr, creature_ptr->afraid + randint0(10) + 10)) {
                if (creature_ptr->incident.count(INCIDENT::EAT_POISON) == 0) {
                    creature_ptr->incident[INCIDENT::EAT_POISON] = 0;
                }
                creature_ptr->incident[INCIDENT::EAT_POISON]++;
                return true;
            }
        }
        break;
    case SV_FOOD_CONFUSION:
        if (!has_resist_conf(creature_ptr)) {
            if (set_confused(creature_ptr, creature_ptr->confused + randint0(10) + 10)) {
                if (creature_ptr->incident.count(INCIDENT::EAT_POISON) == 0) {
                    creature_ptr->incident[INCIDENT::EAT_POISON] = 0;
                }
                creature_ptr->incident[INCIDENT::EAT_POISON]++;
                return true;        
            }
        }
        break;
    case SV_FOOD_HALLUCINATION:
        if (!has_resist_chaos(creature_ptr)) {
            if (set_image(creature_ptr, creature_ptr->image + randint0(250) + 250)) {
                if (creature_ptr->incident.count(INCIDENT::EAT_POISON) == 0) {
                    creature_ptr->incident[INCIDENT::EAT_POISON] = 0;
                }
                creature_ptr->incident[INCIDENT::EAT_POISON]++;
                return true;
            }
        }
        break;
    case SV_FOOD_PARALYSIS:
        if (!creature_ptr->free_act) {
            if (set_paralyzed(creature_ptr, creature_ptr->paralyzed + randint0(10) + 10)) {
                if (creature_ptr->incident.count(INCIDENT::EAT_POISON) == 0) {
                    creature_ptr->incident[INCIDENT::EAT_POISON] = 0;
                }
                creature_ptr->incident[INCIDENT::EAT_POISON]++;
                return true;
            }        
        }
        break;
    case SV_FOOD_WEAKNESS:
        if (creature_ptr->incident.count(INCIDENT::EAT_POISON) == 0) {
            creature_ptr->incident[INCIDENT::EAT_POISON] = 0;
        }
        creature_ptr->incident[INCIDENT::EAT_POISON]++;
        take_hit(creature_ptr, DAMAGE_NOESCAPE, damroll(6, 6), _("毒入り食料", "poisonous food"));
        (void)do_dec_stat(creature_ptr, A_STR);
        return true;
    case SV_FOOD_SICKNESS:
        if (creature_ptr->incident.count(INCIDENT::EAT_POISON) == 0) {
            creature_ptr->incident[INCIDENT::EAT_POISON] = 0;
        }
        creature_ptr->incident[INCIDENT::EAT_POISON]++;
        take_hit(creature_ptr, DAMAGE_NOESCAPE, damroll(6, 6), _("毒入り食料", "poisonous food"));
        (void)do_dec_stat(creature_ptr, A_CON);
        return true;
    case SV_FOOD_STUPIDITY:
        if (creature_ptr->incident.count(INCIDENT::EAT_POISON) == 0) {
            creature_ptr->incident[INCIDENT::EAT_POISON] = 0;
        }
        creature_ptr->incident[INCIDENT::EAT_POISON]++;
        take_hit(creature_ptr, DAMAGE_NOESCAPE, damroll(8, 8), _("毒入り食料", "poisonous food"));
        (void)do_dec_stat(creature_ptr, A_INT);
        return true;
    case SV_FOOD_NAIVETY:
        if (creature_ptr->incident.count(INCIDENT::EAT_POISON) == 0) {
            creature_ptr->incident[INCIDENT::EAT_POISON] = 0;
        }
        creature_ptr->incident[INCIDENT::EAT_POISON]++;
        take_hit(creature_ptr, DAMAGE_NOESCAPE, damroll(8, 8), _("毒入り食料", "poisonous food"));
        (void)do_dec_stat(creature_ptr, A_WIS);
        return true;
    case SV_FOOD_UNHEALTH:
        if (creature_ptr->incident.count(INCIDENT::EAT_POISON) == 0) {
            creature_ptr->incident[INCIDENT::EAT_POISON] = 0;
        }
        creature_ptr->incident[INCIDENT::EAT_POISON]++;
        take_hit(creature_ptr, DAMAGE_NOESCAPE, damroll(10, 10), _("毒入り食料", "poisonous food"));
        (void)do_dec_stat(creature_ptr, A_CON);
        return true;
    case SV_FOOD_DISEASE:
        if (creature_ptr->incident.count(INCIDENT::EAT_POISON) == 0) {
            creature_ptr->incident[INCIDENT::EAT_POISON] = 0;
        }
        creature_ptr->incident[INCIDENT::EAT_POISON]++;
        take_hit(creature_ptr, DAMAGE_NOESCAPE, damroll(10, 10), _("毒入り食料", "poisonous food"));
        (void)do_dec_stat(creature_ptr, A_STR);
        return true;
    case SV_FOOD_CURE_POISON:
        if (set_poisoned(creature_ptr, 0))
            return true;
        break;
    case SV_FOOD_CURE_BLINDNESS:
        if (set_blind(creature_ptr, 0))
            return true;
        break;
    case SV_FOOD_CURE_PARANOIA:
        if (set_afraid(creature_ptr, 0))
            return true;
        break;
    case SV_FOOD_CURE_CONFUSION:
        if (set_confused(creature_ptr, 0))
            return true;
        break;
    case SV_FOOD_CURE_SERIOUS:
        return cure_serious_wounds(creature_ptr, 4, 8);
    case SV_FOOD_RESTORE_STR:
        if (do_res_stat(creature_ptr, A_STR))
            return true;
        break;
    case SV_FOOD_RESTORE_CON:
        if (do_res_stat(creature_ptr, A_CON))
            return true;
        break;
    case SV_FOOD_RESTORING:
        return restore_all_status(creature_ptr);
#ifdef JP
    /* それぞれの食べ物の感想をオリジナルより細かく表現 */
    case SV_FOOD_BISCUIT:
    case SV_FOOD_COOKIE:
        msg_print("甘くてサクサクしてとてもおいしい。");
        return true;
    case SV_FOOD_JERKY:
        msg_print("歯ごたえがあっておいしい。");
        return true;
    case SV_FOOD_SLIME_MOLD:
        msg_print("これはなんとも形容しがたい味だ。");
        return true;
    case SV_FOOD_BROWNIW_OF_ALC:
        msg_print("実際に美味で「しっとりとしていて、それでいてべたつかないスッキリとした甘さ」ではあった。");
        return true;
    case SV_FOOD_RATION:
        msg_print("これはおいしい。");
        return true;
#else
    case SV_FOOD_RATION:
    case SV_FOOD_BISCUIT:
    case SV_FOOD_JERKY:
    case SV_FOOD_SLIME_MOLD:
        msg_print("That tastes good.");
        return true;
#endif
    case SV_FOOD_WAYBREAD:
        msg_print(_("これはひじょうに美味だ。", "That tastes good."));
        (void)set_poisoned(creature_ptr, 0);
        (void)hp_player(creature_ptr, damroll(4, 8));
        return true;
    case SV_FOOD_PINT_OF_ALE:
    case SV_FOOD_PINT_OF_WINE:
        msg_print(_("のどごし爽やかだ。", "That tastes good."));
        return true;
    case SV_FOOD_WELCOME_DRINK_OF_ARE:
    case SV_FOOD_ABA_TEA:
        if (creature_ptr->incident.count(INCIDENT::EAT_POISON) == 0) {
            creature_ptr->incident[INCIDENT::EAT_POISON] = 0;
        }
        creature_ptr->incident[INCIDENT::EAT_POISON]++;
        (void)set_poisoned(creature_ptr, 10);
        msg_print("「非常に新鮮で……非常においしい……」");
        if (creature_ptr->incident.count(INCIDENT::EAT_FECES) == 0) {
            creature_ptr->incident[INCIDENT::EAT_FECES] = 0;
        }
        creature_ptr->incident[INCIDENT::EAT_FECES]++;
        return true;
    case SV_FOOD_SEED_FEA:
        msg_print("脱穀して炊いた方が良かったかもしれないが、多少空腹は収まった。");
        return true;
    case SV_FOOD_HIP:
        if (creature_ptr->incident.count(INCIDENT::EAT_POISON) == 0) {
            creature_ptr->incident[INCIDENT::EAT_POISON] = 0;
        }
        creature_ptr->incident[INCIDENT::EAT_POISON]++;
        msg_print("ヴォエ！食ったら尻の肉だった！");
        msg_print(NULL);
        (void)set_poisoned(creature_ptr, 10);
        msg_print("「作者は広告で収入得てないけど、こんな卑猥なアイテム放置するなよ」");
        msg_print(NULL);
        if (creature_ptr->incident.count(INCIDENT::EAT_FECES) == 0) {
            creature_ptr->incident[INCIDENT::EAT_FECES] = 0;
        }
        creature_ptr->incident[INCIDENT::EAT_FECES]++;
        return true;
    case SV_FOOD_SURSTROMMING:
        msg_print("悪臭が周囲を取り巻いた！");
        msg_print(NULL);
        fire_ball(creature_ptr, GF_POIS, 0, 30, 4);
        (void)set_poisoned(creature_ptr, 10);
        return true;
    case SV_FOOD_HOMOTEA:
        (void)set_stun(creature_ptr, 200);
        msg_print("「お、大丈夫か？大丈夫か？……」");
        return true;
    case SV_FOOD_GOLDEN_EGG:
        (void)do_inc_stat(creature_ptr, randint0(6));
        return true;
    case SV_FOOD_ABESHI:
        gain_exp(creature_ptr, creature_ptr->lev * 50);
        (void)set_hero(creature_ptr, randint1(10) + 10, false);
        if (one_in_(300)) {
            (void)do_inc_stat(creature_ptr, A_STR);
        }
        if (one_in_(300)) {
            (void)do_inc_stat(creature_ptr, A_DEX);
        }
        if (one_in_(300)) {
            (void)do_inc_stat(creature_ptr, A_CON);
        }
        return true;
    case SV_FOOD_HIDEBU:
        gain_exp(creature_ptr, creature_ptr->lev * 100);
        (void)set_hero(creature_ptr, randint1(25) + 25, false);
        if (one_in_(100)) {
            (void)do_inc_stat(creature_ptr, A_STR);
        }
        if (one_in_(100)) {
            (void)do_inc_stat(creature_ptr, A_DEX);
        }
        if (one_in_(100)) {
            (void)do_inc_stat(creature_ptr, A_CON);
        }
        return true;
    default:
        return true;
    }
    return false;
}

/*!
 * @brief 魔法道具のチャージをの食料として食べたときの効果を発動
 * @param creature_ptr プレイヤー情報への参照ポインタ
 * @param o_ptr 食べるオブジェクト
 * @param item オブジェクトのインベントリ番号
 * @return 食べようとしたらTRUE、しなかったらFALSE
 */
bool exe_eat_charge_of_magic_device(player_type *creature_ptr, object_type *o_ptr, INVENTORY_IDX item)
{
    if (o_ptr->tval != TV_STAFF && o_ptr->tval != TV_WAND)
        return false;

    if (player_race_food(creature_ptr) == PlayerRaceFood::MANA) {
        concptr staff;

        if (o_ptr->tval == TV_STAFF && (item < 0) && (o_ptr->number > 1)) {
            msg_print(_("まずは杖を拾わなければ。", "You must first pick up the staffs."));
            return true;
        }

        staff = (o_ptr->tval == TV_STAFF) ? _("杖", "staff") : _("魔法棒", "wand");

        /* "Eat" charges */
        if (o_ptr->pval == 0) {
            msg_format(_("この%sにはもう魔力が残っていない。", "The %s has no charges left."), staff);
            o_ptr->ident |= (IDENT_EMPTY);
            creature_ptr->window_flags |= (PW_INVEN);
            return true;
        }

        msg_format(_("あなたは%sの魔力をエネルギー源として吸収した。", "You absorb mana of the %s as your energy."), staff);

        /* Use a single charge */
        o_ptr->pval--;

        /* Eat a charge */
        set_food(creature_ptr, creature_ptr->food + 5000);

        /* XXX Hack -- unstack if necessary */
        if (o_ptr->tval == TV_STAFF && (item >= 0) && (o_ptr->number > 1)) {
            object_type forge;
            object_type *q_ptr;
            q_ptr = &forge;
            q_ptr->copy_from(o_ptr);

            /* Modify quantity */
            q_ptr->number = 1;

            /* Restore the charges */
            o_ptr->pval++;

            /* Unstack the used item */
            o_ptr->number--;
            item = store_item_to_inventory(creature_ptr, q_ptr);

            msg_format(_("杖をまとめなおした。", "You unstack your staff."));
        }

        if (item >= 0) {
            inven_item_charges(creature_ptr, item);
        } else {
            floor_item_charges(creature_ptr->current_floor_ptr, 0 - item);
        }

        creature_ptr->window_flags |= (PW_INVEN | PW_EQUIP);
        return true;
    }

    return false;
}

/*!
 * @brief 実際にアイテムを食おうとするコマンドのサブルーチン
 * @param item 食べるオブジェクトの所持品ID
 */
void exe_eat_food(player_type *creature_ptr, INVENTORY_IDX item)
{
    if (music_singing_any(creature_ptr))
        stop_singing(creature_ptr);
    if (hex_spelling_any(creature_ptr))
        stop_hex_spell_all(creature_ptr);

    object_type *o_ptr = ref_item(creature_ptr, item);

    sound(SOUND_EAT);

    PlayerEnergy(creature_ptr).set_player_turn_energy(100);

    /* Object level */
    int lev = k_info[o_ptr->k_idx].level;

    /* 基本食い物でないものを喰う判定 */
    bool ate = false;
    ate = exe_eat_soul(creature_ptr, o_ptr);
    if (!ate)
        ate = exe_eat_corpse_type_object(creature_ptr, o_ptr);
    if (!ate)
        ate = exe_eat_junk_type_object(creature_ptr, o_ptr);

    /* Identity not known yet */
    int ident;
    ident = exe_eat_food_type_object(creature_ptr, o_ptr);

    /*
     * Store what may have to be updated for the inventory (including
     * autodestroy if set by something else).  Then turn off those flags
     * so that updates triggered by calling gain_exp() or set_food() below
     * do not rearrange the inventory before the food item is destroyed in
     * the pack.
     */
    BIT_FLAGS inventory_flags = (PU_COMBINE | PU_REORDER | (creature_ptr->update & PU_AUTODESTROY));
    creature_ptr->update &= ~(PU_COMBINE | PU_REORDER | PU_AUTODESTROY);

    if (!(object_is_aware(o_ptr))) {
        chg_virtue(creature_ptr, V_KNOWLEDGE, -1);
        chg_virtue(creature_ptr, V_PATIENCE, -1);
        chg_virtue(creature_ptr, V_CHANCE, 1);
    }

    /* We have tried it */
    if (o_ptr->tval == TV_FOOD)
        object_tried(o_ptr);

    /* The player is now aware of the object */
    if (ident && !object_is_aware(o_ptr)) {
        object_aware(creature_ptr, o_ptr);
        gain_exp(creature_ptr, (lev + (creature_ptr->lev >> 1)) / creature_ptr->lev);
    }

    creature_ptr->window_flags |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

    /* Undeads drain recharge of magic device */
    if (exe_eat_charge_of_magic_device(creature_ptr, o_ptr, item)) {
        creature_ptr->update |= inventory_flags;
        return;
    }

    auto food_type = player_race_food(creature_ptr);

    /* Balrogs change humanoid corpses to energy */
    if (food_type == PlayerRaceFood::CORPSE && (o_ptr->tval == TV_CORPSE && o_ptr->sval == SV_CORPSE && angband_strchr("pht", r_info[o_ptr->pval].d_char))) {
        GAME_TEXT o_name[MAX_NLEN];
        describe_flavor(creature_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
        msg_format(_("%sは燃え上り灰になった。精力を吸収した気がする。", "%^s is burnt to ashes.  You absorb its vitality!"), o_name);
        (void)set_food(creature_ptr, PY_FOOD_MAX - 1);

        creature_ptr->update |= inventory_flags;
        vary_item(creature_ptr, item, -1);
        return;
    }

    if (o_ptr->tval == TV_FOOD) {
        if (is_specific_player_race(creature_ptr, player_race_type::SKELETON)) {
            if (!((o_ptr->sval == SV_FOOD_WAYBREAD) || (o_ptr->sval < SV_FOOD_BISCUIT))) {
                object_type forge;
                object_type *q_ptr = &forge;

                msg_print(_("食べ物がアゴを素通りして落ちた！", "The food falls through your jaws!"));
                q_ptr->prep(lookup_kind(o_ptr->tval, o_ptr->sval));

                /* Drop the object from heaven */
                (void)drop_near(creature_ptr, q_ptr, -1, creature_ptr->y, creature_ptr->x);

                ate = true;
            } else {
                msg_print(_("食べ物がアゴを素通りして落ち、消えた！", "The food falls through your jaws and vanishes!"));
                ate = true;
            }
        } else if (food_type == PlayerRaceFood::BLOOD) {
            /* Vampires are filled only by bloods, so reduced nutritional benefit */
            (void)set_food(creature_ptr, creature_ptr->food + (o_ptr->pval / 10));
            msg_print(_("あなたのような者にとって食糧など僅かな栄養にしかならない。", "Mere victuals hold scant sustenance for a being such as yourself."));

            if (creature_ptr->food < PY_FOOD_ALERT) /* Hungry */
                msg_print(_("あなたの飢えは新鮮な血によってのみ満たされる！", "Your hunger can only be satisfied with fresh blood!"));
            ate = true;

        } else if (food_type == PlayerRaceFood::WATER) {
            msg_print(_("動物の食物はあなたにとってほとんど栄養にならない。", "The food of animals is poor sustenance for you."));
            set_food(creature_ptr, creature_ptr->food + ((o_ptr->pval) / 20));
            ate = true;
        } else if (food_type != PlayerRaceFood::RATION) {
            msg_print(_("生者の食物はあなたにとってほとんど栄養にならない。", "The food of mortals is poor sustenance for you."));
            set_food(creature_ptr, creature_ptr->food + ((o_ptr->pval) / 20));
            ate = true;
        } else {
            if (o_ptr->tval == TV_FOOD && o_ptr->sval == SV_FOOD_WAYBREAD) {
                /* Waybread is always fully satisfying. */
                set_food(creature_ptr, MAX(creature_ptr->food, PY_FOOD_MAX - 1));
            } else {
                /* Food can feed the player */
                (void)set_food(creature_ptr, creature_ptr->food + o_ptr->pval);
            }
            ate = true;
        }    
    }

    if (!ate) {
        msg_print("流石に食べるのを躊躇した。");
        return;
    }

    if (creature_ptr->incident.count(INCIDENT::EAT) == 0) {
        creature_ptr->incident[INCIDENT::EAT] = 0;
    }
    creature_ptr->incident[INCIDENT::EAT]++;

    creature_ptr->update |= inventory_flags;
    vary_item(creature_ptr, item, -1);
}

/*!
 * @brief 食料を食べるコマンドのメインルーチン /
 * Eat some food (from the pack or floor)
 */
void do_cmd_eat_food(player_type *creature_ptr)
{
    OBJECT_IDX item;
    concptr q, s;

    if (creature_ptr->special_defense & (KATA_MUSOU | KATA_KOUKIJIN)) {
        set_action(creature_ptr, ACTION_NONE);
    }

    q = _("どれを食べますか? ", "Eat which item? ");
    s = _("食べ物がない。", "You have nothing to eat.");

    if (!choose_object(creature_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), FuncItemTester(item_tester_hook_eatable, creature_ptr)))
        return;

    exe_eat_food(creature_ptr, item);
}
