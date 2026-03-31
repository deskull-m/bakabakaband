#include "status/bad-status-setter.h"
#include "avatar/avatar.h"
#include "core/disturbance.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "game-option/disturbance-options.h"
#include "mind/mind-sniper.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/bluemage-data-type.h"
#include "player-info/monk-data-type.h"
#include "player/attack-defense-types.h"
#include "player/player-status-flags.h"
#include "player/player-status.h"
#include "player/special-defense-types.h"
#include "spell-realm/spells-hex.h"
#include "status/base-status.h"
#include "status/buff-setter.h"
#include "system/angband-exceptions.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"
#include <algorithm>

BadStatusSetter::BadStatusSetter(CreatureEntity &creature)
    : creature(creature)
{
}

/*!
 * @brief 盲目の継続時間をセットする / Set "blind", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 * @details
 * Note the use of "PU_UN_LITE" and "PU_UN_VIEW", which is needed to\n
 * memorize any terrain features which suddenly become "visible".\n
 * Note that blindness is currently the only thing which can affect\n
 * "player_can_see_bold()".\n
 */
bool BadStatusSetter::set_blindness(const TIME_EFFECT tmp_v)
{
    auto &player = static_cast<PlayerType &>(this->creature);
    auto notice = false;
    auto v = std::clamp<short>(tmp_v, 0, 10000);
    if (player.is_dead()) {
        return false;
    }

    CreatureRace pr(&player);
    auto &blindness = player.effects()->blindness();
    const auto is_blind = blindness.is_blind();
    if (v > 0) {
        if (!is_blind) {
            if (pr.equals(PlayerRaceType::ANDROID)) {
                msg_print(_("センサーをやられた！", "The sensor broke!"));
            } else {
                msg_print(_("目が見えなくなってしまった！", "You are blind!"));
            }

            notice = true;
            chg_virtue(this->creature, Virtue::ENLIGHTEN, -1);
        }
    } else {
        if (is_blind) {
            if (pr.equals(PlayerRaceType::ANDROID)) {
                msg_print(_("センサーが復旧した。", "The sensor has been restored."));
            } else {
                msg_print(_("やっと目が見えるようになった。", "You can see again."));
            }

            notice = true;
        }
    }

    blindness.set(v);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(player, false, false);
    }

    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::UN_VIEW,
        StatusRecalculatingFlag::UN_LITE,
        StatusRecalculatingFlag::VIEW,
        StatusRecalculatingFlag::LITE,
        StatusRecalculatingFlag::MONSTER_STATUSES,
        StatusRecalculatingFlag::MONSTER_LITE,
    };
    rfu.set_flags(flags_srf);
    rfu.set_flag(MainWindowRedrawingFlag::MAP);
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::OVERHEAD,
        SubWindowRedrawingFlag::DUNGEON,
    };
    rfu.set_flags(flags_swrf);
    handle_stuff(player);
    return true;
}

bool BadStatusSetter::mod_blindness(const TIME_EFFECT tmp_v)
{
    auto &player = static_cast<PlayerType &>(this->creature);
    return this->set_blindness(player.effects()->blindness().current() + tmp_v);
}

/*!
 * @brief 混乱の継続時間をセットする / Set "confused", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool BadStatusSetter::set_confusion(const TIME_EFFECT tmp_v)
{
    auto &player = static_cast<PlayerType &>(this->creature);
    auto notice = false;
    auto v = std::clamp<short>(tmp_v, 0, 10000);
    if (player.is_dead()) {
        return false;
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    const auto is_confused = player.is_confused();
    if (v > 0) {
        if (!is_confused) {
            msg_print(_("あなたは混乱した！", "You are confused!"));
            if (player.action == ACTION_LEARN) {
                msg_print(_("学習が続けられない！", "You cannot continue learning!"));
                auto bluemage_data = CreatureClass(player).get_specific_data<bluemage_data_type>();
                bluemage_data->new_magic_learned = false;
                rfu.set_flag(MainWindowRedrawingFlag::ACTION);
                player.action = ACTION_NONE;
            }
            if (player.action == ACTION_MONK_STANCE) {
                msg_print(_("構えがとけた。", "You lose your stance."));
                CreatureClass(player).set_monk_stance(MonkStanceType::NONE);
                rfu.set_flag(StatusRecalculatingFlag::BONUS);
                rfu.set_flag(MainWindowRedrawingFlag::ACTION);
                player.action = ACTION_NONE;
            } else if (player.action == ACTION_SAMURAI_STANCE) {
                msg_print(_("型が崩れた。", "You lose your stance."));
                CreatureClass(player).lose_balance();
            }

            /* Sniper */
            reset_concentration(player, true);

            SpellHex spell_hex(this->creature);
            if (spell_hex.is_spelling_any()) {
                (void)spell_hex.stop_all_spells();
            }

            notice = true;
            player.counter = false;
            chg_virtue(this->creature, Virtue::HARMONY, -1);
        }
    } else {
        if (is_confused) {
            msg_print(_("やっと混乱がおさまった。", "You feel less confused now."));
            player.special_attack &= ~(ATTACK_SUIKEN);
            notice = true;
        }
    }

    player.effects()->confusion().set(v);
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(player, false, false);
    }

    handle_stuff(player);
    return true;
}

