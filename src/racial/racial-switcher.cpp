/*!
 * @brief レイシャルと突然変異の技能処理 / Racial powers (and mutations)
 * @date 2014/01/08
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2014 Deskull rearranged comment for Doxygen. \n
 */

#include "racial/racial-switcher.h"
#include "action/action-limited.h"
#include "action/mutation-execution.h"
#include "cmd-action/cmd-mane.h"
#include "cmd-action/cmd-spell.h"
#include "cmd-item/cmd-magiceat.h"
#include "cmd-item/cmd-zapwand.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "effect/spells-effect-util.h"
#include "game-option/text-display-options.h"
#include "inventory/inventory-slot-types.h"
#include "io/command-repeater.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "main/sound-of-music.h"
#include "melee/melee-postprocess.h"
#include "mind/mind-archer.h"
#include "mind/mind-cavalry.h"
#include "mind/mind-elementalist.h"
#include "mind/mind-force-trainer.h"
#include "mind/mind-hobbit.h"
#include "mind/mind-mage.h"
#include "mind/mind-magic-eater.h"
#include "mind/mind-mirror-master.h"
#include "mind/mind-monk.h"
#include "mind/mind-ninja.h"
#include "mind/mind-priest.h"
#include "mind/mind-samurai.h"
#include "mind/mind-warrior-mage.h"
#include "mind/mind-warrior.h"
#include "mind/monk-attack.h"
#include "mind/stances-table.h"
#include "mutation/mutation-flag-types.h"
#include "object/item-tester-hooker.h"
#include "player-info/class-info.h"
#include "player-info/equipment-info.h"
#include "player-info/race-info.h"
#include "player-info/race-types.h"
#include "player-status/player-energy.h"
#include "player-status/player-hand-types.h"
#include "player/attack-defense-types.h"
#include "player/player-damage.h"
#include "player/player-personality-types.h"
#include "player/player-realm.h"
#include "player/player-status.h"
#include "racial/racial-android.h"
#include "racial/racial-balrog.h"
#include "racial/racial-draconian.h"
#include "racial/racial-kutar.h"
#include "racial/racial-vampire.h"
#include "spell-class/spells-mirror-master.h"
#include "spell-kind/spells-beam.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-grid.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-song.h"
#include "spell/spells-status.h"
#include "status/action-setter.h"
#include "status/bad-status-setter.h"
#include "status/buff-setter.h"
#include "status/experience.h"
#include "system/creature-entity.h"
#include "system/redrawing-flags-updater.h"
#include "target/target-getter.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"

