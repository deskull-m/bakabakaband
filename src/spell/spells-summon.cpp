#include "spell/spells-summon.h"
#include "avatar/avatar.h"
#include "effect/spells-effect-util.h"
#include "floor/floor-object.h"
#include "floor/line-of-sight.h"
#include "game-option/birth-options.h"
#include "hpmp/hp-mp-processor.h"
#include "inventory/inventory-object.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/monster-util.h"
#include "monster/smart-learn-types.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-genocide.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-specific-bolt.h"
#include "spell/spells-diceroll.h"
#include "spell/spells-status.h"
#include "spell/summon-types.h"
#include "status/bad-status-setter.h"
#include "sv-definition/sv-other-types.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "target/projection-path-calculator.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

/*!
 * @brief トランプ魔法独自の召喚処理を行う / Handle summoning and failure of trump spells
 * @param creature クリーチャーへの参照
 * @param num summon_specific()関数を呼び出す回数
 * @param pet ペット化として召喚されるか否か
 * @param y 召喚位置のy座標
 * @param x 召喚位置のx座標
 * @param lev 召喚レベル
 * @param type 召喚条件ID
 * @param mode モンスター生成条件フラグ
 * @return モンスターが（敵対も含めて）召還されたならばTRUEを返す。
 */
bool trump_summoning(CreatureEntity &creature, int num, bool pet, POSITION y, POSITION x, DEPTH lev, summon_type type, BIT_FLAGS mode)
{
    /* Default level */
    PLAYER_LEVEL plev = creature.level;
    if (!lev) {
        lev = plev * 2 / 3 + randint1(plev / 2);
    }

    if (pet) {
        /* Become pet */
        mode |= PM_FORCE_PET;

        /* Only sometimes allow unique monster */
        if (mode & PM_ALLOW_UNIQUE) {
            /* Forbid often */
            if (randint1(50 + plev) >= plev / 10) {
                mode &= ~PM_ALLOW_UNIQUE;
            }
        }
    } else {
        /* Prevent taming, allow unique monster */
        mode |= PM_NO_PET;
    }

    bool success = false;
    for (int i = 0; i < num; i++) {
        if (summon_specific(creature, y, x, lev, type, mode)) {
            success = true;
        }
    }

    if (!success) {
        msg_print(_("誰もあなたのカードの呼び声に答えない。", "Nobody answers to your Trump call."));
    }

    return success;
}

bool cast_summon_demon(CreatureEntity &creature, int power)
{
    uint32_t flg = 0L;
    bool pet = !one_in_(3);
    if (pet) {
        flg |= PM_FORCE_PET;
    } else {
        flg |= PM_NO_PET;
    }
    if (!(pet && (creature.level < 50))) {
        flg |= PM_ALLOW_GROUP;
    }

    if (!summon_specific(creature, creature.y, creature.x, power, SUMMON_DEMON, flg)) {
        return true;
    }

    msg_print(_("硫黄の悪臭が充満した。", "The area fills with a stench of sulphur and brimstone."));
    if (pet) {
        msg_print(_("「ご用でございますか、ご主人様」", "'What is thy bidding... Master?'"));
    } else {
        msg_print(_("「卑しき者よ、我は汝の下僕にあらず！ お前の魂を頂くぞ！」", "'NON SERVIAM! Wretch! I shall feast on thy mortal soul!'"));
    }

    return true;
}

bool cast_summon_undead(CreatureEntity &creature, int power)
{
    bool pet = one_in_(3);
    summon_type type = (creature.level > 47 ? SUMMON_HI_UNDEAD : SUMMON_UNDEAD);

    BIT_FLAGS mode = 0L;
    if (!pet || ((creature.level > 24) && one_in_(3))) {
        mode |= PM_ALLOW_GROUP;
    }
    if (pet) {
        mode |= PM_FORCE_PET;
    } else {
        mode |= (PM_ALLOW_UNIQUE | PM_NO_PET);
    }

    if (summon_specific(creature, creature.y, creature.x, power, type, mode)) {
        msg_print(_("冷たい風があなたの周りに吹き始めた。それは腐敗臭を運んでいる...",
            "Cold winds begin to blow around you, carrying with them the stench of decay..."));
        if (pet) {
            msg_print(_("古えの死せる者共があなたに仕えるため土から甦った！", "Ancient, long-dead forms arise from the ground to serve you!"));
        } else {
            msg_print(_("死者が甦った。眠りを妨げるあなたを罰するために！", "'The dead arise... to punish you for disturbing them!'"));
        }
    }
    return true;
}