bool BadStatusSetter::mod_confusion(const TIME_EFFECT tmp_v)
{
    auto &player = static_cast<PlayerType &>(this->creature);
    return this->set_confusion(player.effects()->confusion().current() + tmp_v);
}

/*!
 * @brief 毒の継続時間をセットする / Set "poisoned", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool BadStatusSetter::set_poison(const TIME_EFFECT tmp_v)
{
    auto &player = static_cast<PlayerType &>(this->creature);
    auto notice = false;
    auto v = std::clamp<short>(tmp_v, 0, 10000);
    if (player.is_dead()) {
        return false;
    }

    auto &player_poison = player.effects()->poison();
    const auto is_poisoned = player_poison.is_poisoned();
    if (v > 0) {
        if (!is_poisoned) {
            msg_print(_("毒に侵されてしまった！", "You are poisoned!"));
            notice = true;
        }
    } else {
        if (is_poisoned) {
            msg_print(_("やっと毒の痛みがなくなった。", "You are no longer poisoned."));
            notice = true;
        }
    }

    player_poison.set(v);
    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(player, false, false);
    }

    handle_stuff(player);
    return true;
}

bool BadStatusSetter::mod_poison(const TIME_EFFECT tmp_v)
{
    auto &player = static_cast<PlayerType &>(this->creature);
    return this->set_poison(player.effects()->poison().current() + tmp_v);
}

/*!
 * @brief 恐怖の継続時間をセットする / Set "fearful", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool BadStatusSetter::set_fear(const TIME_EFFECT tmp_v)
{
    auto &player = static_cast<PlayerType &>(this->creature);
    auto notice = false;
    auto v = std::clamp<short>(tmp_v, 0, 10000);
    if (player.is_dead()) {
        return false;
    }

    auto &fear = player.effects()->fear();
    if (v > 0) {
        if (!fear.is_fearful()) {
            msg_print(_("何もかも恐くなってきた！", "You are terrified!"));
            if (CreatureClass(player).lose_balance()) {
                msg_print(_("型が崩れた。", "You lose your stance."));
            }

            notice = true;
            player.counter = false;
            chg_virtue(this->creature, Virtue::VALOUR, -1);
        }
    } else {
        if (fear.is_fearful()) {
            msg_print(_("やっと恐怖を振り払った。", "You feel bolder now."));
            notice = true;
        }
    }

    fear.set(v);
    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(player, false, false);
    }

    handle_stuff(player);
    return true;
}

bool BadStatusSetter::mod_fear(const TIME_EFFECT tmp_v)
{
    auto &player = static_cast<PlayerType &>(this->creature);
    return this->set_fear(player.effects()->fear().current() + tmp_v);
}

/*!
 * @brief 麻痺の継続時間をセットする / Set "paralyzed", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool BadStatusSetter::set_paralysis(const TIME_EFFECT tmp_v)
{
    auto &player = static_cast<PlayerType &>(this->creature);
    auto notice = false;
    auto v = std::clamp<short>(tmp_v, 0, 10000);
    if (player.is_dead()) {
        return false;
    }

    auto &paralysis = player.effects()->paralysis();
    if (v > 0) {
        if (!paralysis.is_paralyzed()) {
            msg_print(_("体が麻痺してしまった！", "You are paralyzed!"));
            reset_concentration(player, true);

            SpellHex spell_hex(this->creature);
            if (spell_hex.is_spelling_any()) {
                (void)spell_hex.stop_all_spells();
            }

            player.counter = false;
            notice = true;
        }
    } else {
        if (paralysis.is_paralyzed()) {
            msg_print(_("やっと動けるようになった。", "You can move again."));
            notice = true;
        }
    }

    paralysis.set(v);
    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(player, false, false);
    }

    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::ACTION);
    handle_stuff(player);
    return true;
}

bool BadStatusSetter::mod_paralysis(const TIME_EFFECT tmp_v)
{
    auto &player = static_cast<PlayerType &>(this->creature);
    return this->set_paralysis(player.effects()->paralysis().current() + tmp_v);
}

/*!
 * @brief 幻覚の継続時間をセットする / Set "image", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 * @details Note that we must redraw the map when hallucination changes.
 */
