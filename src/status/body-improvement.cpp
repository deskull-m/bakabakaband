#include "status/body-improvement.h"
#include "action/travel-execution.h"
#include "avatar/avatar.h"
#include "core/disturbance.h"
#include "core/speed-table.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "game-option/disturbance-options.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "realm/realm-song-numbers.h"
#include "spell-realm/spells-song.h"
#include "system/creature-entity.h"
#include "system/creature-timed-effect-types.h"
#include "system/redrawing-flags-updater.h"
#include "view/display-messages.h"

BodyImprovement::BodyImprovement(CreatureEntity &creature)
    : creature_ptr(&creature)
{
}

bool BodyImprovement::has_effect() const
{
    return this->is_affected;
}

void BodyImprovement::mod_protection(short v, bool is_decrease)
{
    this->set_protection(this->creature_ptr->get_timed_effect(CreatureTimedEffect::PROTECTION) + v, is_decrease);
}

/*!
 * @brief 対邪悪結界の継続時間をセットする
 * @param v 継続時間
 * @param is_decrease ターン経過による減少か魔力消去の場合のみtrue
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
void BodyImprovement::set_protection(short v, bool is_decrease)
{
    this->is_affected = false;
    auto notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;
    if (this->creature_ptr->is_dead()) {
        return;
    }

    const auto protection = this->creature_ptr->get_timed_effect(CreatureTimedEffect::PROTECTION);
    const auto is_protected = protection > 0;
    if (v) {
        if (is_protected && !is_decrease) {
            if (protection > v) {
                return;
            }
        } else if (!is_protected) {
            msg_print(_("邪悪なる存在から守られているような感じがする！", "You feel safe from evil!"));
            notice = true;
        }
    } else {
        if (is_protected) {
            msg_print(_("邪悪なる存在から守られている感じがなくなった。", "You no longer feel safe from evil."));
            sound(SoundKind::BUFF_EXPIRE);
            notice = true;
        }
    }

    this->creature_ptr->set_timed_effect(CreatureTimedEffect::PROTECTION, v);
    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return;
    }

    if (disturb_state || Travel::get_instance().is_ongoing()) {
        disturb(*this->creature_ptr, false, true);
    }

    handle_stuff(*this->creature_ptr);
    this->is_affected = true;
}

/*!
 * @brief 無傷球の継続時間をセットする / Set "invuln", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_invuln(CreatureEntity &creature, short v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (creature.is_dead()) {
        return false;
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::OVERHEAD,
        SubWindowRedrawingFlag::DUNGEON,
    };
    if (v) {
        if (creature.get_timed_effect(CreatureTimedEffect::INVULNERABILITY) && !do_dec) {
            if (creature.get_timed_effect(CreatureTimedEffect::INVULNERABILITY) > v) {
                return false;
            }
        } else if (!creature.is_invulnerable()) {
            msg_print(_("無敵だ！", "Invulnerability!"));
            notice = true;
            chg_virtue(creature, Virtue::UNLIFE, -2);
            chg_virtue(creature, Virtue::HONOUR, -2);
            chg_virtue(creature, Virtue::SACRIFICE, -3);
            chg_virtue(creature, Virtue::VALOUR, -5);
            rfu.set_flag(MainWindowRedrawingFlag::MAP);
            rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
            rfu.set_flags(flags_swrf);
        }
    } else {
        if (creature.get_timed_effect(CreatureTimedEffect::INVULNERABILITY) && !music_singing(creature, MUSIC_INVULN)) {
            msg_print(_("無敵ではなくなった。", "The invulnerability wears off."));
            sound(SoundKind::BUFF_EXPIRE);
            notice = true;
            rfu.set_flag(MainWindowRedrawingFlag::MAP);
            rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
            rfu.set_flags(flags_swrf);
            creature.energy_need += ENERGY_NEED();
        }
    }

    creature.set_timed_effect(CreatureTimedEffect::INVULNERABILITY, v);
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);

    if (!notice) {
        return false;
    }

    if (disturb_state || Travel::get_instance().is_ongoing()) {
        disturb(creature, false, true);
    }

    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(creature);
    return true;
}

/*!
 * @brief 時限急回復の継続時間をセットする / Set "tim_regen", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_regen(CreatureEntity &creature, short v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (creature.is_dead()) {
        return false;
    }

    if (v) {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_REGEN) && !do_dec) {
            if (creature.get_timed_effect(CreatureTimedEffect::TIM_REGEN) > v) {
                return false;
            }
        } else if (!creature.get_timed_effect(CreatureTimedEffect::TIM_REGEN)) {
            msg_print(_("回復力が上がった！", "You feel yourself regenerating quickly!"));
            notice = true;
        }
    } else {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_REGEN)) {
            msg_print(_("素早く回復する感じがなくなった。", "You feel you are no longer regenerating quickly."));
            sound(SoundKind::BUFF_EXPIRE);
            notice = true;
        }
    }

    creature.set_timed_effect(CreatureTimedEffect::TIM_REGEN, v);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state || Travel::get_instance().is_ongoing()) {
        disturb(creature, false, true);
    }

    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(creature);
    return true;
}

/*!
 * @brief 一時的反射の継続時間をセットする / Set "tim_reflect", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_reflect(CreatureEntity &creature, short v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (creature.is_dead()) {
        return false;
    }

    if (v) {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_REFLECT) && !do_dec) {
            if (creature.get_timed_effect(CreatureTimedEffect::TIM_REFLECT) > v) {
                return false;
            }
        } else if (!creature.get_timed_effect(CreatureTimedEffect::TIM_REFLECT)) {
            msg_print(_("体の表面が滑かになった気がする。", "Your body becames smooth."));
            notice = true;
        }
    } else {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_REFLECT)) {
            msg_print(_("体の表面が滑かでなくなった。", "Your body is no longer smooth."));
            sound(SoundKind::BUFF_EXPIRE);
            notice = true;
        }
    }

    creature.set_timed_effect(CreatureTimedEffect::TIM_REFLECT, v);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state || Travel::get_instance().is_ongoing()) {
        disturb(creature, false, true);
    }

    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(creature);
    return true;
}

/*!
 * @brief 一時的壁抜けの継続時間をセットする / Set "tim_pass_wall", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_pass_wall(CreatureEntity &creature, short v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (creature.is_dead()) {
        return false;
    }

    if (v) {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_PASS_WALL) && !do_dec) {
            if (creature.get_timed_effect(CreatureTimedEffect::TIM_PASS_WALL) > v) {
                return false;
            }
        } else if (!creature.get_timed_effect(CreatureTimedEffect::TIM_PASS_WALL)) {
            msg_print(_("体が半物質の状態になった。", "You became ethereal."));
            notice = true;
        }
    } else {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_PASS_WALL)) {
            msg_print(_("体が物質化した。", "You are no longer ethereal."));
            sound(SoundKind::BUFF_EXPIRE);
            notice = true;
        }
    }

    creature.set_timed_effect(CreatureTimedEffect::TIM_PASS_WALL, v);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state || Travel::get_instance().is_ongoing()) {
        disturb(creature, false, true);
    }

    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(creature);
    return true;
}

/*!
 * @brief 一時的光源強化の継続時間をセットする / Set "tim_emission", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_emission(CreatureEntity &creature, short v, bool do_dec)
{
    auto notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (creature.is_dead()) {
        return false;
    }

    if (v) {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_EMISSION) && !do_dec) {
            if (creature.get_timed_effect(CreatureTimedEffect::TIM_EMISSION) > v) {
                return false;
            }
        } else if (!creature.get_timed_effect(CreatureTimedEffect::TIM_EMISSION)) {
            msg_print(_("体が発光した。", "Your body emit light."));
            notice = true;
        }
    } else {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_EMISSION)) {
            msg_print(_("体の光が消え去った。", "Your body stopped emitting light."));
            sound(SoundKind::BUFF_EXPIRE);
            notice = true;
        }
    }

    creature.set_timed_effect(CreatureTimedEffect::TIM_EMISSION, v);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state || Travel::get_instance().is_ongoing()) {
        disturb(creature, false, true);
    }

    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    rfu.set_flag(StatusRecalculatingFlag::TORCH);
    handle_stuff(creature);
    return true;
}
/*!
 * @brief 一時的悪魔祓いの継続時間をセットする / Set "tim_exorcism", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_exorcism(CreatureEntity &creature, short v, bool do_dec)
{
    auto notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (creature.is_dead()) {
        return false;
    }

    if (v) {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_EXORCISM) && !do_dec) {
            if (creature.get_timed_effect(CreatureTimedEffect::TIM_EXORCISM) > v) {
                return false;
            }
        } else if (!creature.get_timed_effect(CreatureTimedEffect::TIM_EXORCISM)) {
            msg_print(_("浄化の力を得た気がする。", "You feel you become an exorcist."));
            notice = true;
        }
    } else {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_EXORCISM)) {
            msg_print(_("浄化の力を失った。", "You are no longer exorcist."));
            sound(SoundKind::BUFF_EXPIRE);
            notice = true;
        }
    }

    creature.set_timed_effect(CreatureTimedEffect::TIM_EXORCISM, v);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state || Travel::get_instance().is_ongoing()) {
        disturb(creature, false, true);
    }

    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(creature);
    return true;
}