/*!
 * @brief クッソ汚い奴ら召喚の処理
 * @param creature クリーチャーへの参照
 * @param power 召喚パワー
 * @return 召喚できたらTRUEを返す
 */
bool cast_summon_nasty(CreatureEntity &creature, int power)
{
    bool pet = one_in_(3);
    summon_type type = SUMMON_NASTY;

    BIT_FLAGS mode = 0L;
    if (!pet || ((creature.level > 24) && one_in_(3))) {
        mode |= PM_ALLOW_GROUP;
    }
    if (pet) {
        mode |= PM_FORCE_PET;
    } else {
        mode |= (PM_ALLOW_UNIQUE | PM_NO_PET);
    }

    if (summon_specific(creature, creature.y, creature.x, power, type, mode)) {
        msg_print(_("悪臭が漂い始めた。それは汚物と腐敗の匂いを運んでいる...",
            "A foul stench begins to drift, carrying the smell of filth and corruption..."));
        if (pet) {
            msg_print(_("クッソ汚い奴らがあなたに仕えるため現れた！", "Filthy creatures appear to serve you!"));
        } else {
            msg_print(_("クッソ汚い奴らが現れた。あなたを汚すために！", "Filthy creatures appear to defile you!"));
        }
    }
    return true;
}

bool cast_summon_hound(CreatureEntity &creature, int power)
{
    BIT_FLAGS mode = PM_ALLOW_GROUP;
    bool pet = !one_in_(5);
    if (pet) {
        mode |= PM_FORCE_PET;
    } else {
        mode |= PM_NO_PET;
    }

    if (summon_specific(creature, creature.y, creature.x, power, SUMMON_HOUND, mode)) {
        if (pet) {
            msg_print(_("ハウンドがあなたの下僕として出現した。", "A group of hounds appear as your servants."));
        } else {
            msg_print(_("ハウンドはあなたに牙を向けている！", "A group of hounds appear as your enemies!"));
        }
    }

    return true;
}

bool cast_summon_elemental(CreatureEntity &creature, int power)
{
    bool pet = one_in_(3);
    BIT_FLAGS mode = 0L;
    if (!(pet && (creature.level < 50))) {
        mode |= PM_ALLOW_GROUP;
    }
    if (pet) {
        mode |= PM_FORCE_PET;
    } else {
        mode |= PM_NO_PET;
    }

    if (summon_specific(creature, creature.y, creature.x, power, SUMMON_ELEMENTAL, mode)) {
        msg_print(_("エレメンタルが現れた...", "An elemental materializes..."));
        if (pet) {
            msg_print(_("あなたに服従しているようだ。", "It seems obedient to you."));
        } else {
            msg_print(_("それをコントロールできなかった！", "You fail to control it!"));
        }
    }

    return true;
}

bool cast_summon_octopus(CreatureEntity &creature)
{
    BIT_FLAGS mode = PM_ALLOW_GROUP;
    bool pet = !one_in_(5);
    if (pet) {
        mode |= PM_FORCE_PET;
    }
    if (summon_named_creature(creature, 0, creature.y, creature.x, MonraceId::JIZOTAKO, mode)) {
        if (pet) {
            msg_print(_("蛸があなたの下僕として出現した。", "A group of octopuses appear as your servants."));
        } else {
            msg_print(_("蛸はあなたを睨んでいる！", "A group of octopuses appear as your enemies!"));
        }
    }

    return true;
}

/*!
 * @brief 悪魔領域のグレーターデーモン召喚を処理する / Daemon spell Summon Greater Demon
 * @param creature クリーチャーへの参照
 * @return 処理を実行したならばTRUEを返す。
 */
bool cast_summon_greater_demon(CreatureEntity &creature)
{
    constexpr auto q = _("どの死体を捧げますか? ", "Sacrifice which corpse? ");
    constexpr auto s = _("捧げられる死体を持っていない。", "You have nothing to sacrifice.");
    const auto &[item, i_idx] = choose_item(creature, q, s, (USE_INVEN | USE_FLOOR), FuncItemTester(&ItemEntity::is_offerable));
    if (!item) {
        return false;
    }

    const auto summon_lev = creature.level * 2 / 3 + item->get_monrace().level;
    if (summon_specific(creature, creature.y, creature.x, summon_lev, SUMMON_HI_DEMON, (PM_ALLOW_GROUP | PM_FORCE_PET))) {
        msg_print(_("硫黄の悪臭が充満した。", "The area fills with a stench of sulphur and brimstone."));
        msg_print(_("「ご用でございますか、ご主人様」", "'What is thy bidding... Master?'"));
        vary_item(creature, i_idx, -1);
    } else {
        msg_print(_("悪魔は現れなかった。", "No Greater Demon arrives."));
    }

    return true;
}

