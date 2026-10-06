/*!
 * @brief ものまねの処理実装 / Imitation code
 * @date 2014/01/14
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2014 Deskull rearranged comment for Doxygen.\n
 */

#include "cmd-action/cmd-mane.h"
#include "action/action-limited.h"
#include "artifact/fixed-art-types.h"
#include "cmd-action/cmd-spell.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "floor/floor-object.h"
#include "game-option/disturbance-options.h"
#include "game-option/text-display-options.h"
#include "hpmp/hp-mp-processor.h"
#include "inventory/inventory-slot-types.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-mage.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-flags-resistance.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-processor.h"
#include "monster/monster-status.h"
#include "mspell/monster-power-table.h"
#include "mspell/mspell-projection-table.h"
#include "player-base/player-class.h"
#include "player-info/mane-data-type.h"
#include "player-status/player-energy.h"
#include "player/player-status-table.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell/spells-status.h"
#include "spell/spells-summon.h"
#include "spell/summon-types.h"
#include "status/bad-status-setter.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/projection-path-calculator.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "target/target-setter.h"
#include "target/target-types.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "util/enum-converter.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include <iterator>
#include <unordered_set>

namespace {
//! 方向を選ぶ魔法 (ブレス・ボール・ボルトは find_mspell_projection() で判定する)
const std::unordered_set<MonsterAbilityType> AIMING_SPELLS = {
    MonsterAbilityType::ROCKET,
    MonsterAbilityType::SHOOT,
    MonsterAbilityType::DRAIN_MANA,
    MonsterAbilityType::MIND_BLAST,
    MonsterAbilityType::BRAIN_SMASH,
    MonsterAbilityType::CAUSE_1,
    MonsterAbilityType::CAUSE_2,
    MonsterAbilityType::CAUSE_3,
    MonsterAbilityType::CAUSE_4,
    MonsterAbilityType::SCARE,
    MonsterAbilityType::BLIND,
    MonsterAbilityType::CONF,
    MonsterAbilityType::SLOW,
    MonsterAbilityType::HOLD,
    MonsterAbilityType::HAND_DOOM,
    MonsterAbilityType::TELE_AWAY,
    MonsterAbilityType::PSY_SPEAR,
};
}

static int damage;

/*!
 * @brief 受け取ったパラメータに応じてものまねの効果情報をまとめたフォーマットを返す
 * @param power ものまねの効力の種類
 * @param dam ものまねの威力
 * @param std::string ものまねの効果を表す文字列
 */
static std::string mane_info(CreatureEntity &creature, MonsterAbilityType power, int dam)
{
    PLAYER_LEVEL plev = static_cast<PLAYER_LEVEL>(creature.get_level());

    const auto power_int = enum2i(power);
    using Mat = MonsterAbilityType;
    EnumClassFlagGroup<Mat> flags{
        Mat::PSY_SPEAR, Mat::BO_VOID, Mat::BO_ABYSS, Mat::BA_VOID, Mat::BA_ABYSS, Mat::BR_VOID, Mat::BR_ABYSS, Mat::BR_FECES
    };
    if ((power_int > 2 && power_int < 41) || (power_int > 41 && power_int < 59) || flags.has(power)) {
        return format(" %s%d", KWD_DAM, (int)dam);
    }
    switch (power) {
    case MonsterAbilityType::DRAIN_MANA:
        return format(" %sd%d+%d", KWD_HEAL, plev * 3, plev);
    case MonsterAbilityType::HASTE:
        return format(" %sd%d+%d", KWD_DURATION, 20 + plev, plev);
    case MonsterAbilityType::HEAL:
        return format(" %s%d", KWD_HEAL, plev * 6);
    case MonsterAbilityType::INVULNER:
        return format(" %sd7+7", KWD_DURATION);
    case MonsterAbilityType::BLINK:
        return format(" %s10", KWD_SPHERE);
    case MonsterAbilityType::TPORT:
        return format(" %s%d", KWD_SPHERE, plev * 5);
    case MonsterAbilityType::RAISE_DEAD:
        return format(" %s5", KWD_SPHERE);
    default:
        return std::string();
    }
}

