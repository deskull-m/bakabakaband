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
#include "system/creature-entity.h"
#include "system/creature-timed-effect-types.h"
#include "system/redrawing-flags-updater.h"
#include "timed-effect/player-cut.h"
#include "timed-effect/player-stun.h"
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
    auto notice = false;
    auto v = std::clamp<short>(tmp_v, 0, 10000);
    if (this->creature.is_dead()) {
        return false;
    }

    CreatureRace pr(&this->creature);
    const auto is_blind = this->creature.is_blind();
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

    this->creature.set_timed_effect(CreatureTimedEffect::BLINDNESS, v);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(this->creature, false, false);
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
    handle_stuff(this->creature);
    return true;
}

bool BadStatusSetter::mod_blindness(const TIME_EFFECT tmp_v)
{
    return this->set_blindness(this->creature.get_timed_effect(CreatureTimedEffect::BLINDNESS) + tmp_v);
}

/*!
 * @brief 混乱の継続時間をセットする / Set "confused", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool BadStatusSetter::set_confusion(const TIME_EFFECT tmp_v)
{
    auto notice = false;
    auto v = std::clamp<short>(tmp_v, 0, 10000);
    if (this->creature.is_dead()) {
        return false;
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    const auto is_confused = this->creature.is_confused();
    if (v > 0) {
        if (!is_confused) {
            msg_print(_("あなたは混乱した！", "You are confused!"));
            if (this->creature.action == ACTION_LEARN) {
                msg_print(_("学習が続けられない！", "You cannot continue learning!"));
                auto bluemage_data = CreatureClass(this->creature).get_specific_data<bluemage_data_type>();
                bluemage_data->new_magic_learned = false;
                rfu.set_flag(MainWindowRedrawingFlag::ACTION);
                this->creature.action = ACTION_NONE;
            }
            if (this->creature.action == ACTION_MONK_STANCE) {
                msg_print(_("構えがとけた。", "You lose your stance."));
                CreatureClass(this->creature).set_monk_stance(MonkStanceType::NONE);
                rfu.set_flag(StatusRecalculatingFlag::BONUS);
                rfu.set_flag(MainWindowRedrawingFlag::ACTION);
                this->creature.action = ACTION_NONE;
            } else if (this->creature.action == ACTION_SAMURAI_STANCE) {
                msg_print(_("型が崩れた。", "You lose your stance."));
                CreatureClass(this->creature).lose_balance();
            }

            /* Sniper */
            reset_concentration(this->creature, true);

            SpellHex spell_hex(this->creature);
            if (spell_hex.is_spelling_any()) {
                (void)spell_hex.stop_all_spells();
            }

            notice = true;
            this->creature.counter = false;
            chg_virtue(this->creature, Virtue::HARMONY, -1);
        }
    } else {
        if (is_confused) {
            msg_print(_("やっと混乱がおさまった。", "You feel less confused now."));
            this->creature.special_attack &= ~(ATTACK_SUIKEN);
            notice = true;
        }
    }

    this->creature.set_timed_effect(CreatureTimedEffect::CONFUSION, v);
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(this->creature, false, false);
    }

    handle_stuff(this->creature);
    return true;
}

bool BadStatusSetter::mod_confusion(const TIME_EFFECT tmp_v)
{
    return this->set_confusion(this->creature.get_timed_effect(CreatureTimedEffect::CONFUSION) + tmp_v);
}