/*!
 * @brief 同族召喚(援軍)処理
 * @param creature クリーチャーへの参照
 * @param level 召喚基準レベル
 * @param y 召喚先Y座標
 * @param x 召喚先X座標
 * @param mode 召喚オプション
 * @return ターンを消費した場合TRUEを返す
 */
bool summon_kin_player(CreatureEntity &creature, DEPTH level, POSITION y, POSITION x, BIT_FLAGS mode)
{
    bool pet = (bool)(mode & PM_FORCE_PET);
    if (!pet) {
        mode |= PM_NO_PET;
    }
    return summon_specific(creature, y, x, level, SUMMON_KIN, mode).has_value();
}

/*!
 * @brief サイバーデーモンの召喚
 * @param creature クリーチャーへの参照
 * @param y 召喚位置Y座標
 * @param x 召喚位置X座標
 * @param summoner_m_idx モンスターの召喚による場合、召喚者のモンスターID
 * @return 作用が実際にあった場合TRUEを返す
 */
int summon_cyber(CreatureEntity &creature, POSITION y, POSITION x, tl::optional<MONSTER_IDX> summoner_m_idx)
{
    /* Summoned by a monster */
    BIT_FLAGS mode = PM_ALLOW_GROUP;
    const auto &floor = *creature.get_floor();
    if (summoner_m_idx) {
        const auto &monster = floor.get_monster(*summoner_m_idx);
        if (monster.is_pet()) {
            mode |= PM_FORCE_PET;
        }
    }

    int max_cyber = (floor.dun_level / 50) + randint1(2);
    if (max_cyber > 4) {
        max_cyber = 4;
    }

    int count = 0;
    for (int i = 0; i < max_cyber; i++) {
        count += summon_specific(creature, y, x, 100, SUMMON_CYBER, mode, summoner_m_idx) ? 1 : 0;
    }

    return count;
}

void mitokohmon(CreatureEntity &creature)
{
    int count = 0;
    concptr sukekakusan = "";
    if (summon_named_creature(creature, 0, creature.y, creature.x, MonraceId::SUKE, PM_FORCE_PET)) {
        msg_print(_("『助さん』が現れた。", "Suke-san apperars."));
        sukekakusan = "Suke-san";
        count++;
    }

    if (summon_named_creature(creature, 0, creature.y, creature.x, MonraceId::KAKU, PM_FORCE_PET)) {
        msg_print(_("『格さん』が現れた。", "Kaku-san appears."));
        sukekakusan = "Kaku-san";
        count++;
    }

    if (!count) {
        const auto &floor = *creature.get_floor();
        const auto p_pos = creature.get_position();
        for (auto i = floor.m_max - 1; i > 0; i--) {
            const auto &monster = floor.get_monster(static_cast<MONSTER_IDX>(i));
            if (!monster.is_valid()) {
                continue;
            }
            if (!((monster.r_idx == MonraceId::SUKE) || (monster.r_idx == MonraceId::KAKU))) {
                continue;
            }

            const auto m_pos = monster.get_position();
            if (!los(floor, m_pos, p_pos)) {
                continue;
            }
            if (!projectable(floor, m_pos, p_pos)) {
                continue;
            }
            count++;
            break;
        }
    }

    if (count == 0) {
        msg_print(_("しかし、何も起きなかった。", "Nothing happens."));
        return;
    }

    msg_format(
        _("「者ども、ひかえおろう！！！このお方をどなたとこころえる。」", "%s^ says 'WHO do you think this person is! Bow your head, down to your knees!'"),
        sukekakusan);
    sukekaku = true;
    stun_monsters(creature, 120);
    confuse_monsters(creature, 120);
    turn_monsters(creature, 120);
    stasis_monsters(creature, 120);
    sukekaku = false;
}