bool BadStatusSetter::hallucination(const TIME_EFFECT tmp_v)
{
    auto &player = static_cast<PlayerType &>(this->creature);
    auto notice = false;
    auto v = std::clamp<short>(tmp_v, 0, 10000);
    if (player.is_dead()) {
        return false;
    }

    if (player.is_chargeman()) {
        v = 0;
    }

    auto &hallucination = player.effects()->hallucination();
    if (v > 0) {
        set_tsuyoshi(player, 0, true);
        if (!hallucination.is_hallucinated()) {
            msg_print(_("ワーオ！何もかも虹色に見える！", "Oh, wow! Everything looks so cosmic now!"));
            reset_concentration(player, true);

            player.counter = false;
            notice = true;
        }
    } else {
        if (hallucination.is_hallucinated()) {
            msg_print(_("やっとはっきりと物が見えるようになった。", "You can see clearly again."));
            notice = true;
        }
    }

    hallucination.set(v);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(player, false, true);
    }

    static constexpr auto flags_mwrf = {
        MainWindowRedrawingFlag::MAP,
        MainWindowRedrawingFlag::HEALTH,
        MainWindowRedrawingFlag::UHEALTH,
    };
    rfu.set_flags(flags_mwrf);
    rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::OVERHEAD,
        SubWindowRedrawingFlag::DUNGEON,
    };
    rfu.set_flags(flags_swrf);
    handle_stuff(player);
    return true;
}

bool BadStatusSetter::mod_hallucination(const TIME_EFFECT tmp_v)
{
    auto &player = static_cast<PlayerType &>(this->creature);
    return this->hallucination(player.effects()->hallucination().current() + tmp_v);
}

/*!
 * @brief 減速の継続時間をセットする / Set "slow", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool BadStatusSetter::set_deceleration(const TIME_EFFECT tmp_v, bool do_dec)
{
    auto &player = static_cast<PlayerType &>(this->creature);
    auto notice = false;
    auto v = std::clamp<short>(tmp_v, 0, 10000);
    if (player.is_dead()) {
        return false;
    }

    auto &deceleration = player.effects()->deceleration();
    auto is_slow = deceleration.is_slow();
    if (v > 0) {
        if (is_slow && !do_dec) {
            if (deceleration.current() > v) {
                return false;
            }
        } else if (!is_slow) {
            msg_print(_("体の動きが遅くなってしまった！", "You feel yourself moving slower!"));
            notice = true;
        }
    } else {
        if (is_slow) {
            msg_print(_("動きの遅さがなくなったようだ。", "You feel yourself speed up."));
            notice = true;
        }
    }

    deceleration.set(v);
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(player, false, false);
    }

    RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(player);
    return true;
}

bool BadStatusSetter::mod_deceleration(const TIME_EFFECT tmp_v, bool do_dec)
{
    auto &player = static_cast<PlayerType &>(this->creature);
    return this->set_deceleration(player.effects()->deceleration().current() + tmp_v, do_dec);
}

/*!
 * @brief 朦朧の継続時間をセットする / Set "stun", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 * @details
 * Note the special code to only notice "range" changes.
 */
bool BadStatusSetter::set_stun(const TIME_EFFECT tmp_v)
{
    auto &player = static_cast<PlayerType &>(this->creature);
    auto v = std::clamp<short>(tmp_v, 0, 10000);
    if (player.is_dead()) {
        return false;
    }

    if (CreatureRace(&player).has_stun_immunity() || CreatureClass(player).has_stun_immunity()) {
        v = 0;
    }

    auto notice = this->process_stun_effect(v);
    player.effects()->stun().set(v);
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(player, false, false);
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    rfu.set_flag(MainWindowRedrawingFlag::STUN);
    handle_stuff(player);
    return true;
}

bool BadStatusSetter::mod_stun(const TIME_EFFECT tmp_v)
{
    auto &player = static_cast<PlayerType &>(this->creature);
    return this->set_stun(player.effects()->stun().current() + tmp_v);
}

/*!
 * @brief 出血の継続時間をセットする / Set "cut", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 * @details
 * Note the special code to only notice "range" changes.
 */
bool BadStatusSetter::set_cut(const TIME_EFFECT tmp_v)
{
    auto &player = static_cast<PlayerType &>(this->creature);
    auto v = std::clamp<short>(tmp_v, 0, 10000);
    if (player.is_dead()) {
        return false;
    }

    if (CreatureRace(&player).has_cut_immunity()) {
        v = 0;
    }

    auto notice = this->process_cut_effect(v);
    player.effects()->cut().set(v);
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(player, false, false);
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    rfu.set_flag(MainWindowRedrawingFlag::CUT);
    handle_stuff(player);
    return true;
}

