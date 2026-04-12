#include "mutation/mutation-processor.h"
#include "core/asking-player.h"
#include "core/disturbance.h"
#include "effect/attribute-types.h"
#include "floor/geometry.h"
#include "grid/grid.h"
#include "hpmp/hp-mp-processor.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-requester.h"
#include "main/sound-of-music.h"
#include "monster-attack/monster-attack-player.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-status.h"
#include "mutation/mutation-flag-types.h"
#include "mutation/mutation-investor-remover.h"
#include "object/lite-processor.h"
#include "object/tval-types.h"
#include "player-info/equipment-info.h"
#include "player/digestion-processor.h"
#include "player/player-damage.h"
#include "player/player-status-flags.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-song.h"
#include "spell/summon-types.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "status/shape-changer.h"
#include "status/sight-setter.h"
#include "store/store-owners.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/angband-system.h"
#include "system/creature-entity.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "target/target-setter.h"
#include "target/target-types.h"
#include "term/screen-processor.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"
#include "world/world-collapsion.h"
#include "world/world.h"

/*!
 * @brief 10ゲームターンが進行するごとに敵対的存在の襲撃を判定する処置
 */
void process_world_aux_sudden_attack(CreatureEntity &creature)
{
    if (randint1(100) == 36) {
        for (auto a : alliance_list) {
            a.second->panishment(creature);
        }
    }

    if (wc_ptr->get_collapsion_parcentage() > 66) {
        auto pow = 100 - wc_ptr->get_collapsion_parcentage();
        if (pow < 1) {
            pow = 1;
        } else if (pow > 30) {
            pow = 30;
        }

        if (randint1(1000) < 364 / pow) {
            switch (randint1(3)) {
            case 1:
                msg_print(_("「いやよ～お世界こわれる～」", "No, no, the world is breaking!"));
                break;
            case 2:
                msg_print(_("「こわれるぅ～Foo~」", "Breaking, Foo~"));
                break;
            case 3:
                msg_print(_("「お世界が壊れるわ」（しんみり）", "The world is breaking..."));
                break;
            }
            if (one_in_(5)) {
                summon_specific(creature, creature.y, creature.x, creature.current_floor_ptr->dun_level, SUMMON_KACHO, PM_IGNORE_LEVEL | PM_NO_KAGE);
            }
        }
    }
}

/*!
 * @brief 10ゲームターンが進行するごとに突然変異の発動判定を行う処理
 * / Handle mutation effects once every 10 game turns
 */