/*!
 * @brief HI_SUMMON(上級召喚)処理発動
 * @param creature クリーチャーへの参照
 * @param y 召喚位置Y座標
 * @param x 召喚位置X座標
 * @param can_pet プレイヤーのペットとなる可能性があるならばTRUEにする
 * @return 作用が実際にあった場合TRUEを返す
 * @todo 引数にPOSITION x/yは必要か？ 要調査
 */
int activate_hi_summon(CreatureEntity &creature, POSITION y, POSITION x, bool can_pet)
{
    BIT_FLAGS mode = PM_ALLOW_GROUP;
    bool pet = false;
    if (can_pet) {
        if (one_in_(4)) {
            mode |= PM_FORCE_FRIENDLY;
        } else {
            mode |= PM_FORCE_PET;
            pet = true;
        }
    }

    if (!pet) {
        mode |= PM_NO_PET;
    }

    DEPTH dungeon_level = creature.get_floor()->dun_level;
    DEPTH summon_lev = (pet ? creature.level * 2 / 3 + randint1(creature.level / 2) : dungeon_level);
    int count = 0;
    for (int i = 0; i < (randint1(7) + (dungeon_level / 40)); i++) {
        switch (randint1(25) + (dungeon_level / 20)) {
        case 1:
        case 2:
            count += summon_specific(creature, y, x, summon_lev, SUMMON_ANT, mode) ? 1 : 0;
            break;
        case 3:
        case 4:
            count += summon_specific(creature, y, x, summon_lev, SUMMON_SPIDER, mode) ? 1 : 0;
            break;
        case 5:
        case 6:
            count += summon_specific(creature, y, x, summon_lev, SUMMON_HOUND, mode) ? 1 : 0;
            break;
        case 7:
        case 8:
            count += summon_specific(creature, y, x, summon_lev, SUMMON_HYDRA, mode) ? 1 : 0;
            break;
        case 9:
        case 10:
            count += summon_specific(creature, y, x, summon_lev, SUMMON_ANGEL, mode) ? 1 : 0;
            break;
        case 11:
        case 12:
            count += summon_specific(creature, y, x, summon_lev, SUMMON_UNDEAD, mode) ? 1 : 0;
            break;
        case 13:
        case 14:
            count += summon_specific(creature, y, x, summon_lev, SUMMON_DRAGON, mode) ? 1 : 0;
            break;
        case 15:
        case 16:
            count += summon_specific(creature, y, x, summon_lev, SUMMON_DEMON, mode) ? 1 : 0;
            break;
        case 17:
            if (can_pet) {
                break;
            }
            count += summon_specific(creature, y, x, summon_lev, SUMMON_AMBERITES, (mode | PM_ALLOW_UNIQUE)) ? 1 : 0;
            break;
        case 18:
        case 19:
            if (can_pet) {
                break;
            }
            count += summon_specific(creature, y, x, summon_lev, SUMMON_UNIQUE, (mode | PM_ALLOW_UNIQUE)) ? 1 : 0;
            break;
        case 20:
        case 21:
            if (!can_pet) {
                mode |= PM_ALLOW_UNIQUE;
            }
            count += summon_specific(creature, y, x, summon_lev, SUMMON_HI_UNDEAD, mode) ? 1 : 0;
            break;
        case 22:
        case 23:
            if (!can_pet) {
                mode |= PM_ALLOW_UNIQUE;
            }
            count += summon_specific(creature, y, x, summon_lev, SUMMON_HI_DRAGON, mode) ? 1 : 0;
            break;
        case 24:
            count += summon_specific(creature, y, x, 100, SUMMON_CYBER, mode) ? 1 : 0;
            break;
        default:
            if (!can_pet) {
                mode |= PM_ALLOW_UNIQUE;
            }
            count += summon_specific(creature, y, x, pet ? summon_lev : (((summon_lev * 3) / 2) + 5), SUMMON_NONE, mode) ? 1 : 0;
        }
    }

    return count;
}

/*!
 * @brief 「悪霊召喚」のランダムな効果を決定して処理する。
 * @param creature クリーチャーへの参照
 * @param dir 方向ID
 */
