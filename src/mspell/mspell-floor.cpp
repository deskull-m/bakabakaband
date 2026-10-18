/*!
 * @brief フロアの一定範囲に効果を及ぼす (悲鳴、テレポート等)スペルの効果
 * @date 2020/05/16
 * @author Hourier
 */

#include "mspell/mspell-floor.h"
#include "blue-magic/blue-magic-checker.h"
#include "core/disturbance.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "mind/drs-types.h"
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-brightness-mask.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-resistance-mask.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "mspell/mspell-result.h"
#include "mspell/mspell-status.h"
#include "mspell/mspell-util.h"
#include "player-base/player-class.h"
#include "player/player-personality-types.h"
#include "player/player-status-flags.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-hex.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/redrawing-flags-updater.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <fmt/base.h>

/*!
 * @brief RF4_SHRIEKの処理。叫び。 /
 * @param m_idx 呪文を唱えるモンスターID
 * @param creature クリーチャーへの参照
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * ラーニング不可。
 */
MonsterSpellResult spell_RF4_SHRIEK(MONSTER_IDX m_idx, CreatureEntity &creature, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_simple msg(_("{}がかん高い金切り声をあげた。", "{} makes a high pitched shriek."),
        _("{}が{}に向かって叫んだ。", "{} shrieks at {}."));

    simple_monspell_message(creature, m_idx, t_idx, msg, target_type);

    const auto &monster = creature.get_floor()->get_monster(m_idx);
    auto result = MonsterSpellResult::make_valid();
    if (monster.get_r_idx() == MonraceId::LEE_QIEZI) {
        msg_print(_("しかし、その声は誰の心にも響かなかった…。", "However, that voice didn't touch anyone's heart..."));
        return result;
    }

    if (target_type == MONSTER_TO_PLAYER) {
        aggravate_monsters(creature, m_idx);
    } else if (target_type == MONSTER_TO_MONSTER) {
        set_monster_csleep(*creature.get_floor(), t_idx, 0);
    }

    return result;
}

/*!
 * @brief RF6_WORLDの処理。時を止める。 /
 * @param creature クリーチャーへの参照
 * @param m_idx 呪文を唱えるモンスターID
 *
 * ラーニング不可。
 */
MonsterSpellResult spell_RF6_WORLD(CreatureEntity &creature, MONSTER_IDX m_idx)
{
    disturb(creature, true, true);
    (void)set_monster_timewalk(creature, m_idx, randint1(2) + 2, true);

    return MonsterSpellResult::make_valid();
}

/*!
 * @brief RF6_BLINKの処理。ショート・テレポート。 /
 * @param creature クリーチャーへの参照
 * @param m_idx 呪文を唱えるモンスターID
 * @param is_quantum_effect 量子的効果によるショート・テレポートの場合時TRUE
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * ラーニング不可。
 */
MonsterSpellResult spell_RF6_BLINK(CreatureEntity &creature, MONSTER_IDX m_idx, int target_type, bool is_quantum_effect)
{
    const auto res = MonsterSpellResult::make_valid();
    const auto m_name = monster_name(creature, m_idx);

    if (target_type == MONSTER_TO_PLAYER) {
        disturb(creature, true, true);
    }

    if (!is_quantum_effect && SpellHex(creature).check_hex_barrier(m_idx, HEX_ANTI_TELE)) {
        if (see_monster(creature, m_idx)) {
            msg_print(_("魔法のバリアが{}のテレポートを邪魔した。", "Magic barrier obstructs teleporting of {}."), str_upcase_first(m_name));
        }
        return res;
    }

    if (see_monster(creature, m_idx)) {
        msg_print(_("{}が瞬時に消えた。", "{} blinks away."), str_upcase_first(m_name));
    }

    teleport_away(creature, m_idx, 10, TELEPORT_SPONTANEOUS);

    if (target_type == MONSTER_TO_PLAYER) {
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
    }

    return res;
}

/*!
 * @brief RF6_TPORTの処理。テレポート。 /
 * @param creature クリーチャーへの参照
 * @param m_idx 呪文を唱えるモンスターID
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * ラーニング不可。
 */