/*!
 * @brief どのものまねを発動するか選択する処理 /
 * Allow user to choose a imitation.
 * @param sn 実行したものまねのIDを返す参照ポインタ（キャンセルなどの場合-1を返す）
 * @param baigaesi TRUEならば倍返し上の処理として行う
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 * @details
 * If a valid spell is chosen, saves it in '*sn' and returns TRUE
 * If the user hits escape, returns FALSE, and set '*sn' to -1
 * If there are no legal choices, returns FALSE, and sets '*sn' to -2
 *
 * The "prompt" should be "cast", "recite", or "study"
 * The "known" should be TRUE for cast/pray, false for study
 *
 * nb: This function has a (trivial) display bug which will be obvious
 * when you run it. It's probably easy to fix but I haven't tried,
 * sorry.
 */
static int get_mane_power(CreatureEntity &creature, int *sn, bool baigaesi)
{
    int i = 0;
    int num = 0;
    TERM_LEN y = 1;
    TERM_LEN x = 18;
    PERCENTAGE minfail = 0;
    PLAYER_LEVEL plev = static_cast<PLAYER_LEVEL>(creature.get_level());
    PERCENTAGE chance = 0;
    char choice;
    concptr p = _("能力", "power");

    monster_power spell;
    bool flag, redraw;

    /* Assume cancelled */
    *sn = (-1);

    flag = false;
    redraw = false;

    auto mane_data = CreatureClass(creature).get_specific_data<mane_data_type>();

    num = mane_data->mane_list.size();

    /* Build a prompt (accept all spells) */
    constexpr auto fmt = _("(%c-%c, '*'で一覧, ESC) どの%sをまねますか？", "(%c-%c, *=List, ESC=exit) Use which %s? ");
    const auto prompt = format(fmt, I2A(0), I2A(num - 1), p);

    choice = always_show_list ? ESCAPE : 1;
    while (!flag) {
        if (choice == ESCAPE) {
            choice = ' ';
        } else {
            const auto new_choice = input_command(prompt);
            if (!new_choice) {
                break;
            }

            choice = new_choice.value();
        }

        /* Request redraw */
        if ((choice == ' ') || (choice == '*') || (choice == '?')) {
            /* Show the list */
            if (!redraw) {
                redraw = true;
                screen_save();

                /* Display a list of spells */
                prt("", y, x);
                put_str(_("名前", "Name"), y, x + 5);
                put_str(_("失率 効果", "Fail Info"), y, x + 36);

                /* Dump the spells */
                for (i = 0; i < num; i++) {
                    const auto &mane = mane_data->mane_list[i];
                    /* Access the spell */
                    spell = monster_powers.at(mane.spell);

                    chance = spell.manefail;

                    /* Reduce failure rate by "effective" level adjustment */
                    if (plev > spell.level) {
                        chance -= 3 * (plev - spell.level);
                    }

                    /* Reduce failure rate by INT/WIS adjustment */
                    chance -= 3 * (adj_mag_stat[creature.get_stat_index(spell.use_stat)] + adj_mag_stat[creature.get_stat_index(A_DEX)] - 2) / 2;

                    if (spell.manedam) {
                        chance = chance * (baigaesi ? mane.damage * 2 : mane.damage) / spell.manedam;
                    }

                    chance += creature.get_to_m_chance();

                    if (creature.is_wielding(FixedArtifactId::GOGO_PENDANT)) {
                        chance -= 10;
                    }

                    /* Extract the minimum failure rate */
                    minfail = adj_mag_fail[creature.get_stat_index(spell.use_stat)];

                    /* Minimum failure rate */
                    if (chance < minfail) {
                        chance = minfail;
                    }

                    chance += creature.get_stun_magic_chance_penalty();
                    if (chance > 95) {
                        chance = 95;
                    }

                    /* Get info */
                    const auto comment = mane_info(creature, mane.spell, (baigaesi ? mane.damage * 2 : mane.damage));

                    /* Dump the spell --(-- */
                    prt(format("  %c) %-30s %3d%%%s", I2A(i), spell.name, chance, comment.data()), y + i + 1, x);
                }

                /* Clear the bottom line */
                prt("", y + i + 1, x);
            }

            /* Hide the list */
            else {
                /* Hide list */
                redraw = false;
                screen_load();
            }

            /* Redo asking */
            continue;
        }

        /* Extract request */
        i = A2I(choice);

        /* Totally Illegal */
        if ((i < 0) || (i >= num)) {
            bell();
            continue;
        }

        /* Save the spell index */
        spell = monster_powers.at(mane_data->mane_list[i].spell);

        /* Stop the loop */
        flag = true;
    }
    if (redraw) {
        screen_load();
    }

    RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::SPELL);
    handle_stuff(creature);

    /* Abort if needed */
    if (!flag) {
        return false;
    }

    /* Save the choice */
    (*sn) = i;

    damage = (baigaesi ? mane_data->mane_list[i].damage * 2 : mane_data->mane_list[i].damage);

    /* Success */
    return true;
}