void cast_invoke_spirits(CreatureEntity &creature, const Direction &dir)
{
    PLAYER_LEVEL plev = creature.level;
    int die = randint1(100) + plev / 5;
    int vir = virtue_number(creature, Virtue::CHANCE);

    if (vir != 0) {
        auto it = creature.virtues.find(Virtue::CHANCE);
        if (it != creature.virtues.end()) {
            if (it->second > 0) {
                while (randint1(400) < it->second) {
                    die++;
                }
            } else {
                while (randint1(400) < (0 - it->second)) {
                    die--;
                }
            }
        }
    }

    msg_print(_("あなたは死者たちの力を招集した...", "You call on the power of the dead..."));
    if (die < 26) {
        chg_virtue(creature, Virtue::CHANCE, 1);
    }

    if (die > 100) {
        msg_print(_("あなたはおどろおどろしい力のうねりを感じた！", "You feel a surge of eldritch force!"));
    }

    BadStatusSetter bss(creature);
    if (die < 8) {
        msg_print(_("なんてこった！あなたの周りの地面から朽ちた人影が立ち上がってきた！", "Oh no! Mouldering forms rise from the earth around you!"));

        (void)summon_specific(creature, creature.y, creature.x, creature.get_floor()->dun_level, SUMMON_UNDEAD,
            (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET));
        chg_virtue(creature, Virtue::UNLIFE, 1);
    } else if (die < 14) {
        msg_print(_("名状し難い邪悪な存在があなたの心を通り過ぎて行った...", "An unnamable evil brushes against your mind..."));
        (void)bss.mod_fear(randint1(4) + 4);
    } else if (die < 26) {
        msg_print(_("あなたの頭に大量の幽霊たちの騒々しい声が押し寄せてきた...", "Your head is invaded by a horde of gibbering spectral voices..."));
        (void)bss.mod_confusion(randint1(4) + 4);
    } else if (die < 31) {
        poly_monster(creature, dir, plev);
    } else if (die < 36) {
        fire_bolt_or_beam(creature, beam_chance(creature) - 10, AttributeType::MISSILE, dir, Dice::roll(3 + ((plev - 1) / 5), 4));
    } else if (die < 41) {
        confuse_monster(creature, dir, plev);
    } else if (die < 46) {
        fire_ball(creature, AttributeType::POIS, dir, 20 + (plev / 2), 3);
    } else if (die < 51) {
        (void)lite_line(creature, dir, Dice::roll(6, 8));
    } else if (die < 56) {
        fire_bolt_or_beam(creature, beam_chance(creature) - 10, AttributeType::ELEC, dir, Dice::roll(3 + ((plev - 5) / 4), 8));
    } else if (die < 61) {
        fire_bolt_or_beam(creature, beam_chance(creature) - 10, AttributeType::COLD, dir, Dice::roll(5 + ((plev - 5) / 4), 8));
    } else if (die < 66) {
        fire_bolt_or_beam(creature, beam_chance(creature), AttributeType::ACID, dir, Dice::roll(6 + ((plev - 5) / 4), 8));
    } else if (die < 71) {
        fire_bolt_or_beam(creature, beam_chance(creature), AttributeType::FIRE, dir, Dice::roll(8 + ((plev - 5) / 4), 8));
    } else if (die < 76) {
        hypodynamic_bolt(creature, dir, 75);
    } else if (die < 81) {
        fire_ball(creature, AttributeType::ELEC, dir, 30 + plev / 2, 2);
    } else if (die < 86) {
        fire_ball(creature, AttributeType::ACID, dir, 40 + plev, 2);
    } else if (die < 91) {
        fire_ball(creature, AttributeType::ICE, dir, 70 + plev, 3);
    } else if (die < 96) {
        fire_ball(creature, AttributeType::FIRE, dir, 80 + plev, 3);
    } else if (die < 101) {
        hypodynamic_bolt(creature, dir, 100 + plev);
    } else if (die < 104) {
        earthquake(creature, creature.get_position(), 12);
    } else if (die < 106) {
        (void)destroy_area(creature, creature.y, creature.x, 13 + randint0(5), false);
    } else if (die < 108) {
        symbol_genocide(creature, plev + 50, true);
    } else if (die < 110) {
        dispel_monsters(creature, 120);
    } else {
        dispel_monsters(creature, 150);
        slow_monsters(creature, plev);
        sleep_monsters(creature, plev);
        hp_player(creature, 300);
    }

    if (die < 31) {
        msg_print(
            _("陰欝な声がクスクス笑う。「もうすぐおまえは我々の仲間になるだろう。弱き者よ。」", "Sepulchral voices chuckle. 'Soon you will join us, mortal.'"));
    }
}