MonsterSpellResult spell_RF6_TPORT(CreatureEntity &creature, MONSTER_IDX m_idx, int target_type)
{
    const auto res = MonsterSpellResult::make_valid();
    const auto m_name = monster_name(creature, m_idx);

    if (target_type == MONSTER_TO_PLAYER) {
        disturb(creature, true, true);
    }
    if (SpellHex(creature).check_hex_barrier(m_idx, HEX_ANTI_TELE)) {
        if (see_monster(creature, m_idx)) {
            msg_print(_("魔法のバリアが{}のテレポートを邪魔した。", "Magic barrier obstructs teleporting of {}."), str_upcase_first(m_name));
        }
        return res;
    }

    if (see_monster(creature, m_idx)) {
        msg_print(_("{}がテレポートした。", "{} teleports away."), str_upcase_first(m_name));
    }

    teleport_away_followable(creature, m_idx);

    return res;
}

/*!
 * @brief RF6_TELE_TOの処理。テレポート・バック。 /
 * @param creature クリーチャーへの参照
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_TELE_TO(CreatureEntity &creature, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    const auto &floor = *creature.get_floor();
    const auto &monster = floor.get_monster(m_idx);
    const auto &monster_target = floor.get_monster(t_idx);
    auto &monrace_target = monster_target.get_monrace();

    mspell_cast_msg_simple msg(_("{}があなたを引き戻した。", "{} commands you to return."),
        _("{}が{}を引き戻した。", "{} commands {} to return."));

    simple_monspell_message(creature, m_idx, t_idx, msg, target_type);

    if (target_type == MONSTER_TO_PLAYER) {
        teleport_player_to(creature, monster.y, monster.x, TELEPORT_PASSIVE);
        return res;
    }

    if (target_type != MONSTER_TO_MONSTER) {
        return res;
    }

    bool resists_tele = false;
    const auto t_name = monster_name(creature, t_idx);

    if (monrace_target.resistance_flags.has(MonsterResistanceType::RESIST_TELEPORT)) {
        if (monrace_target.kind_flags.has(MonsterKindType::UNIQUE) || monrace_target.resistance_flags.has(MonsterResistanceType::RESIST_ALL)) {
            if (is_original_ap_and_seen(creature, monster_target)) {
                monrace_target.r_resistance_flags.set(MonsterResistanceType::RESIST_TELEPORT);
            }
            if (see_monster(creature, t_idx)) {
                msg_print(_("{}には効果がなかった。", "{} is unaffected!"), str_upcase_first(t_name));
            }
            resists_tele = true;
        } else if (monrace_target.level > randint1(100)) {
            if (is_original_ap_and_seen(creature, monster_target)) {
                monrace_target.r_resistance_flags.set(MonsterResistanceType::RESIST_TELEPORT);
            }
            if (see_monster(creature, t_idx)) {
                msg_print(_("{}は耐性を持っている！", "{} resists!"), str_upcase_first(t_name));
            }
            resists_tele = true;
        }
    }

    if (resists_tele) {
        set_monster_csleep(*creature.get_floor(), t_idx, 0);
        return res;
    }

    if (monster_target.is_riding()) {
        teleport_player_to(creature, monster.y, monster.x, TELEPORT_PASSIVE);
    } else {
        teleport_monster_to(creature, t_idx, monster.y, monster.x, 100, TELEPORT_PASSIVE);
    }
    set_monster_csleep(*creature.get_floor(), t_idx, 0);

    return res;
}

/*!
 * @brief RF6_TELE_AWAYの処理。テレポート・アウェイ。 /
 * @param creature クリーチャーへの参照
 * @param m_idx 伜文を唱えるモンスターID
 * @param t_idx 伜文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_TELE_AWAY(CreatureEntity &creature, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    const auto &floor = *creature.get_floor();
    const auto &monster_target = floor.get_monster(t_idx);
    auto &monrace_target = monster_target.get_monrace();

    mspell_cast_msg_simple msg(_("{}にテレポートさせられた。", "{} teleports you away."),
        _("{}は{}をテレポートさせた。", "{} teleports {} away."));

    simple_monspell_message(creature, m_idx, t_idx, msg, target_type);

    if (target_type == MONSTER_TO_PLAYER) {
        if (creature.is_echizen()) {
            msg_print(_("くっそ～", ""));
        } else if (creature.is_chargeman()) {
            if (randint0(2) == 0) {
                msg_print(_("ジュラル星人め！", ""));
            } else {
                msg_print(_("弱い者いじめは止めるんだ！", ""));
            }
        } else if (creature.is_tough()) {
            msg_print(_("う わ あ あ あ あ あ あ あ あ", ""));
        }

        teleport_player_away(m_idx, creature, 100, false);
        return res;
    }

    if (target_type != MONSTER_TO_MONSTER) {
        return res;
    }

    bool resists_tele = false;
    const auto t_name = monster_name(creature, t_idx);

    if (monrace_target.resistance_flags.has(MonsterResistanceType::RESIST_TELEPORT)) {
        if (monrace_target.kind_flags.has(MonsterKindType::UNIQUE) || monrace_target.resistance_flags.has(MonsterResistanceType::RESIST_ALL)) {
            if (is_original_ap_and_seen(creature, monster_target)) {
                monrace_target.r_resistance_flags.set(MonsterResistanceType::RESIST_TELEPORT);
            }
            if (see_monster(creature, t_idx)) {
                msg_print(_("{}には効果がなかった。", "{} is unaffected!"), str_upcase_first(t_name));
            }
            resists_tele = true;
        } else if (monrace_target.level > randint1(100)) {
            if (is_original_ap_and_seen(creature, monster_target)) {
                monrace_target.r_resistance_flags.set(MonsterResistanceType::RESIST_TELEPORT);
            }
            if (see_monster(creature, t_idx)) {
                msg_print(_("{}は耐性を持っている！", "{} resists!"), str_upcase_first(t_name));
            }
            resists_tele = true;
        }
    }

    if (resists_tele) {
        set_monster_csleep(*creature.get_floor(), t_idx, 0);
        return res;
    }

    if (monster_target.is_riding()) {
        teleport_player_away(m_idx, creature, MAX_PLAYER_SIGHT * 2 + 5, false);
    } else {
        teleport_away(creature, t_idx, MAX_PLAYER_SIGHT * 2 + 5, TELEPORT_PASSIVE);
    }
    set_monster_csleep(*creature.get_floor(), t_idx, 0);

    return res;
}

/*!
 * @brief RF6_TELE_LEVELの処理。テレポート・レベル。 /
 * @param creature クリーチャーへの参照
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * ラーニング不可。
 */