/*!
 * @brief 毒の継続時間をセットする / Set "poisoned", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool BadStatusSetter::set_poison(const TIME_EFFECT tmp_v)
{
    auto notice = false;
    auto v = std::clamp<short>(tmp_v, 0, 10000);
    if (this->creature.is_dead()) {
        return false;
    }

    const auto is_poisoned = this->creature.is_poisoned();
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

    this->creature.set_timed_effect(CreatureTimedEffect::POISON, v);
    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(this->creature, false, false);
    }

    handle_stuff(this->creature);
    return true;
}

bool BadStatusSetter::mod_poison(const TIME_EFFECT tmp_v)
{
    return this->set_poison(this->creature.get_timed_effect(CreatureTimedEffect::POISON) + tmp_v);
}

/*!
 * @brief 恐怖の継続時間をセットする / Set "fearful", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool BadStatusSetter::set_fear(const TIME_EFFECT tmp_v)
{
    auto notice = false;
    auto v = std::clamp<short>(tmp_v, 0, 10000);
    if (this->creature.is_dead()) {
        return false;
    }

    const auto is_fearful = this->creature.is_fearful();
    if (v > 0) {
        if (!is_fearful) {
            msg_print(_("何もかも恐くなってきた！", "You are terrified!"));
            if (CreatureClass(this->creature).lose_balance()) {
                msg_print(_("型が崩れた。", "You lose your stance."));
            }

            notice = true;
            this->creature.counter = false;
            chg_virtue(this->creature, Virtue::VALOUR, -1);
        }
    } else {
        if (is_fearful) {
            msg_print(_("やっと恐怖を振り払った。", "You feel bolder now."));
            notice = true;
        }
    }

    this->creature.set_timed_effect(CreatureTimedEffect::FEAR, v);
    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(this->creature, false, false);
    }

    handle_stuff(this->creature);
    return true;
}

bool BadStatusSetter::mod_fear(const TIME_EFFECT tmp_v)
{
    return this->set_fear(this->creature.get_timed_effect(CreatureTimedEffect::FEAR) + tmp_v);
}

/*!
 * @brief 麻痺の継続時間をセットする / Set "paralyzed", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool BadStatusSetter::set_paralysis(const TIME_EFFECT tmp_v)
{
    auto notice = false;
    auto v = std::clamp<short>(tmp_v, 0, 10000);
    if (this->creature.is_dead()) {
        return false;
    }

    const auto is_paralyzed = this->creature.is_paralyzed();
    if (v > 0) {
        if (!is_paralyzed) {
            msg_print(_("体が麻痺してしまった！", "You are paralyzed!"));
            reset_concentration(this->creature, true);

            SpellHex spell_hex(this->creature);
            if (spell_hex.is_spelling_any()) {
                (void)spell_hex.stop_all_spells();
            }

            this->creature.counter = false;
            notice = true;
        }
    } else {
        if (is_paralyzed) {
            msg_print(_("やっと動けるようになった。", "You can move again."));
            notice = true;
        }
    }

    this->creature.set_timed_effect(CreatureTimedEffect::PARALYSIS, v);
    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(this->creature, false, false);
    }

    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::ACTION);
    handle_stuff(this->creature);
    return true;
}

bool BadStatusSetter::mod_paralysis(const TIME_EFFECT tmp_v)
{
    return this->set_paralysis(this->creature.get_timed_effect(CreatureTimedEffect::PARALYSIS) + tmp_v);
}

/*!
 * @brief 幻覚の継続時間をセットする / Set "image", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 * @details Note that we must redraw the map when hallucination changes.
 */
bool BadStatusSetter::hallucination(const TIME_EFFECT tmp_v)
{
    auto notice = false;
    auto v = std::clamp<short>(tmp_v, 0, 10000);
    if (this->creature.is_dead()) {
        return false;
    }

    if (this->creature.is_chargeman()) {
        v = 0;
    }

    const auto is_hallucinated = this->creature.is_hallucinated();
    if (v > 0) {
        set_tsuyoshi(this->creature, 0, true);
        if (!is_hallucinated) {
            msg_print(_("ワーオ！何もかも虹色に見える！", "Oh, wow! Everything looks so cosmic now!"));
            reset_concentration(this->creature, true);

            this->creature.counter = false;
            notice = true;
        }
    } else {
        if (is_hallucinated) {
            msg_print(_("やっとはっきりと物が見えるようになった。", "You can see clearly again."));
            notice = true;
        }
    }

    this->creature.set_timed_effect(CreatureTimedEffect::HALLUCINATION, v);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(this->creature, false, true);
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
    handle_stuff(this->creature);
    return true;
}

bool BadStatusSetter::mod_hallucination(const TIME_EFFECT tmp_v)
{
    return this->hallucination(this->creature.get_timed_effect(CreatureTimedEffect::HALLUCINATION) + tmp_v);
}