bool BadStatusSetter::mod_cut(const TIME_EFFECT tmp_v)
{
    auto &player = static_cast<PlayerType &>(this->creature);
    return this->set_cut(player.effects()->cut().current() + tmp_v);
}

bool BadStatusSetter::process_stun_effect(const short v)
{
    auto &player = static_cast<PlayerType &>(this->creature);
    auto old_rank = player.effects()->stun().get_rank();
    auto new_rank = PlayerStun::get_rank(v);
    if (new_rank > old_rank) {
        this->process_stun_status(new_rank, v);
        return true;
    }

    if (new_rank < old_rank) {
        this->clear_head();
        return true;
    }

    return false;
}

void BadStatusSetter::process_stun_status(const PlayerStunRank new_rank, const short v)
{
    auto &player = static_cast<PlayerType &>(this->creature);
    auto stun_mes = PlayerStun::get_stun_mes(new_rank);
    msg_print(stun_mes);
    this->decrease_int_wis(v);
    if (CreatureClass(player).lose_balance()) {
        msg_print(_("型が崩れた。", "You lose your stance."));
    }

    reset_concentration(player, true);

    SpellHex spell_hex(this->creature);
    if (spell_hex.is_spelling_any()) {
        (void)spell_hex.stop_all_spells();
    }
}

void BadStatusSetter::clear_head()
{
    auto &player = static_cast<PlayerType &>(this->creature);
    if (player.is_stunned()) {
        return;
    }

    msg_print(_("やっと朦朧状態から回復した。", "You are no longer stunned."));
    if (disturb_state) {
        disturb(player, false, false);
    }
}

/*!
 * @todo 後で知能と賢さが両方減る確率を減らす.
 */
void BadStatusSetter::decrease_int_wis(const short v)
{
    auto &player = static_cast<PlayerType &>(this->creature);
    if ((v <= randint1(1000)) && !one_in_(16)) {
        return;
    }

    msg_print(_("割れるような頭痛がする。", "A vicious blow hits your head."));
    auto rand = randint0(5);
    switch (rand) {
    case 0:
        if (has_sustain_int(player) == 0) {
            (void)do_dec_stat(player, A_INT);
        }

        if (has_sustain_wis(player) == 0) {
            (void)do_dec_stat(player, A_WIS);
        }

        return;
    case 1:
    case 2:
        if (has_sustain_int(player) == 0) {
            (void)do_dec_stat(player, A_INT);
        }

        return;
    case 3:
    case 4:
        if (has_sustain_wis(player) == 0) {
            (void)do_dec_stat(player, A_WIS);
        }

        return;
    default:
        THROW_EXCEPTION(std::logic_error, "Invalid random number is specified!");
    }
}

bool BadStatusSetter::process_cut_effect(const short v)
{
    auto &player = static_cast<PlayerType &>(this->creature);
    const auto &player_cut = player.effects()->cut();
    auto old_rank = player_cut.get_rank();
    auto new_rank = player_cut.get_rank(v);
    if (new_rank > old_rank) {
        this->decrease_charisma(new_rank, v);
        return true;
    }

    if (new_rank < old_rank) {
        this->stop_blooding(new_rank);
        return true;
    }

    return false;
}

void BadStatusSetter::decrease_charisma(const PlayerCutRank new_rank, const short v)
{
    auto &player = static_cast<PlayerType &>(this->creature);
    auto cut_mes = PlayerCut::get_cut_mes(new_rank);
    msg_print(cut_mes);
    if (v <= randint1(1000) && !one_in_(16)) {
        return;
    }

    if (has_sustain_chr(player)) {
        return;
    }

    msg_print(_("ひどい傷跡が残ってしまった。", "You have been horribly scarred."));
    do_dec_stat(player, A_CHR);
}

void BadStatusSetter::stop_blooding(const PlayerCutRank new_rank)
{
    auto &player = static_cast<PlayerType &>(this->creature);
    if (new_rank >= PlayerCutRank::GRAZING) {
        return;
    }

    auto blood_stop_mes = CreatureRace(&player).equals(PlayerRaceType::ANDROID)
                              ? _("怪我が直った", "leaking fluid")
                              : _("出血が止まった", "bleeding");
    msg_format(_("やっと%s。", "You are no longer %s."), blood_stop_mes);
    if (disturb_state) {
        disturb(player, false, false);
    }
}