MonsterSpellResult spell_RF6_TELE_LEVEL(CreatureEntity &creature, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    const auto res = MonsterSpellResult::make_valid();

    const auto &floor = *creature.get_floor();
    const auto &monster_target = floor.get_monster(t_idx);
    const auto &monrace_target = monster_target.get_monrace();
    DEPTH rlev = monster_level_idx(floor, m_idx);
    bool resist, saving_throw;

    if (target_type == MONSTER_TO_PLAYER) {
        resist = (creature.has_resist_shard() != 0);
        saving_throw = (creature.does_save_against(rlev));

        mspell_cast_msg_bad_status_to_player msg(_("{}が何か奇妙な言葉をつぶやいた。", "{} mumbles strangely."),
            _("{}があなたの足を指さした。", "{} gestures at your feet."), _("しかし効果がなかった！", "You are unaffected!"),
            _("しかし効力を跳ね返した！", "You resist the effects!"));

        spell_badstatus_message_to_player(creature, m_idx, msg, resist, saving_throw);

        if (!resist && !saving_throw) {
            teleport_level(creature, 0);
        }

        update_smart_learn(creature, m_idx, DRS_NEXUS);
        return res;
    }

    if (target_type != MONSTER_TO_MONSTER) {
        return res;
    }

    resist = monrace_target.resistance_flags.has_any_of(RFR_EFF_RESIST_NEXUS_MASK) || monrace_target.resistance_flags.has(MonsterResistanceType::RESIST_TELEPORT);
    saving_throw = (monrace_target.misc_flags.has(MonsterMiscType::QUESTOR)) || (monrace_target.level > randint1((rlev - 10) < 1 ? 1 : (rlev - 10)) + 10);

    mspell_cast_msg_bad_status_to_monster msg(_("{}が{}の足を指さした。", "{} gestures at {}'s feet."),
        _("{}には効果がなかった。", "{} is unaffected!"), _("{}は効力を跳ね返した！", "{} resist the effects!"), "");

    spell_badstatus_message_to_mons(creature, m_idx, t_idx, msg, resist, saving_throw);

    if (!resist && !saving_throw) {
        teleport_level(creature, monster_target.is_riding() ? 0 : t_idx);
    }

    return res;
}

/*!
 * @brief RF6_DARKNESSの処理。暗闇or閃光。 /
 * @param creature クリーチャーへの参照
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象かつ暗闇ならラーニング可。
 */