bool switch_class_racial_execution(CreatureEntity &creature, const int32_t command)
{
    switch (creature.pclass) {
    case PlayerClassType::WARRIOR:
        return sword_dancing(creature);
    case PlayerClassType::HIGH_MAGE:
        if (PlayerRealm(creature).is_realm_hex()) {
            const auto retval = SpellHex(creature).stop_spells_with_selection();
            if (retval) {
                PlayerEnergy(creature).set_player_turn_energy(10);
            }

            return retval;
        }

        [[fallthrough]];
    case PlayerClassType::MAGE:
    case PlayerClassType::SORCERER:
        return eat_magic(creature, creature.level * 2);
    case PlayerClassType::PRIEST:
        if (!PlayerRealm(creature).realm1().is_good_attribute()) {
            (void)dispel_monsters(creature, creature.level * 4);
            turn_monsters(creature, creature.level * 4);
            banish_monsters(creature, creature.level * 4);
            return true;
        }

        return bless_weapon(creature);
    case PlayerClassType::ROGUE:
        return hit_and_away(creature);
    case PlayerClassType::RANGER:
    case PlayerClassType::SNIPER:
        msg_print(_("敵を調査した...", "You examine your foes..."));
        probing(creature);
        return true;
    case PlayerClassType::PALADIN: {
        const auto dir = get_aim_dir(creature);
        if (!dir) {
            return false;
        }

        fire_beam(creature, PlayerRealm(creature).realm1().is_good_attribute() ? AttributeType::HOLY_FIRE : AttributeType::HELL_FIRE, dir, creature.level * 3);
        return true;
    }
    case PlayerClassType::WARRIOR_MAGE:
        if (command == -3) {
            return comvert_hp_to_mp(creature);
        } else if (command == -4) {
            return comvert_mp_to_hp(creature);
        }

        return true;
    case PlayerClassType::CHAOS_WARRIOR:
        return confusing_light(creature);
    case PlayerClassType::MONK:
        if (none_bits(empty_hands(creature, true), EMPTY_HAND_MAIN)) {
            msg_print(_("素手じゃないとできません。", "You need to be barehanded."));
            return false;
        }

        if (creature.riding) {
            msg_print(_("乗馬中はできません。", "You need to get off a pet."));
            return false;
        }

        if (command == -3) {
            if (!choose_monk_stance(creature)) {
                return false;
            }

            RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
            return true;
        }

        if (command == -4) {
            return double_attack(creature);
        }

        return true;
    case PlayerClassType::MINDCRAFTER:
    case PlayerClassType::FORCETRAINER:
        return clear_mind(creature);
    case PlayerClassType::TOURIST:
        if (command == -3) {
            const auto dir = get_aim_dir(creature);
            if (!dir) {
                return false;
            }

            project_length = 1;
            fire_beam(creature, AttributeType::PHOTO, dir, 1);
            return true;
        }

        return (command != -4) || identify_fully(creature, false);
    case PlayerClassType::IMITATOR:
        handle_stuff(creature);
        return do_cmd_mane(creature, true);
    case PlayerClassType::BEASTMASTER:
        if (command == -3) {
            const auto dir = get_aim_dir(creature);
            if (!dir) {
                return false;
            }

            (void)fire_ball_hide(creature, AttributeType::CHARM_LIVING, dir, creature.level, 0);
            return true;
        }

        if (command == -4) {
            project_all_los(creature, AttributeType::CHARM_LIVING, creature.level);
        }

        return true;
    case PlayerClassType::ARCHER:
        return create_ammo(creature);
    case PlayerClassType::MAGIC_EATER:
        if (command == -3) {
            return import_magic_device(creature);
        }

        return (command != -4) || (!cmd_limit_cast(creature) && do_cmd_magic_eater(creature, false, true));
    case PlayerClassType::BARD:
        if ((get_singing_song_effect(creature) == 0) && (get_interrupting_song_effect(creature) == 0)) {
            return false;
        }

        stop_singing(creature);
        PlayerEnergy(creature).set_player_turn_energy(10);
        return true;
    case PlayerClassType::RED_MAGE:
        if (cmd_limit_cast(creature)) {
            return false;
        }

        handle_stuff(creature);
        if (!do_cmd_cast(creature)) {
            return false;
        }

        if (!creature.is_paralyzed() && !cmd_limit_cast(creature)) {
            handle_stuff(creature);
            command_dir = Direction::none();
            (void)do_cmd_cast(creature);
        }
        return true;
    case PlayerClassType::SAMURAI:
        if (command == -3) {
            concentration(creature);
            return true;
        }

        if (command != -4) {
            return true;
        }

        if (!has_melee_weapon(creature, INVEN_MAIN_HAND) && !has_melee_weapon(creature, INVEN_SUB_HAND)) {
            msg_print(_("\u6b66\u5668\u3092\u6301\u305f\u306a\u3044\u3068\u3044\u3051\u307e\u305b\u3093\u3002", "You need to wield a weapon."));
            return false;
        }

        if (!choose_samurai_stance(creature)) {
            return false;
        }

        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
        return true;
    case PlayerClassType::BLUE_MAGE:
        set_action(creature, creature.action == ACTION_LEARN ? ACTION_NONE : ACTION_LEARN);
        PlayerEnergy(creature).reset_player_turn();
        return true;
    case PlayerClassType::CAVALRY:
        return rodeo(creature);
    case PlayerClassType::BERSERKER:
        return recall_player(creature, randint0(21) + 15);
    case PlayerClassType::SMITH:
        if (creature.level <= 29) {
            return ident_spell(creature, true);
        }

        return identify_fully(creature, true);
    case PlayerClassType::MIRROR_MASTER: {
        SpellsMirrorMaster smm(creature);
        if (command == -3) {
            smm.remove_all_mirrors(true);
            return true;
        }

        if (command == -4) {
            return smm.mirror_concentration();
        }

        return true;
    }
    case PlayerClassType::NINJA:
        return hayagake(creature);
    case PlayerClassType::ELEMENTALIST:
        if (command == -3) {
            return clear_mind(creature);
        }
        if (command == -4) {
            return switch_element_execution(creature);
        }
        return true;
    default:
        return true;
    }
}