/*!
 * @brief 減速の継続時間をセットする / Set "slow", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool BadStatusSetter::set_deceleration(const TIME_EFFECT tmp_v, bool do_dec)
{
    auto notice = false;
    auto v = std::clamp<short>(tmp_v, 0, 10000);
    if (this->creature.is_dead()) {
        return false;
    }

    const auto is_slow = this->creature.is_decelerated();
    if (v > 0) {
        if (is_slow && !do_dec) {
            if (this->creature.get_timed_effect(CreatureTimedEffect::DECELERATION) > v) {
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

    this->creature.set_timed_effect(CreatureTimedEffect::DECELERATION, v);
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(this->creature, false, false);
    }

    RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(this->creature);
    return true;
}

bool BadStatusSetter::mod_deceleration(const TIME_EFFECT tmp_v, bool do_dec)
{
    return this->set_deceleration(this->creature.get_timed_effect(CreatureTimedEffect::DECELERATION) + tmp_v, do_dec);
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
    auto v = std::clamp<short>(tmp_v, 0, 10000);
    if (this->creature.is_dead()) {
        return false;
    }

    if (CreatureRace(&this->creature).has_stun_immunity() || CreatureClass(this->creature).has_stun_immunity()) {
        v = 0;
    }

    auto notice = this->process_stun_effect(v);
    this->creature.set_timed_effect(CreatureTimedEffect::STUN, v);
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(this->creature, false, false);
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    rfu.set_flag(MainWindowRedrawingFlag::STUN);
    handle_stuff(this->creature);
    return true;
}

bool BadStatusSetter::mod_stun(const TIME_EFFECT tmp_v)
{
    return this->set_stun(this->creature.get_timed_effect(CreatureTimedEffect::STUN) + tmp_v);
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
    auto v = std::clamp<short>(tmp_v, 0, 10000);
    if (this->creature.is_dead()) {
        return false;
    }

    if (CreatureRace(&this->creature).has_cut_immunity()) {
        v = 0;
    }

    auto notice = this->process_cut_effect(v);
    this->creature.set_timed_effect(CreatureTimedEffect::CUT, v);
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(this->creature, false, false);
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    rfu.set_flag(MainWindowRedrawingFlag::CUT);
    handle_stuff(this->creature);
    return true;
}

bool BadStatusSetter::mod_cut(const TIME_EFFECT tmp_v)
{
    return this->set_cut(this->creature.get_timed_effect(CreatureTimedEffect::CUT) + tmp_v);
}

bool BadStatusSetter::process_stun_effect(const short v)
{
    auto old_rank = PlayerStun::get_rank(this->creature.get_timed_effect(CreatureTimedEffect::STUN));
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
    auto stun_mes = PlayerStun::get_stun_mes(new_rank);
    msg_print(stun_mes);
    this->decrease_int_wis(v);
    if (CreatureClass(this->creature).lose_balance()) {
        msg_print(_("型が崩れた。", "You lose your stance."));
    }

    reset_concentration(this->creature, true);

    SpellHex spell_hex(this->creature);
    if (spell_hex.is_spelling_any()) {
        (void)spell_hex.stop_all_spells();
    }
}

void BadStatusSetter::clear_head()
{
    if (this->creature.is_stunned()) {
        return;
    }

    msg_print(_("やっと朦朧状態から回復した。", "You are no longer stunned."));
    if (disturb_state) {
        disturb(this->creature, false, false);
    }
}

/*!
 * @todo 後で知能と賢さが両方減る確率を減らす.
 */
void BadStatusSetter::decrease_int_wis(const short v)
{
    if ((v <= randint1(1000)) && !one_in_(16)) {
        return;
    }

    msg_print(_("割れるような頭痛がする。", "A vicious blow hits your head."));
    auto rand = randint0(5);
    switch (rand) {
    case 0:
        if (has_sustain_int(this->creature) == 0) {
            (void)do_dec_stat(this->creature, A_INT);
        }

        if (has_sustain_wis(this->creature) == 0) {
            (void)do_dec_stat(this->creature, A_WIS);
        }

        return;
    case 1:
    case 2:
        if (has_sustain_int(this->creature) == 0) {
            (void)do_dec_stat(this->creature, A_INT);
        }

        return;
    case 3:
    case 4:
        if (has_sustain_wis(this->creature) == 0) {
            (void)do_dec_stat(this->creature, A_WIS);
        }

        return;
    default:
        THROW_EXCEPTION(std::logic_error, "Invalid random number is specified!");
    }
}

bool BadStatusSetter::process_cut_effect(const short v)
{
    auto old_rank = PlayerCut::get_rank(this->creature.get_timed_effect(CreatureTimedEffect::CUT));
    auto new_rank = PlayerCut::get_rank(v);
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
    auto cut_mes = PlayerCut::get_cut_mes(new_rank);
    msg_print(cut_mes);
    if (v <= randint1(1000) && !one_in_(16)) {
        return;
    }

    if (has_sustain_chr(this->creature)) {
        return;
    }

    msg_print(_("ひどい傷跡が残ってしまった。", "You have been horribly scarred."));
    do_dec_stat(this->creature, A_CHR);
}

void BadStatusSetter::stop_blooding(const PlayerCutRank new_rank)
{
    if (new_rank >= PlayerCutRank::GRAZING) {
        return;
    }

    auto blood_stop_mes = CreatureRace(&this->creature).equals(PlayerRaceType::ANDROID)
                              ? _("怪我が直った", "leaking fluid")
                              : _("出血が止まった", "bleeding");
    msg_format(_("やっと%s。", "You are no longer %s."), blood_stop_mes);
    if (disturb_state) {
        disturb(this->creature, false, false);
    }
}