MonsterSpellResult spell_RF6_DARKNESS(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    const Pos2D pos(y, x);
    mspell_cast_msg_blind msg;
    const auto &floor = *creature.get_floor();
    const auto &monster = floor.get_monster(m_idx);
    const auto &monrace = monster.get_monrace();
    bool can_use_lite_area = false;
    bool monster_to_monster = target_type == MONSTER_TO_MONSTER;
    bool monster_to_player = target_type == MONSTER_TO_PLAYER;
    const auto t_name = monster_name(creature, t_idx);

    const auto is_ninja = CreatureClass(creature).equals(PlayerClassType::NINJA);
    const auto is_living_monster = monrace.kind_flags.has_not(MonsterKindType::UNDEAD);
    const auto is_not_weak_lite = monrace.resistance_flags.has_not(MonsterResistanceType::HURT_LITE);
    if (is_ninja && is_living_monster && is_not_weak_lite && monrace.brightness_flags.has_none_of(dark_mask)) {
        can_use_lite_area = true;
    }

    const auto &t_ref = floor.get_monster(t_idx);
    if (monster_to_monster && !t_ref.is_hostile()) {
        can_use_lite_area = false;
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = monster_to_player && !can_use_lite_area;

    if (can_use_lite_area) {
        msg.blind = _("{}が何かをつぶやいた。", "{} mumbles.");
        msg.to_player = _("{}が辺りを明るく照らした。", "{} cast a spell to light up.");
        msg.to_mons = _("{}が辺りを明るく照らした。", "{} cast a spell to light up.");
    } else {
        msg.blind = _("{}が何かをつぶやいた。", "{} mumbles.");
        msg.to_player = _("{}が暗闇の中で手を振った。", "{} gestures in shadow.");
        msg.to_mons = _("{}が暗闇の中で手を振った。", "{} gestures in shadow.");
    }

    monspell_message(creature, m_idx, t_idx, msg, target_type);

    using DoneFormat = fmt::format_string<std::string_view>;
    const auto msg_done = can_use_lite_area ? DoneFormat(_("{}は白い光に包まれた。", "{} is surrounded by a white light.")) : DoneFormat(_("{}は暗闇に包まれた。", "{} is surrounded by darkness."));
    if (see_monster(creature, t_idx) && monster_to_monster) {
        msg_print(msg_done, std::string_view(str_upcase_first(t_name)));
    }

    if (monster_to_player) {
        if (can_use_lite_area) {
            (void)lite_area(creature, 0, 3);
        } else {
            (void)unlite_area(creature, 0, 3);
        }
    } else if (monster_to_monster) {
        if (can_use_lite_area) {
            (void)project(creature, m_idx, 3, y, x, 0, AttributeType::LITE_WEAK, PROJECT_GRID | PROJECT_KILL);
            lite_room(creature, pos);
        } else {
            (void)project(creature, m_idx, 3, y, x, 0, AttributeType::DARK_WEAK, PROJECT_GRID | PROJECT_KILL);
            unlite_room(creature, pos);
        }
    }

    return res;
}

/*!
 * @brief RF6_TRAPSの処理。トラップ。 /
 * @param creature クリーチャーへの参照
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 *
 * ラーニング可。
 */
MonsterSpellResult spell_RF6_TRAPS(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx)
{
    const auto m_name = monster_name(creature, m_idx);
    disturb(creature, true, true);

    if (creature.is_blind()) {
        msg_print(_("{}が何かをつぶやいて邪悪に微笑んだ。", "{} mumbles, and then cackles evilly."), str_upcase_first(m_name));
    } else {
        msg_print(_("{}が呪文を唱えて邪悪に微笑んだ。", "{} casts a spell and cackles evilly."), str_upcase_first(m_name));
    }

    (void)trap_creation(creature, y, x);

    auto res = MonsterSpellResult::make_valid();
    res.learnable = true;

    return res;
}

/*!
 * @brief RF6_RAISE_DEADの処理。死者復活。 /
 * @param creature クリーチャーへの参照
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * ラーニング不可。
 */
MonsterSpellResult spell_RF6_RAISE_DEAD(CreatureEntity &creature, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    const auto &monster = creature.get_floor()->get_monster(m_idx);
    mspell_cast_msg_blind msg(_("{}が何かをつぶやいた。", "{} mumbles."),
        _("{}が死者復活の呪文を唱えた。", "{} casts a spell to revive corpses."), _("{}が死者復活の呪文を唱えた。", "{} casts a spell to revive corpses."));

    monspell_message(creature, m_idx, t_idx, msg, target_type);

    animate_dead(creature, m_idx, monster.y, monster.x);

    return MonsterSpellResult::make_valid();
}