bool switch_mimic_racial_execution(CreatureEntity &creature)
{
    switch (creature.get_mimic_form()) {
    case MimicKindType::DEMON:
    case MimicKindType::DEMON_LORD: {
        return demonic_breath(creature);
    }
    case MimicKindType::VAMPIRE:
        vampirism(creature);
        return true;
    default:
        return true;
    }
}

bool switch_race_racial_execution(CreatureEntity &creature, const int32_t command)
{
    // 性格ベースのレイシャル能力をチェック
    if (creature.ppersonality == PERSONALITY_MESUGAKI) {
        const auto dir = get_aim_dir(creature);
        if (!dir) {
            return false;
        }
        msg_print(_("社会的抹殺ボルトを放った！", "You fire a social genocide bolt!"));
        fire_bolt(creature, AttributeType::SOCIAL_GENOCIDE, dir, creature.level * 2);
        return true;
    }

    switch (creature.prace) {
    case PlayerRaceType::DWARF:
        msg_print(_("周囲を調べた。", "You examine your surroundings."));
        (void)detect_traps(creature, DETECT_RAD_DEFAULT, true);
        (void)detect_doors(creature, DETECT_RAD_DEFAULT);
        (void)detect_stairs(creature, DETECT_RAD_DEFAULT);
        return true;
    case PlayerRaceType::HOBBIT:
        return create_ration(creature);
    case PlayerRaceType::GNOME:
        msg_print(_("パッ！", "Blink!"));
        teleport_player(creature, 10, TELEPORT_SPONTANEOUS);
        return true;
    case PlayerRaceType::HALF_ORC:
        msg_print(_("勇気を出した。", "You play tough."));
        (void)BadStatusSetter(creature).set_fear(0);
        return true;
    case PlayerRaceType::HALF_TROLL:
        msg_print(_("うがぁぁ！", "RAAAGH!"));
        (void)berserk(creature, 10 + randint1(creature.level));
        return true;
    case PlayerRaceType::AMBERITE:
        if (command == -1) {
            msg_print(_("あなたは歩き周り始めた。", "You start walking around. "));
            reserve_alter_reality(creature, randint0(21) + 15);
            return true;
        }

        if (command != -2) {
            return true;
        }

        msg_print(_("あなたは「パターン」を心に描いてその上を歩いた...", "You picture the Pattern in your mind and walk it..."));
        (void)true_healing(creature, 0);
        (void)restore_all_status(creature);
        (void)restore_level(creature);
        return true;
    case PlayerRaceType::BARBARIAN:
        msg_print(_("うぉぉおお！", "Raaagh!"));
        (void)berserk(creature, 10 + randint1(creature.level));
        return true;
    case PlayerRaceType::HALF_OGRE:
        msg_print(_("爆発のルーンを慎重に仕掛けた...", "You carefully set an explosive rune..."));
        (void)create_rune_explosion(creature, creature.y, creature.x);
        return true;
    case PlayerRaceType::HALF_GIANT: {
        const auto dir = get_aim_dir(creature);
        if (!dir) {
            return false;
        }

        (void)wall_to_mud(creature, dir, 20 + randint1(30));
        return true;
    }
    case PlayerRaceType::HALF_TITAN:
        msg_print(_("敵を調査した...", "You examine your foes..."));
        (void)probing(creature);
        return true;
    case PlayerRaceType::CYCLOPS: {
        const auto dir = get_aim_dir(creature);
        if (!dir) {
            return false;
        }

        msg_print(_("巨大な岩を投げた。", "You throw a huge boulder."));
        (void)fire_bolt(creature, AttributeType::MISSILE, dir, (3 * creature.level) / 2);
        return true;
    }
    case PlayerRaceType::YEEK: {
        const auto dir = get_aim_dir(creature);
        if (!dir) {
            return false;
        }

        stop_mouth(creature);
        msg_print(_("身の毛もよだつ叫び声を上げた！", "You make a horrible scream!"));
        (void)fear_monster(creature, dir, creature.level);
        return true;
    }
    case PlayerRaceType::KLACKON: {
        const auto dir = get_aim_dir(creature);
        if (!dir) {
            return false;
        }

        stop_mouth(creature);
        msg_print(_("酸を吐いた。", "You spit acid."));
        if (creature.level < 25) {
            (void)fire_bolt(creature, AttributeType::ACID, dir, creature.level);
        } else {
            (void)fire_ball(creature, AttributeType::ACID, dir, creature.level, 2);
        }

        return true;
    }
    case PlayerRaceType::KOBOLD: {
        const auto dir = get_aim_dir(creature);
        if (!dir) {
            return false;
        }

        msg_print(_("毒のダーツを投げた。", "You throw a poisoned dart."));
        (void)fire_bolt(creature, AttributeType::POIS, dir, creature.level);
        return true;
    }
    case PlayerRaceType::NIBELUNG:
        msg_print(_("周囲を調査した。", "You examine your surroundings."));
        (void)detect_traps(creature, DETECT_RAD_DEFAULT, true);
        (void)detect_doors(creature, DETECT_RAD_DEFAULT);
        (void)detect_stairs(creature, DETECT_RAD_DEFAULT);
        return true;
    case PlayerRaceType::DARK_ELF: {
        const auto dir = get_aim_dir(creature);
        if (!dir) {
            return false;
        }

        msg_print(_("マジック・ミサイルを放った。", "You cast a magic missile."));
        (void)fire_bolt_or_beam(creature, 10, AttributeType::MISSILE, dir, Dice::roll(3 + ((creature.level - 1) / 5), 4));
        return true;
    }
    case PlayerRaceType::DRACONIAN:
        return draconian_breath(creature);
    case PlayerRaceType::MIND_FLAYER: {
        const auto dir = get_aim_dir(creature);
        if (!dir) {
            return false;
        }

        msg_print(_("あなたは集中し、目が赤く輝いた...", "You concentrate and your eyes glow red..."));
        (void)fire_bolt(creature, AttributeType::PSI, dir, creature.level);
        return true;
    }
    case PlayerRaceType::IMP: {
        const auto dir = get_aim_dir(creature);
        if (!dir) {
            return false;
        }

        if (creature.level >= 30) {
            msg_print(_("ファイア・ボールを放った。", "You cast a ball of fire."));
            (void)fire_ball(creature, AttributeType::FIRE, dir, creature.level, 2);
        } else {
            msg_print(_("ファイア・ボルトを放った。", "You cast a bolt of fire."));
            (void)fire_bolt(creature, AttributeType::FIRE, dir, creature.level);
        }

        return true;
    }
    case PlayerRaceType::GOLEM:
        (void)set_shield(creature, randint1(20) + 30, false);
        return true;
    case PlayerRaceType::SKELETON:
    case PlayerRaceType::ZOMBIE:
        msg_print(_("あなたは失ったエネルギーを取り戻そうと試みた。", "You attempt to restore your lost energies."));
        (void)restore_level(creature);
        return true;
    case PlayerRaceType::VAMPIRE:
        (void)vampirism(creature);
        return true;
    case PlayerRaceType::SPECTRE: {
        const auto dir = get_aim_dir(creature);
        if (!dir) {
            return false;
        }

        stop_mouth(creature);
        msg_print(_("あなたはおどろおどろしい叫び声をあげた！", "You emit an eldritch howl!"));
        (void)fear_monster(creature, dir, creature.level);
        return true;
    }
    case PlayerRaceType::SPRITE:
        msg_print(_("あなたは魔法の粉を投げつけた...", "You throw some magic dust..."));
        if (creature.level < 25) {
            (void)sleep_monsters_touch(creature);
        } else {
            (void)sleep_monsters(creature, creature.level);
        }

        return true;
    case PlayerRaceType::BALROG:
        return demonic_breath(creature);
    case PlayerRaceType::KUTAR:
        (void)set_leveling(creature, randint1(20) + 30, false);
        return true;
    case PlayerRaceType::ANDROID:
        return android_inside_weapon(creature);
    case PlayerRaceType::MERFOLK: {
        msg_print(_("あなたは水流を呼び寄せた！", "You have summoned a stream of water!"));
        fire_ball_hide(creature, AttributeType::WATER_FLOW, Direction::self(), 3, 5);
        return true;
    }
    default:
        msg_print(_("この種族は特殊な能力を持っていません。", "This race has no bonus power."));
        PlayerEnergy(creature).reset_player_turn();
        return true;
    }
}