/*!
 * @brief ものまね処理の発動 /
 * do_cmd_cast calls this function if the creature's class is 'imitator'.
 * @param creature クリーチャーへの参照
 * @param spell 発動するモンスター攻撃のID
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
static bool use_mane(CreatureEntity &creature, MonsterAbilityType spell)
{
    PLAYER_LEVEL plev = static_cast<PLAYER_LEVEL>(creature.get_level());
    BIT_FLAGS mode = (PM_ALLOW_GROUP | PM_FORCE_PET);
    BIT_FLAGS u_mode = 0L;

    if (randint1(50 + plev) < plev / 10) {
        u_mode = PM_ALLOW_UNIQUE;
    }

    const auto projection = find_mspell_projection(spell);
    auto dir = Direction::none();
    if (projection || AIMING_SPELLS.contains(spell)) {
        dir = get_aim_dir(creature);
        if (!dir) {
            return false;
        }
    }

    if (projection) {
        fire_mspell_projection(creature, *projection, dir, damage, (plev > 35 ? 3 : 2));
        return true;
    }

    /* spell code */
    switch (spell) {
    case MonsterAbilityType::SHRIEK:
        msg_print(_("かん高い金切り声をあげた。", "You make a high pitched shriek."));
        aggravate_monsters(creature, 0);
        break;

    case MonsterAbilityType::XXX1:
        break;

    case MonsterAbilityType::DISPEL: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }

        const auto &floor = *creature.get_floor();
        const auto &grid = floor.get_grid(*pos);
        const auto m_idx = grid.m_idx;
        const auto p_pos = creature.get_position();
        auto should_dispel = m_idx == 0;
        should_dispel &= grid.has_los();
        should_dispel &= projectable(floor, p_pos, *pos);
        if (!should_dispel) {
            break;
        }

        dispel_monster_status(creature, m_idx);
        break;
    }

    case MonsterAbilityType::ROCKET:
        msg_print(_("ロケットを発射した。", "You fire a rocket."));
        fire_rocket(creature, AttributeType::ROCKET, dir, damage, 2);
        break;

    case MonsterAbilityType::SHOOT:
        msg_print(_("矢を放った。", "You fire an arrow."));
        fire_bolt(creature, AttributeType::MONSTER_SHOOT, dir, damage);
        break;

    case MonsterAbilityType::XXX2:
        break;

    case MonsterAbilityType::XXX3:
        break;

    case MonsterAbilityType::XXX4:
        break;

    case MonsterAbilityType::DRAIN_MANA:
        fire_ball_hide(creature, AttributeType::DRAIN_MANA, dir, randint1(plev * 3) + plev, 0);
        break;
    case MonsterAbilityType::MIND_BLAST:
        fire_ball_hide(creature, AttributeType::MIND_BLAST, dir, damage, 0);
        break;
    case MonsterAbilityType::BRAIN_SMASH:
        fire_ball_hide(creature, AttributeType::BRAIN_SMASH, dir, damage, 0);
        break;
    case MonsterAbilityType::CAUSE_1:
        fire_ball_hide(creature, AttributeType::CAUSE_1, dir, damage, 0);
        break;
    case MonsterAbilityType::CAUSE_2:
        fire_ball_hide(creature, AttributeType::CAUSE_2, dir, damage, 0);
        break;
    case MonsterAbilityType::CAUSE_3:
        fire_ball_hide(creature, AttributeType::CAUSE_3, dir, damage, 0);
        break;
    case MonsterAbilityType::CAUSE_4:
        fire_ball_hide(creature, AttributeType::CAUSE_4, dir, damage, 0);
        break;
    case MonsterAbilityType::SCARE:
        msg_print(_("恐ろしげな幻覚を作り出した。", "You cast a fearful illusion."));
        fear_monster(creature, dir, plev + 10);
        break;
    case MonsterAbilityType::BLIND:
        confuse_monster(creature, dir, plev * 2);
        break;
    case MonsterAbilityType::CONF:
        msg_print(_("誘惑的な幻覚をつくり出した。", "You cast a mesmerizing illusion."));
        confuse_monster(creature, dir, plev * 2);
        break;
    case MonsterAbilityType::SLOW:
        slow_monster(creature, dir, plev);
        break;
    case MonsterAbilityType::HOLD:
        sleep_monster(creature, dir, plev);
        break;
    case MonsterAbilityType::HASTE:
        (void)set_acceleration(creature, randint1(20 + plev) + plev, false);
        break;
    case MonsterAbilityType::HAND_DOOM: {
        msg_print(_("<破滅の手>を放った！", "You invoke the Hand of Doom!"));
        fire_ball_hide(creature, AttributeType::HAND_DOOM, dir, 200, 0);
        break;
    }
    case MonsterAbilityType::HEAL: {
        msg_print(_("自分の傷に念を集中した。", "You concentrate on your wounds!"));
        (void)hp_player(creature, plev * 6);
        BadStatusSetter bss(creature);
        (void)bss.set_stun(0);
        (void)bss.set_cut(0);
        break;
    }
    case MonsterAbilityType::INVULNER:
        msg_print(_("無傷の球の呪文を唱えた。", "You cast a Globe of Invulnerability."));
        (void)set_invuln(creature, randint1(7) + 7, false);
        break;
    case MonsterAbilityType::BLINK:
        teleport_player(creature, 10, TELEPORT_SPONTANEOUS);
        break;
    case MonsterAbilityType::TPORT:
        teleport_player(creature, plev * 5, TELEPORT_SPONTANEOUS);
        break;
    case MonsterAbilityType::WORLD:
        (void)time_walk(creature);
        break;
    case MonsterAbilityType::SPECIAL:
        break;
    case MonsterAbilityType::TELE_TO: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }

        const auto &floor = *creature.get_floor();
        const auto &grid_target = floor.get_grid(*pos);
        const auto p_pos = creature.get_position();
        auto should_teleport = grid_target.has_monster();
        should_teleport &= grid_target.has_los();
        should_teleport &= projectable(floor, p_pos, *pos);
        if (!should_teleport) {
            break;
        }

        const auto &monster = floor.get_monster(grid_target.m_idx);
        auto &monrace = monster.get_monrace();
        const auto m_name = monster_desc(creature, monster, 0);
        if (monrace.resistance_flags.has(MonsterResistanceType::RESIST_TELEPORT)) {
            if (monrace.kind_flags.has(MonsterKindType::UNIQUE) || monrace.resistance_flags.has(MonsterResistanceType::RESIST_ALL)) {
                if (is_original_ap_and_seen(creature, monster)) {
                    monrace.r_resistance_flags.set(MonsterResistanceType::RESIST_TELEPORT);
                }
                msg_format(_("%sには効果がなかった！", "%s is unaffected!"), m_name.data());

                break;
            } else if (monrace.level > randint1(100)) {
                if (is_original_ap_and_seen(creature, monster)) {
                    monrace.r_resistance_flags.set(MonsterResistanceType::RESIST_TELEPORT);
                }
                msg_format(_("%sには耐性がある！", "%s resists!"), m_name.data());

                break;
            }
        }
        msg_format(_("%sを引き戻した。", "You command %s to return."), m_name.data());

        teleport_monster_to(creature, grid_target.m_idx, creature.y, creature.x, 100, TELEPORT_PASSIVE);
        break;
    }
    case MonsterAbilityType::TELE_AWAY:
        (void)fire_beam(creature, AttributeType::AWAY_ALL, dir, plev);
        break;

    case MonsterAbilityType::TELE_LEVEL:
        return teleport_level_other(creature);
        break;

    case MonsterAbilityType::PSY_SPEAR:
        msg_print(_("光の剣を放った。", "You throw a psycho-spear."));
        (void)fire_beam(creature, AttributeType::PSY_SPEAR, dir, damage);
        break;

    case MonsterAbilityType::DARKNESS:
        msg_print(_("暗闇の中で手を振った。", "You gesture in shadow."));
        (void)unlite_area(creature, 10, 3);
        break;

    case MonsterAbilityType::TRAPS: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("呪文を唱えて邪悪に微笑んだ。", "You cast a spell and cackle evilly."));
        trap_creation(creature, pos->y, pos->x);
        break;
    }
    case MonsterAbilityType::FORGET:
        msg_print(_("しかし何も起きなかった。", "Nothing happens."));
        break;
    case MonsterAbilityType::RAISE_DEAD:
        msg_print(_("死者復活の呪文を唱えた。", "You animate the dead."));
        (void)animate_dead(creature, 0, creature.y, creature.x);
        break;
    case MonsterAbilityType::S_KIN: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }

        msg_print(_("援軍を召喚した。", "You summon minions."));
        for (auto k = 0; k < 4; k++) {
            (void)summon_kin_player(creature, plev, pos->y, pos->x, (PM_FORCE_PET | PM_ALLOW_GROUP));
        }
        break;
    }
    case MonsterAbilityType::S_CYBER: {
        int max_cyber = (creature.get_floor()->dun_level / 50) + randint1(3);
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("サイバーデーモンを召喚した！", "You summon Cyberdemons!"));
        if (max_cyber > 4) {
            max_cyber = 4;
        }
        for (auto k = 0; k < max_cyber; k++) {
            summon_specific(creature, pos->y, pos->x, plev, SUMMON_CYBER, mode);
        }
        break;
    }
    case MonsterAbilityType::S_MONSTER: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("仲間を召喚した。", "You summon help."));
        for (auto k = 0; k < 1; k++) {
            summon_specific(creature, pos->y, pos->x, plev, SUMMON_NONE, (mode | u_mode));
        }
        break;
    }
    case MonsterAbilityType::S_MONSTERS: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("モンスターを召喚した！", "You summon monsters!"));
        for (auto k = 0; k < 6; k++) {
            summon_specific(creature, pos->y, pos->x, plev, SUMMON_NONE, (mode | u_mode));
        }
        break;
    }
    case MonsterAbilityType::S_ANT: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("アリを召喚した。", "You summon ants."));
        for (auto k = 0; k < 6; k++) {
            summon_specific(creature, pos->y, pos->x, plev, SUMMON_ANT, mode);
        }
        break;
    }
    case MonsterAbilityType::S_SPIDER: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("蜘蛛を召喚した。", "You summon spiders."));
        for (auto k = 0; k < 6; k++) {
            summon_specific(creature, pos->y, pos->x, plev, SUMMON_SPIDER, mode);
        }
        break;
    }
    case MonsterAbilityType::S_HOUND: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("ハウンドを召喚した。", "You summon hounds."));
        for (auto k = 0; k < 4; k++) {
            summon_specific(creature, pos->y, pos->x, plev, SUMMON_HOUND, mode);
        }
        break;
    }
    case MonsterAbilityType::S_HYDRA: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("ヒドラを召喚した。", "You summon hydras."));
        for (auto k = 0; k < 4; k++) {
            summon_specific(creature, pos->y, pos->x, plev, SUMMON_HYDRA, mode);
        }
        break;
    }
    case MonsterAbilityType::S_FAIRY: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("フェアリーを召喚した。", "You summon fairies."));
        for (auto k = 0; k < 4; k++) {
            summon_specific(creature, pos->y, pos->x, plev, SUMMON_FAIRY, mode);
        }
        break;
    }
    case MonsterAbilityType::S_APE: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("類人猿を召喚した。", "You summon apes."));
        for (auto k = 0; k < 4; k++) {
            summon_specific(creature, pos->y, pos->x, plev, SUMMON_APE, mode);
        }
        break;
    }
    case MonsterAbilityType::S_BIRD: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("鳥を召喚した。", "You summon birds."));
        for (auto k = 0; k < 4; k++) {
            summon_specific(creature, pos->y, pos->x, plev, SUMMON_BIRD, mode);
        }
        break;
    }
    case MonsterAbilityType::S_INSECT: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("昆虫を召喚した！", "You summon insects!"));
        for (auto k = 0; k < 4; k++) {
            summon_specific(creature, pos->y, pos->x, plev, SUMMON_INSECT, mode);
        }
        break;
    }
    case MonsterAbilityType::S_ELDRAZI: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("エルドラージを召喚した！", "You summon Eldrazi!"));
        for (auto k = 0; k < std::max(1, plev / 30); k++) {
            summon_specific(creature, pos->y, pos->x, plev, SUMMON_ELDRAZI, mode);
        }
        break;
    }
    case MonsterAbilityType::S_ROBOT: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("ロボットを召喚した！", "You summon robots!"));
        for (auto k = 0; k < 3; k++) {
            summon_specific(creature, pos->y, pos->x, plev, SUMMON_ROBOT, (mode | PM_ALLOW_GROUP));
        }
        break;
    }
    case MonsterAbilityType::S_ANGEL: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("天使を召喚した！", "You summon an angel!"));
        for (auto k = 0; k < 1; k++) {
            summon_specific(creature, pos->y, pos->x, plev, SUMMON_ANGEL, mode);
        }
        break;
    }
    case MonsterAbilityType::S_DEMON: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("混沌の宮廷から悪魔を召喚した！", "You summon a demon from the Courts of Chaos!"));
        for (auto k = 0; k < 1; k++) {
            summon_specific(creature, pos->y, pos->x, plev, SUMMON_DEMON, (mode | u_mode));
        }
        break;
    }
    case MonsterAbilityType::S_UNDEAD: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("アンデッドの強敵を召喚した！", "You summon an undead adversary!"));
        for (auto k = 0; k < 1; k++) {
            summon_specific(creature, pos->y, pos->x, plev, SUMMON_UNDEAD, (mode | u_mode));
        }
        break;
    }
    case MonsterAbilityType::S_DRAGON: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("ドラゴンを召喚した！", "You summon a dragon!"));
        for (auto k = 0; k < 1; k++) {
            summon_specific(creature, pos->y, pos->x, plev, SUMMON_DRAGON, (mode | u_mode));
        }
        break;
    }
    case MonsterAbilityType::S_HI_UNDEAD: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("強力なアンデッドを召喚した！", "You summon greater undead!"));
        for (auto k = 0; k < 6; k++) {
            summon_specific(creature, pos->y, pos->x, plev, SUMMON_HI_UNDEAD, (mode | u_mode));
        }
        break;
    }
    case MonsterAbilityType::S_HI_DRAGON: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("古代ドラゴンを召喚した！", "You summon ancient dragons!"));
        for (auto k = 0; k < 4; k++) {
            summon_specific(creature, pos->y, pos->x, plev, SUMMON_HI_DRAGON, (mode | u_mode));
        }
        break;
    }
    case MonsterAbilityType::S_AMBERITES: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("アンバーの王族を召喚した！", "You summon Lords of Amber!"));
        for (auto k = 0; k < 4; k++) {
            summon_specific(creature, pos->y, pos->x, plev, SUMMON_AMBERITES, (mode | PM_ALLOW_UNIQUE));
        }
        break;
    }
    case MonsterAbilityType::S_CHOASIANS: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("混沌の王族を召喚した！", "You summon Lords of Chaos!"));
        for (auto k = 0; k < 4; k++) {
            summon_specific(creature, pos->y, pos->x, plev, SUMMON_CHOASIANS, (mode | PM_ALLOW_UNIQUE));
        }
        break;
    }
    case MonsterAbilityType::S_UNIQUE: {
        int count = 0;
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("特別な強敵を召喚した！", "You summon special opponents!"));
        for (auto k = 0; k < 4; k++) {
            if (summon_specific(creature, pos->y, pos->x, plev, SUMMON_UNIQUE, (mode | PM_ALLOW_UNIQUE))) {
                count++;
            }
        }
        for (auto k = count; k < 4; k++) {
            summon_specific(creature, pos->y, pos->x, plev, SUMMON_HI_UNDEAD, (mode | u_mode));
        }
        break;
    }
    case MonsterAbilityType::S_DEAD_UNIQUE: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("特別な強敵を蘇生した！", "You summon special dead opponents!"));
        for (auto k = 0; k < 4; k++) {
            summon_specific(creature, pos->y, pos->x, plev, SUMMON_DEAD_UNIQUE, (mode | PM_ALLOW_UNIQUE | PM_CLONE));
        }
        break;
    }
    case MonsterAbilityType::S_NASTY: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("汚い怪物を召喚した！", "You summon nasty monsters!"));
        for (auto k = 0; k < 6; k++) {
            summon_specific(creature, pos->y, pos->x, plev, SUMMON_NASTY, (mode | PM_ALLOW_GROUP));
        }
        break;
    }
    case MonsterAbilityType::S_GOLEM: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("ゴーレムを召喚した！", "You summon golems!"));
        for (auto k = 0; k < 3; k++) {
            summon_specific(creature, pos->y, pos->x, plev, SUMMON_GOLEM, (mode | PM_ALLOW_GROUP));
        }
        break;
    }
    case MonsterAbilityType::S_CAT: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("猫を召喚した！", "You summon cats!"));
        for (auto k = 0; k < 3; k++) {
            summon_specific(creature, pos->y, pos->x, plev, SUMMON_CATS, (mode | PM_ALLOW_GROUP));
        }
        break;
    }
    case MonsterAbilityType::S_PERVERT: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("変質者を召喚した！", "You summon perverts!"));
        for (auto k = 0; k < 3; k++) {
            summon_specific(creature, pos->y, pos->x, plev, SUMMON_PERVERTS, (mode | PM_ALLOW_GROUP));
        }
        break;
    }
    case MonsterAbilityType::S_PUYO: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("ぷよを召喚した！", "You summon puyo!"));
        for (auto k = 0; k < 3; k++) {
            summon_specific(creature, pos->y, pos->x, plev, SUMMON_PUYO, (mode | PM_ALLOW_GROUP));
        }
        break;
    }
    case MonsterAbilityType::S_HOMO: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("ホモを召喚した！", "You summon homos!"));
        for (auto k = 0; k < 3; k++) {
            summon_specific(creature, pos->y, pos->x, plev, SUMMON_HOMO, (mode | PM_ALLOW_GROUP));
        }
        break;
    }
    case MonsterAbilityType::S_WALL: {
        const auto pos = target_set(creature, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }
        msg_print(_("壁を召喚した！", "You summon walls!"));
        for (auto k = 0; k < 1; k++) {
            summon_specific(creature, pos->y, pos->x, plev, SUMMON_WALL, (mode | u_mode));
        }
        break;
    }
    default:
        msg_print("hoge?");
    }

    return true;
}

