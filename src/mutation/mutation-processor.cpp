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
#include "monster/monster-describer.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "mutation/mutation-flag-types.h"
#include "mutation/mutation-investor-remover.h"
#include "object/lite-processor.h"
#include "object/tval-types.h"
#include "player-info/equipment-info.h"
#include "player/digestion-processor.h"
#include "player/player-damage.h"
#include "player/player-sex.h"
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
#include "system/creature-timed-effect-types.h"
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

    // とくさんレイドイベント (#2094)
    // 男性プレイヤーに極低確率で発生する。「とくさんか？」の声に Y/N どちらで答えても、
    // 敵対的な男性ホモ (HOMO_SEXUAL かつ MALE) のモンスターに襲撃される。
    constexpr auto tokusan_raid_chance = 100000;
    if (creature.is_player() && (creature.get_psex() == SEX_MALE) && one_in_(tokusan_raid_chance)) {
        disturb(creature, true, true);
        msg_print(_("「とくさんか？」", "A voice asks, \"Are you Tokusan?\""));
        (void)input_check(_("とくさんか？", "Are you Tokusan?"));
        msg_print(_("どう答えても、もう遅かった……！", "It was already too late, whatever you answered...!"));
        auto summoned = false;
        for (auto i = 0; i < 1 + randint1(2); i++) {
            if (summon_specific(creature, creature.y, creature.x, 100, SUMMON_TOKUSAN, PM_AMBUSH | PM_ALLOW_GROUP)) {
                summoned = true;
            }
        }
        if (summoned) {
            msg_print(_("敵対的なとくさんに襲撃された！", "You are raided by hostile Tokusans!"));
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
                summon_specific(creature, creature.y, creature.x, creature.get_floor()->dun_level, SUMMON_KACHO, PM_IGNORE_LEVEL | PM_NO_KAGE);
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
    if (creature.get_mutations().none() || AngbandSystem::get_instance().is_phase_out() || AngbandWorld::get_instance().is_wild_mode()) {
        return;
    }

    BadStatusSetter bss(creature);
    if (creature.get_mutations().has(PlayerMutationType::BERS_RAGE) && one_in_(3000)) {
        disturb(creature, false, true);
        msg_print(_("ウガァァア！", "RAAAAGHH!"));
        msg_print(_("激怒の発作に襲われた！", "You feel a fit of rage coming over you!"));
        (void)set_berserk(creature, 10 + randint1(creature.get_level()), false);
        (void)bss.set_fear(0);
    }

    if (creature.get_mutations().has(PlayerMutationType::COWARDICE) && (randint1(3000) == 13)) {
        if (!creature.has_resist_fear()) {
            disturb(creature, false, true);
            msg_print(_("とても暗い... とても恐い！", "It's so dark... so scary!"));
            (void)bss.mod_fear(13 + randint1(26));
        }
    }

    if (creature.get_mutations().has(PlayerMutationType::RTELEPORT) && (randint1(5000) == 88)) {
        if (!creature.has_resist_shard() && creature.get_mutations().has_not(PlayerMutationType::VTELEPORT) && !creature.has_anti_tele()) {
            disturb(creature, false, true);
            msg_print(_("あなたの位置は突然ひじょうに不確定になった...", "Your position suddenly seems very uncertain..."));
            msg_erase();
            teleport_player(creature, 40, TELEPORT_PASSIVE);
        }
    }

    if (creature.get_mutations().has(PlayerMutationType::ALCOHOL) && (randint1(6400) == 321)) {
        if (!creature.has_resist_conf() && !creature.has_resist_chaos()) {
            disturb(creature, false, true);
            RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::EXTRA);
            msg_print(_("いひきがもーろーとひてきたきがふる...ヒック！", "You feel a SSSCHtupor cOmINg over yOu... *HIC*!"));
        }

        if (!creature.has_resist_conf()) {
            (void)bss.mod_confusion(randint0(20) + 15);
        }

        if (!creature.has_resist_chaos()) {
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

    if (creature.get_mutations().has(PlayerMutationType::HALLU) && (randint1(6400) == 42)) {
        if (!creature.has_resist_chaos()) {
            disturb(creature, false, true);
            RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::EXTRA);
            (void)bss.mod_hallucination(randint0(50) + 20);
        }
    }

    if (creature.get_mutations().has(PlayerMutationType::FLATULENT) && (randint1(3000) == 13)) {
        disturb(creature, false, true);
        msg_print(_("ブゥーーッ！おっと。", "BRRAAAP! Oops."));
        msg_erase();
        fire_ball(creature, AttributeType::POIS, Direction::self(), creature.get_level(), 3);
    }

    if (creature.get_mutations().has(PlayerMutationType::IKISUGI) && (randint1(3000) == 13)) {
        disturb(creature, false, true);
        msg_print(_("ンアアアアー！", "NAAAAAAAH!"));
        msg_erase();
        fire_ball(creature, AttributeType::SOUND, Direction::self(), creature.get_level(), 3);
        aggravate_monsters(creature, 0);
    }

    if (creature.get_mutations().has(PlayerMutationType::DEFECATION) && (randint1(1500) == 13)) {
        player_defecate(creature);
    }

    if (creature.get_mutations().has(PlayerMutationType::ZEERO_VIRUS) && (randint1(721) == 1)) {
        msg_print(_("SEX!DAAAAAAAAAAAA!", "SEX!DAAAAAAAAAAAA!"));
        msg_erase();
        disturb(creature, false, true);
        const auto flags = {
            MainWindowRedrawingFlag::EXTRA,
        };
        RedrawingFlagsUpdater::get_instance().set_flags(flags);

        (void)bss.mod_hallucination(randint0(10) + 20);
        (void)set_berserk(creature, 10 + randint1(creature.get_level()), false);
        (void)set_acceleration(creature, 10 + randint1(creature.get_level()), false);
    }

    if (creature.get_mutations().has(PlayerMutationType::PROD_MANA) && !creature.has_anti_magic() && one_in_(9000)) {
        disturb(creature, false, true);
        msg_print(_("魔法のエネルギーが突然あなたの中に流れ込んできた！エネルギーを解放しなければならない！",
            "Magical energy flows through you! You must release it!"));

        flush();
        msg_erase();
        const auto dir = get_aim_dir(creature, false);
        fire_ball(creature, AttributeType::MANA, dir ? dir : Direction::self(), creature.get_level() * 2, 3);
    }

    if (creature.get_mutations().has(PlayerMutationType::ATT_DEMON) && !creature.has_anti_magic() && (randint1(6666) == 666)) {
        bool pet = one_in_(6);
        BIT_FLAGS mode = PM_ALLOW_GROUP;

        if (pet) {
            mode |= PM_FORCE_PET;
        } else {
            mode |= (PM_ALLOW_UNIQUE | PM_NO_PET);
        }

        if (summon_specific(creature, creature.y, creature.x, creature.get_floor()->dun_level, SUMMON_DEMON, mode)) {
            msg_print(_("あなたはデーモンを引き寄せた！", "You have attracted a demon!"));
            disturb(creature, false, true);
        }
    }

    if (creature.get_mutations().has(PlayerMutationType::ATT_NASTY) && !creature.has_anti_magic() && (randint1(6666) == 666)) {
        bool pet = one_in_(6);
        BIT_FLAGS mode = PM_ALLOW_GROUP;

        if (pet) {
            mode |= PM_FORCE_PET;
        } else {
            mode |= (PM_ALLOW_UNIQUE | PM_NO_PET);
        }

        if (summon_specific(creature, creature.y, creature.x, creature.get_floor()->dun_level, SUMMON_NASTY, mode)) {
            msg_print(_("あなたはクッソ汚い輩を引き寄せた！", "You have attracted nasty creatures!"));
            disturb(creature, false, true);
        }
    }

    if (has_pervert_attraction(creature) && !creature.has_anti_magic() && (randint1(6666) == 666)) {
        bool pet = one_in_(6);
        BIT_FLAGS mode = PM_ALLOW_GROUP;

        if (pet) {
            mode |= PM_FORCE_PET;
        } else {
            mode |= (PM_ALLOW_UNIQUE | PM_NO_PET);
        }

        if (summon_specific(creature, creature.y, creature.x, creature.get_floor()->dun_level, SUMMON_PERVERTS, mode)) {
            msg_print(_("あなたは変質者を引き寄せた！", "You have attracted perverts!"));
            disturb(creature, false, true);
        }
    }

    if (creature.get_mutations().has(PlayerMutationType::SPEED_FLUX) && one_in_(6000)) {
        disturb(creature, false, true);
        if (one_in_(2)) {
            msg_print(_("精力的でなくなった気がする。", "You feel less energetic."));
            if (creature.is_accelerated()) {
                set_acceleration(creature, 0, true);
            } else {
                (void)bss.set_deceleration(randint1(30) + 10, false);
            }
        } else {
            msg_print(_("精力的になった気がする。", "You feel more energetic."));
            if (creature.is_decelerated()) {
                (void)bss.set_deceleration(0, true);
            } else {
                set_acceleration(creature, randint1(30) + 10, false);
            }
        }

        msg_erase();
    }

    if (creature.get_mutations().has(PlayerMutationType::BANISH_ALL) && one_in_(9000)) {
        disturb(creature, false, true);
        msg_print(_("突然ほとんど孤独になった気がする。", "You suddenly feel almost lonely."));

        banish_monsters(creature, 100);
        msg_erase();
    }

    if (creature.get_mutations().has(PlayerMutationType::EAT_LIGHT) && one_in_(3000)) {
        msg_print(_("影につつまれた。", "A shadow passes over you."));
        msg_erase();

        if ((creature.get_floor()->grid_array[creature.y][creature.x].info & (CAVE_GLOW | CAVE_MNDK)) == CAVE_GLOW) {
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

        if (creature.get_timed_effect(CreatureTimedEffect::TIM_EMISSION) > 0) {
            hp_player(creature, creature.get_timed_effect(CreatureTimedEffect::TIM_EMISSION));
            set_tim_emission(creature, 0, true);
            msg_print(_("あなたは自身の光をエネルギーとして吸収した！", "You absorb energy from your own light!"));
        }

        /*
         * Unlite the area (radius 10) around player and
         * do 50 points damage to every affected monster
         */
        unlite_area(creature, 50, 10);
    }

    if (creature.get_mutations().has(PlayerMutationType::ATT_ANIMAL) && !creature.has_anti_magic() && one_in_(7000)) {
        bool pet = one_in_(3);
        BIT_FLAGS mode = PM_ALLOW_GROUP;

        if (pet) {
            mode |= PM_FORCE_PET;
        } else {
            mode |= (PM_ALLOW_UNIQUE | PM_NO_PET);
        }

        if (summon_specific(creature, creature.y, creature.x, creature.get_floor()->dun_level, SUMMON_ANIMAL, mode)) {
            msg_print(_("動物を引き寄せた！", "You have attracted an animal!"));
            disturb(creature, false, true);
        }
    }

    if (creature.get_mutations().has(PlayerMutationType::RAW_CHAOS) && !creature.has_anti_magic() && one_in_(8000)) {
        disturb(creature, false, true);
        msg_print(_("周りの空間が歪んでいる気がする！", "You feel the world warping around you!"));
        msg_erase();
        fire_ball(creature, AttributeType::CHAOS, Direction::self(), creature.get_level(), 8);
    }

    if (creature.get_mutations().has(PlayerMutationType::NORMALITY) && one_in_(5000)) {
        if (!lose_mutation(creature, 0)) {
            msg_print(_("奇妙なくらい普通になった気がする。", "You feel oddly normal."));
        }
    }

    if (creature.get_mutations().has(PlayerMutationType::WRAITH) && !creature.has_anti_magic() && one_in_(3000)) {
        disturb(creature, false, true);
        msg_print(_("非物質化した！", "You feel insubstantial!"));
        msg_erase();
        set_wraith_form(creature, randint1(creature.get_level() / 2) + (creature.get_level() / 2), false);
    }

    if (creature.get_mutations().has(PlayerMutationType::POLY_WOUND) && one_in_(3000)) {
        do_poly_wounds(creature);
    }

    if (creature.get_mutations().has(PlayerMutationType::WASTING) && one_in_(3000)) {
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

    if (creature.get_mutations().has(PlayerMutationType::ATT_DRAGON) && !creature.has_anti_magic() && one_in_(3000)) {
        bool pet = one_in_(5);
        BIT_FLAGS mode = PM_ALLOW_GROUP;
        if (pet) {
            mode |= PM_FORCE_PET;
        } else {
            mode |= (PM_ALLOW_UNIQUE | PM_NO_PET);
        }

        if (summon_specific(creature, creature.y, creature.x, creature.get_floor()->dun_level, SUMMON_DRAGON, mode)) {
            msg_print(_("ドラゴンを引き寄せた！", "You have attracted a dragon!"));
            disturb(creature, false, true);
        }
    }

    if (creature.get_mutations().has(PlayerMutationType::WEIRD_MIND) && !creature.has_anti_magic() && one_in_(3000)) {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_ESP) > 0) {
            msg_print(_("精神にもやがかかった！", "Your mind feels cloudy!"));
            set_tim_esp(creature, 0, true);
        } else {
            msg_print(_("精神が広がった！", "Your mind expands!"));
            set_tim_esp(creature, creature.get_level(), false);
        }
    }

    if (creature.get_mutations().has(PlayerMutationType::NAUSEA) && !creature.has_slow_digest_flag() && one_in_(9000)) {
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

    if (creature.get_mutations().has(PlayerMutationType::WALK_SHAD) && !creature.has_anti_magic() && one_in_(12000) && !creature.get_floor()->inside_arena) {
        reserve_alter_reality(creature, randint0(21) + 15);
    }

    if (creature.get_mutations().has(PlayerMutationType::WARNING) && one_in_(1000)) {
        int danger_amount = 0;
        for (auto m_idx = 0; m_idx < creature.get_floor()->m_max; m_idx++) {
            const auto &monster = creature.get_floor()->get_monster(static_cast<MONSTER_IDX>(m_idx));
            const auto &monrace = monster.get_monrace();
            if (!monster.is_valid()) {
                continue;
            }

            if (monrace.level >= creature.get_level()) {
                danger_amount += monrace.level - creature.get_level() + 1;
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

    if (creature.get_mutations().has(PlayerMutationType::INVULN) && !creature.has_anti_magic() && one_in_(5000)) {
        disturb(creature, false, true);
        msg_print(_("無敵な気がする！", "You feel invincible!"));
        msg_erase();
        (void)set_invuln(creature, randint1(8) + 8, false);
    }

    static constexpr auto flags = {
        MainWindowRedrawingFlag::HP,
        MainWindowRedrawingFlag::MP,
    };
    if (creature.get_mutations().has(PlayerMutationType::SP_TO_HP) && one_in_(2000)) {
        MANA_POINT wounds = (MANA_POINT)(creature.maxhp - creature.hp);
        if (wounds > 0) {
            int healing = creature.get_current_mp();
            if (healing > wounds) {
                healing = wounds;
            }

            hp_player(creature, healing);
            creature.sub_current_mp(healing);
            RedrawingFlagsUpdater::get_instance().set_flags(flags);
        }
    }

    if (creature.get_mutations().has(PlayerMutationType::HP_TO_SP) && !creature.has_anti_magic() && one_in_(4000)) {
        int wounds = (int)(creature.get_max_mp() - creature.get_current_mp());
        if (wounds > 0) {
            int healing = creature.hp;
            if (healing > wounds) {
                healing = wounds;
            }

            creature.add_current_mp(healing);
            RedrawingFlagsUpdater::get_instance().set_flags(flags);
            take_hit(creature, DAMAGE_LOSELIFE, healing, _("頭に昇った血", "blood rushing to the head"));
        }
    }

    if (creature.get_mutations().has(PlayerMutationType::DISARM) && one_in_(10000)) {
        disturb(creature, false, true);
        msg_print(_("足がもつれて転んだ！", "You trip over your own feet!"));
        take_hit(creature, DAMAGE_NOESCAPE, randint1(creature.get_wt() / 6), _("転倒", "tripping"));
        drop_weapons(creature);
    }
}

/*!
 * @brief モンスターの突然変異を 10 ゲームターン毎に処理する (提案C5)
 * @param player プレイヤー (メッセージ表示・テレポート処理の視点)
 * @param monster 対象モンスター
 * @details プレイヤー用 process_world_aux_mutation() とは分離した、モンスターに
 *          意味のある能動突然変異のみを扱う専用処理。効果はモンスター安全な
 *          プリミティブ (set_monster_* / teleport_away) で適用し、プレイヤー専用の
 *          副作用 (BadStatusSetter の徳変化・UI プロンプト・脱糞等) は用いない。
 *          メッセージはモンスターが視認可能な時のみ表示する。
 */
void process_monster_mutation(CreatureEntity &player, CreatureEntity &monster)
{
    if (monster.is_player() || monster.get_mutations().none()) {
        return;
    }

    const auto m_idx = monster.get_self_m_idx();
    if (m_idx <= 0) {
        return;
    }

    auto &floor = *monster.get_floor();
    const auto &muta = monster.get_mutations();
    const auto visible = monster.is_visible_on_map();
    const auto level = monster.get_level();
    const auto notify = [&](concptr msg) {
        if (visible) {
            const auto m_name = monster_desc(player, monster, 0);
            msg_format(msg, m_name.data());
        }
    };

    // 激怒 (BERS_RAGE): 恐怖を解除し一時的に加速する (モンスターの狂乱表現)
    if (muta.has(PlayerMutationType::BERS_RAGE) && one_in_(3000)) {
        set_monster_monfear(floor, m_idx, 0);
        set_monster_fast(floor, m_idx, monster.get_timed_effect(CreatureTimedEffect::ACCELERATION) + 10 + randint1(level));
        notify(_("%sは激怒の発作に襲われた！", "%s^ is seized by a fit of rage!"));
    }

    // 臆病 (COWARDICE): 恐怖状態になる
    if (muta.has(PlayerMutationType::COWARDICE) && (randint1(3000) == 13)) {
        if (!monster.has_resist_fear()) {
            set_monster_monfear(floor, m_idx, monster.get_timed_effect(CreatureTimedEffect::FEAR) + 13 + randint1(26));
            notify(_("%sは急に何かに怯えだした！", "%s^ is suddenly afraid!"));
        }
    }

    // ランダムテレポート (RTELEPORT)
    if (muta.has(PlayerMutationType::RTELEPORT) && (randint1(5000) == 88)) {
        if (!monster.has_resist_shard() && muta.has_not(PlayerMutationType::VTELEPORT) && !monster.has_anti_tele()) {
            notify(_("%sの位置は突然ひじょうに不確定になった...", "%s^'s position suddenly seems very uncertain..."));
            teleport_away(player, m_idx, 40, TELEPORT_PASSIVE);
        }
    }

    // 加速度変動 (SPEED_FLUX)
    if (muta.has(PlayerMutationType::SPEED_FLUX) && one_in_(6000)) {
        if (one_in_(2)) {
            set_monster_slow(floor, m_idx, monster.get_timed_effect(CreatureTimedEffect::DECELERATION) + randint1(30) + 10);
        } else {
            set_monster_fast(floor, m_idx, monster.get_timed_effect(CreatureTimedEffect::ACCELERATION) + randint1(30) + 10);
        }
        notify(_("%sの速度が急に変化した。", "%s^'s speed suddenly changes."));
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