void process_world_aux_mutation(CreatureEntity &creature)
{
    if (creature.muta.none() || AngbandSystem::get_instance().is_phase_out() || AngbandWorld::get_instance().is_wild_mode()) {
        return;
    }

    BadStatusSetter bss(creature);
    if (creature.muta.has(PlayerMutationType::BERS_RAGE) && one_in_(3000)) {
        disturb(creature, false, true);
        msg_print(_("ウガァァア！", "RAAAAGHH!"));
        msg_print(_("激怒の発作に襲われた！", "You feel a fit of rage coming over you!"));
        (void)set_berserk(creature, 10 + randint1(creature.level), false);
        (void)bss.set_fear(0);
    }

    if (creature.muta.has(PlayerMutationType::COWARDICE) && (randint1(3000) == 13)) {
        if (!has_resist_fear(creature)) {
            disturb(creature, false, true);
            msg_print(_("とても暗い... とても恐い！", "It's so dark... so scary!"));
            (void)bss.mod_fear(13 + randint1(26));
        }
    }

    if (creature.muta.has(PlayerMutationType::RTELEPORT) && (randint1(5000) == 88)) {
        if (!has_resist_shard(creature) && creature.muta.has_not(PlayerMutationType::VTELEPORT) && !creature.anti_tele) {
            disturb(creature, false, true);
            msg_print(_("あなたの位置は突然ひじょうに不確定になった...", "Your position suddenly seems very uncertain..."));
            msg_erase();
            teleport_player(creature, 40, TELEPORT_PASSIVE);
        }
    }

    if (creature.muta.has(PlayerMutationType::ALCOHOL) && (randint1(6400) == 321)) {
        if (!has_resist_conf(creature) && !has_resist_chaos(creature)) {
            disturb(creature, false, true);
            RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::EXTRA);
            msg_print(_("いひきがもーろーとひてきたきがふる...ヒック！", "You feel a SSSCHtupor cOmINg over yOu... *HIC*!"));
        }

        if (!has_resist_conf(creature)) {
            (void)bss.mod_confusion(randint0(20) + 15);
        }

        if (!has_resist_chaos(creature)) {
            if (one_in_(20)) {
                msg_erase();
                if (one_in_(3)) {
                    lose_all_info(creature);
                } else {
                    wiz_dark(creature);
                }
                (void)teleport_player_aux(creature, 100, false, i2enum<teleport_flags>(TELEPORT_NONMAGICAL | TELEPORT_PASSIVE));
                wiz_dark(creature);
                msg_print(_("あなたは見知らぬ場所で目が醒めた...頭が痛い。", "You wake up somewhere with a sore head..."));
                msg_print(_("何も覚えていない。どうやってここに来たかも分からない！", "You can't remember a thing or how you got here!"));
            } else {
                if (one_in_(3)) {
                    msg_print(_("き～れいなちょおちょらとんれいる～", "Thishcischs GooDSChtuff!"));
                    (void)bss.mod_hallucination(randint0(150) + 150);
                }
            }
        }
    }

    if (creature.muta.has(PlayerMutationType::HALLU) && (randint1(6400) == 42)) {
        if (!has_resist_chaos(creature)) {
            disturb(creature, false, true);
            RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::EXTRA);
            (void)bss.mod_hallucination(randint0(50) + 20);
        }
    }

    if (creature.muta.has(PlayerMutationType::FLATULENT) && (randint1(3000) == 13)) {
        disturb(creature, false, true);
        msg_print(_("ブゥーーッ！おっと。", "BRRAAAP! Oops."));
        msg_erase();
        fire_ball(creature, AttributeType::POIS, Direction::self(), creature.level, 3);
    }

    if (creature.muta.has(PlayerMutationType::IKISUGI) && (randint1(3000) == 13)) {
        disturb(creature, false, true);
        msg_print(_("ンアアアアー！", "NAAAAAAAH!"));
        msg_erase();
        fire_ball(creature, AttributeType::SOUND, Direction::self(), creature.level, 3);
        aggravate_monsters(creature, 0);
    }

    if (creature.muta.has(PlayerMutationType::DEFECATION) && (randint1(1500) == 13)) {
        player_defecate(creature);
    }

    if (creature.muta.has(PlayerMutationType::ZEERO_VIRUS) && (randint1(721) == 1)) {
        msg_print(_("SEX!DAAAAAAAAAAAA!", "SEX!DAAAAAAAAAAAA!"));
        msg_erase();
        disturb(creature, false, true);
        const auto flags = {
            MainWindowRedrawingFlag::EXTRA,
        };
        RedrawingFlagsUpdater::get_instance().set_flags(flags);

        (void)bss.mod_hallucination(randint0(10) + 20);
        (void)set_berserk(creature, 10 + randint1(creature.level), false);
        (void)set_acceleration(creature, 10 + randint1(creature.level), false);
    }

    if (creature.muta.has(PlayerMutationType::PROD_MANA) && !creature.anti_magic && one_in_(9000)) {
        disturb(creature, false, true);
        msg_print(_("魔法のエネルギーが突然あなたの中に流れ込んできた！エネルギーを解放しなければならない！",
            "Magical energy flows through you! You must release it!"));

        flush();
        msg_erase();
        const auto dir = get_aim_dir(creature, false);
        fire_ball(creature, AttributeType::MANA, dir ? dir : Direction::self(), creature.level * 2, 3);
    }

    if (creature.muta.has(PlayerMutationType::ATT_DEMON) && !creature.anti_magic && (randint1(6666) == 666)) {
        bool pet = one_in_(6);
        BIT_FLAGS mode = PM_ALLOW_GROUP;

        if (pet) {
            mode |= PM_FORCE_PET;
        } else {
            mode |= (PM_ALLOW_UNIQUE | PM_NO_PET);
        }

        if (summon_specific(creature, creature.y, creature.x, creature.current_floor_ptr->dun_level, SUMMON_DEMON, mode)) {
            msg_print(_("あなたはデーモンを引き寄せた！", "You have attracted a demon!"));
            disturb(creature, false, true);
        }
    }

    if (creature.muta.has(PlayerMutationType::ATT_NASTY) && !creature.anti_magic && (randint1(6666) == 666)) {
        bool pet = one_in_(6);
        BIT_FLAGS mode = PM_ALLOW_GROUP;

        if (pet) {
            mode |= PM_FORCE_PET;
        } else {
            mode |= (PM_ALLOW_UNIQUE | PM_NO_PET);
        }

        if (summon_specific(creature, creature.y, creature.x, creature.current_floor_ptr->dun_level, SUMMON_NASTY, mode)) {
            msg_print(_("あなたはクッソ汚い輩を引き寄せた！", "You have attracted nasty creatures!"));
            disturb(creature, false, true);
        }
    }

    if (has_pervert_attraction(creature) && !creature.anti_magic && (randint1(6666) == 666)) {
        bool pet = one_in_(6);
        BIT_FLAGS mode = PM_ALLOW_GROUP;

        if (pet) {
            mode |= PM_FORCE_PET;
        } else {
            mode |= (PM_ALLOW_UNIQUE | PM_NO_PET);
        }

        if (summon_specific(creature, creature.y, creature.x, creature.current_floor_ptr->dun_level, SUMMON_PERVERTS, mode)) {
            msg_print(_("あなたは変質者を引き寄せた！", "You have attracted perverts!"));
            disturb(creature, false, true);
        }
    }

    if (creature.muta.has(PlayerMutationType::SPEED_FLUX) && one_in_(6000)) {
        disturb(creature, false, true);
        if (one_in_(2)) {
            msg_print(_("精力的でなくなった気がする。", "You feel less energetic."));
            if (creature.effects()->acceleration().is_fast()) {
                set_acceleration(creature, 0, true);
            } else {
                (void)bss.set_deceleration(randint1(30) + 10, false);
            }
        } else {
            msg_print(_("精力的になった気がする。", "You feel more energetic."));
            if (creature.effects()->deceleration().is_slow()) {
                (void)bss.set_deceleration(0, true);
            } else {
                set_acceleration(creature, randint1(30) + 10, false);
            }
        }

        msg_erase();
    }

    if (creature.muta.has(PlayerMutationType::BANISH_ALL) && one_in_(9000)) {
        disturb(creature, false, true);
        msg_print(_("突然ほとんど孤独になった気がする。", "You suddenly feel almost lonely."));

        banish_monsters(creature, 100);
        msg_erase();
    }

    if (creature.muta.has(PlayerMutationType::EAT_LIGHT) && one_in_(3000)) {
        msg_print(_("影につつまれた。", "A shadow passes over you."));
        msg_erase();

        if ((creature.current_floor_ptr->grid_array[creature.y][creature.x].info & (CAVE_GLOW | CAVE_MNDK)) == CAVE_GLOW) {
            hp_player(creature, 10);
        }

        auto &item = *creature.inventory[INVEN_LITE];
        if (item.bi_key.tval() == ItemKindType::LITE) {
            if (!item.is_fixed_artifact() && (item.fuel > 0)) {
                hp_player(creature, item.fuel / 20);
                item.fuel /= 2;
                msg_print(_("光源からエネルギーを吸収した！", "You absorb energy from your light!"));
                notice_lite_change(creature, &item);
            }
        }

        if (creature.tim_emission > 0) {
            hp_player(creature, creature.tim_emission);
            set_tim_emission(creature, 0, true);
            msg_print(_("あなたは自身の光をエネルギーとして吸収した！", "You absorb energy from your own light!"));
        }

        /*
         * Unlite the area (radius 10) around player and
         * do 50 points damage to every affected monster
         */
        unlite_area(creature, 50, 10);
    }

    if (creature.muta.has(PlayerMutationType::ATT_ANIMAL) && !creature.anti_magic && one_in_(7000)) {
        bool pet = one_in_(3);
        BIT_FLAGS mode = PM_ALLOW_GROUP;

        if (pet) {
            mode |= PM_FORCE_PET;
        } else {
            mode |= (PM_ALLOW_UNIQUE | PM_NO_PET);
        }

        if (summon_specific(creature, creature.y, creature.x, creature.current_floor_ptr->dun_level, SUMMON_ANIMAL, mode)) {
            msg_print(_("動物を引き寄せた！", "You have attracted an animal!"));
            disturb(creature, false, true);
        }
    }

    if (creature.muta.has(PlayerMutationType::RAW_CHAOS) && !creature.anti_magic && one_in_(8000)) {
        disturb(creature, false, true);
        msg_print(_("周りの空間が歪んでいる気がする！", "You feel the world warping around you!"));
        msg_erase();
        fire_ball(creature, AttributeType::CHAOS, Direction::self(), creature.level, 8);
    }

    if (creature.muta.has(PlayerMutationType::NORMALITY) && one_in_(5000)) {
        if (!lose_mutation(creature, 0)) {
            msg_print(_("奇妙なくらい普通になった気がする。", "You feel oddly normal."));
        }
    }

    if (creature.muta.has(PlayerMutationType::WRAITH) && !creature.anti_magic && one_in_(3000)) {
        disturb(creature, false, true);
        msg_print(_("非物質化した！", "You feel insubstantial!"));
        msg_erase();
        set_wraith_form(creature, randint1(creature.level / 2) + (creature.level / 2), false);
    }

    if (creature.muta.has(PlayerMutationType::POLY_WOUND) && one_in_(3000)) {
        do_poly_wounds(creature);
    }

    if (creature.muta.has(PlayerMutationType::WASTING) && one_in_(3000)) {
        int which_stat = randint0(A_MAX);
        int sustained = false;

        switch (which_stat) {
        case A_STR:
            if (has_sustain_str(creature)) {
                sustained = true;
            }
            break;
        case A_INT:
            if (has_sustain_int(creature)) {
                sustained = true;
            }
            break;
        case A_WIS:
            if (has_sustain_wis(creature)) {
                sustained = true;
            }
            break;
        case A_DEX:
            if (has_sustain_dex(creature)) {
                sustained = true;
            }
            break;
        case A_CON:
            if (has_sustain_con(creature)) {
                sustained = true;
            }
            break;
        case A_CHR:
            if (has_sustain_chr(creature)) {
                sustained = true;
            }
            break;
        default:
            msg_print(_("不正な状態！", "Invalid stat chosen!"));
            sustained = true;
        }

        if (!sustained) {
            disturb(creature, false, true);
            msg_print(_("自分が衰弱していくのが分かる！", "You can feel yourself wasting away!"));
            msg_erase();
            (void)dec_stat(creature, which_stat, randint1(6) + 6, one_in_(3));
        }
    }

    if (creature.muta.has(PlayerMutationType::ATT_DRAGON) && !creature.anti_magic && one_in_(3000)) {
        bool pet = one_in_(5);
        BIT_FLAGS mode = PM_ALLOW_GROUP;
        if (pet) {
            mode |= PM_FORCE_PET;
        } else {
            mode |= (PM_ALLOW_UNIQUE | PM_NO_PET);
        }

        if (summon_specific(creature, creature.y, creature.x, creature.current_floor_ptr->dun_level, SUMMON_DRAGON, mode)) {
            msg_print(_("ドラゴンを引き寄せた！", "You have attracted a dragon!"));
            disturb(creature, false, true);
        }
    }

    if (creature.muta.has(PlayerMutationType::WEIRD_MIND) && !creature.anti_magic && one_in_(3000)) {
        if (creature.tim_esp > 0) {
            msg_print(_("精神にもやがかかった！", "Your mind feels cloudy!"));
            set_tim_esp(creature, 0, true);
        } else {
            msg_print(_("精神が広がった！", "Your mind expands!"));
            set_tim_esp(creature, creature.level, false);
        }
    }

    if (creature.muta.has(PlayerMutationType::NAUSEA) && !creature.slow_digest && one_in_(9000)) {
        disturb(creature, false, true);
        msg_print(_("胃が痙攣し、食事を失った！", "Your stomach roils, and you lose your lunch!"));
        msg_erase();
        set_food(creature, PY_FOOD_WEAK);
        if (music_singing_any(creature)) {
            stop_singing(creature);
        }

        SpellHex spell_hex(creature);
        if (spell_hex.is_spelling_any()) {
            (void)spell_hex.stop_all_spells();
        }
    }

    if (creature.muta.has(PlayerMutationType::WALK_SHAD) && !creature.anti_magic && one_in_(12000) && !creature.current_floor_ptr->inside_arena) {
        reserve_alter_reality(creature, randint0(21) + 15);
    }

    if (creature.muta.has(PlayerMutationType::WARNING) && one_in_(1000)) {
        int danger_amount = 0;
        for (auto m_idx = 0; m_idx < creature.current_floor_ptr->m_max; m_idx++) {
            const auto &monster = creature.current_floor_ptr->get_monster(m_idx);
            const auto &monrace = monster.get_monrace();
            if (!monster.is_valid()) {
                continue;
            }

            if (monrace.level >= creature.level) {
                danger_amount += monrace.level - creature.level + 1;
            }
        }

        if (danger_amount > 100) {
            msg_print(_("非常に恐ろしい気がする！", "You feel utterly terrified!"));
        } else if (danger_amount > 50) {
            msg_print(_("恐ろしい気がする！", "You feel terrified!"));
        } else if (danger_amount > 20) {
            msg_print(_("非常に心配な気がする！", "You feel very worried!"));
        } else if (danger_amount > 10) {
            msg_print(_("心配な気がする！", "You feel paranoid!"));
        } else if (danger_amount > 5) {
            msg_print(_("ほとんど安全な気がする。", "You feel almost safe."));
        } else {
            msg_print(_("寂しい気がする。", "You feel lonely."));
        }
    }

    if (creature.muta.has(PlayerMutationType::INVULN) && !creature.anti_magic && one_in_(5000)) {
        disturb(creature, false, true);
        msg_print(_("無敵な気がする！", "You feel invincible!"));
        msg_erase();
        (void)set_invuln(creature, randint1(8) + 8, false);
    }

    static constexpr auto flags = {
        MainWindowRedrawingFlag::HP,
        MainWindowRedrawingFlag::MP,
    };
    if (creature.muta.has(PlayerMutationType::SP_TO_HP) && one_in_(2000)) {
        MANA_POINT wounds = (MANA_POINT)(creature.maxhp - creature.hp);
        if (wounds > 0) {
            int healing = creature.csp;
            if (healing > wounds) {
                healing = wounds;
            }

            hp_player(creature, healing);
            creature.csp -= healing;
            RedrawingFlagsUpdater::get_instance().set_flags(flags);
        }
    }

    if (creature.muta.has(PlayerMutationType::HP_TO_SP) && !creature.anti_magic && one_in_(4000)) {
        int wounds = (int)(creature.msp - creature.csp);
        if (wounds > 0) {
            int healing = creature.hp;
            if (healing > wounds) {
                healing = wounds;
            }

            creature.csp += healing;
            RedrawingFlagsUpdater::get_instance().set_flags(flags);
            take_hit(creature, DAMAGE_LOSELIFE, healing, _("頭に昇った血", "blood rushing to the head"));
        }
    }

    if (creature.muta.has(PlayerMutationType::DISARM) && one_in_(10000)) {
        disturb(creature, false, true);
        msg_print(_("足がもつれて転んだ！", "You trip over your own feet!"));
        take_hit(creature, DAMAGE_NOESCAPE, randint1(creature.wt / 6), _("転倒", "tripping"));
        drop_weapons(creature);
    }
}

bool drop_weapons(CreatureEntity &creature)
{
    INVENTORY_IDX slot = 0;
    ItemEntity *o_ptr = nullptr;

    if (AngbandWorld::get_instance().is_wild_mode()) {
        return false;
    }

    msg_erase();
    if (has_melee_weapon(creature, INVEN_MAIN_HAND)) {
        slot = INVEN_MAIN_HAND;
        o_ptr = creature.inventory[INVEN_MAIN_HAND].get();

        if (has_melee_weapon(creature, INVEN_SUB_HAND) && one_in_(2)) {
            o_ptr = creature.inventory[INVEN_SUB_HAND].get();
            slot = INVEN_SUB_HAND;
        }
    } else if (has_melee_weapon(creature, INVEN_SUB_HAND)) {
        o_ptr = creature.inventory[INVEN_SUB_HAND].get();
        slot = INVEN_SUB_HAND;
    }

    if ((slot == 0) || o_ptr->is_cursed()) {
        return false;
    }

    msg_print(_("武器を落としてしまった！", "You drop your weapon!"));
    drop_from_inventory(creature, slot, 1);
    return true;
}