/*!
 * @brief ものまねコマンドのメインルーチン /
 * do_cmd_cast calls this function if the creature's class is 'imitator'.
 * @param baigaesi TRUEならば倍返し上の処理として行う
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 * @details
 * If a valid spell is chosen, saves it in '*sn' and returns TRUE
 * If the user hits escape, returns FALSE, and set '*sn' to -1
 * If there are no legal choices, returns FALSE, and sets '*sn' to -2
 *
 * The "prompt" should be "cast", "recite", or "study"
 * The "known" should be TRUE for cast/pray, false for study
 *
 * nb: This function has a (trivial) display bug which will be obvious
 * when you run it. It's probably easy to fix but I haven't tried,
 * sorry.
 */
bool do_cmd_mane(CreatureEntity &creature, bool baigaesi)
{
    int n = 0;
    PERCENTAGE chance;
    PERCENTAGE minfail = 0;
    PLAYER_LEVEL plev = static_cast<PLAYER_LEVEL>(creature.get_level());
    monster_power spell;
    bool cast;

    if (cmd_limit_confused(creature)) {
        return false;
    }

    auto mane_data = CreatureClass(creature).get_specific_data<mane_data_type>();

    if (mane_data->mane_list.empty()) {
        msg_print(_("まねられるものが何もない！", "You don't remember any action!"));
        return false;
    }

    if (!get_mane_power(creature, &n, baigaesi)) {
        return false;
    }

    spell = monster_powers.at(mane_data->mane_list[n].spell);

    /* Spell failure chance */
    chance = spell.manefail;

    /* Reduce failure rate by "effective" level adjustment */
    if (plev > spell.level) {
        chance -= 3 * (plev - spell.level);
    }

    /* Reduce failure rate by 1 stat and DEX adjustment */
    chance -= 3 * (adj_mag_stat[creature.get_stat_index(spell.use_stat)] + adj_mag_stat[creature.get_stat_index(A_DEX)] - 2) / 2;

    if (spell.manedam) {
        chance = chance * damage / spell.manedam;
    }

    chance += creature.get_to_m_chance();

    /* Extract the minimum failure rate */
    minfail = adj_mag_fail[creature.get_stat_index(spell.use_stat)];

    /* Minimum failure rate */
    if (chance < minfail) {
        chance = minfail;
    }

    chance += creature.get_stun_magic_chance_penalty();
    if (chance > 95) {
        chance = 95;
    }

    /* Failed spell */
    if (evaluate_percent(chance)) {
        if (flush_failure) {
            flush();
        }
        msg_print(_("ものまねに失敗した！", "You failed to concentrate hard enough!"));
        sound(SoundKind::FAIL);
    } else {
        sound(SoundKind::ZAP);
        cast = use_mane(creature, mane_data->mane_list[n].spell);
        if (!cast) {
            return false;
        }
    }

    mane_data->mane_list.erase(std::next(mane_data->mane_list.begin(), n));
    PlayerEnergy(creature).set_player_turn_energy(100);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::IMITATION);
    static constexpr auto flags = {
        SubWindowRedrawingFlag::PLAYER,
        SubWindowRedrawingFlag::SPELL,
    };
    rfu.set_flags(flags);
    return true;
}
