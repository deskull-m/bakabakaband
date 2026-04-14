#include "status/temporary-resistance.h"
#include "action/travel-execution.h"
#include "core/disturbance.h"
#include "core/stuff-handler.h"
#include "game-option/disturbance-options.h"
#include "system/creature-entity.h"
#include "system/redrawing-flags-updater.h"
#include "view/display-messages.h"

/*!
 * @brief 一時的浮遊の継続時間をセットする / Set "tim_levitation", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_levitation(CreatureEntity &creature, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (creature.is_dead()) {
        return false;
    }

    if (v) {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_LEVITATION) && !do_dec) {
            if (creature.get_timed_effect(CreatureTimedEffect::TIM_LEVITATION) > v) {
                return false;
            }
        } else if (!creature.get_timed_effect(CreatureTimedEffect::TIM_LEVITATION)) {
            msg_print(_("体が宙に浮き始めた。", "You begin to fly!"));
            notice = true;
        }
    } else {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_LEVITATION)) {
            msg_print(_("もう宙に浮かべなくなった。", "You stop flying."));
            notice = true;
        }
    }

    creature.set_timed_effect(CreatureTimedEffect::TIM_LEVITATION, v);
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

bool set_ultimate_res(CreatureEntity &creature, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (creature.is_dead()) {
        return false;
    }

    if (v) {
        if (creature.get_timed_effect(CreatureTimedEffect::ULTIMATE_RESISTANCE) && !do_dec) {
            if (creature.get_timed_effect(CreatureTimedEffect::ULTIMATE_RESISTANCE) > v) {
                return false;
            }
        } else if (!creature.get_timed_effect(CreatureTimedEffect::ULTIMATE_RESISTANCE)) {
            msg_print(_("あらゆることに対して耐性がついた気がする！", "You feel resistant!"));
            notice = true;
        }
    } else {
        if (creature.get_timed_effect(CreatureTimedEffect::ULTIMATE_RESISTANCE)) {
            msg_print(_("あらゆることに対する耐性が薄れた気がする。", "You feel less resistant"));
            notice = true;
        }
    }

    creature.set_timed_effect(CreatureTimedEffect::ULTIMATE_RESISTANCE, v);
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

bool set_tim_res_nether(CreatureEntity &creature, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (creature.is_dead()) {
        return false;
    }

    if (v) {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_RES_NETHER) && !do_dec) {
            if (creature.get_timed_effect(CreatureTimedEffect::TIM_RES_NETHER) > v) {
                return false;
            }
        } else if (!creature.get_timed_effect(CreatureTimedEffect::TIM_RES_NETHER)) {
            msg_print(_("地獄の力に対して耐性がついた気がする！", "You feel nether-resistant!"));
            notice = true;
        }
    }

    else {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_RES_NETHER)) {
            msg_print(_("地獄の力に対する耐性が薄れた気がする。", "You feel less nether-resistant"));
            notice = true;
        }
    }

    creature.set_timed_effect(CreatureTimedEffect::TIM_RES_NETHER, v);
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

bool set_tim_res_lite(CreatureEntity &creature, TIME_EFFECT v, bool do_dec)
{
    auto notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;
    if (creature.is_dead()) {
        return false;
    }

    if (v) {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_RES_LITE) && !do_dec) {
            if (creature.get_timed_effect(CreatureTimedEffect::TIM_RES_LITE) > v) {
                return false;
            }
        } else if (!creature.get_timed_effect(CreatureTimedEffect::TIM_RES_LITE)) {
            msg_print(_("閃光の力に対して耐性がついた気がする！", "You feel lite-resistant!"));
            notice = true;
        }
    } else {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_RES_LITE)) {
            msg_print(_("閃光の力に対する耐性が薄れた気がする。", "You feel less lite-resistant"));
            notice = true;
        }
    }

    creature.set_timed_effect(CreatureTimedEffect::TIM_RES_LITE, v);
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

bool set_tim_res_dark(CreatureEntity &creature, TIME_EFFECT v, bool do_dec)
{
    auto notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;
    if (creature.is_dead()) {
        return false;
    }

    if (v) {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_RES_DARK) && !do_dec) {
            if (creature.get_timed_effect(CreatureTimedEffect::TIM_RES_DARK) > v) {
                return false;
            }
        } else if (!creature.get_timed_effect(CreatureTimedEffect::TIM_RES_DARK)) {
            msg_print(_("暗黒の力に対して耐性がついた気がする！", "You feel dark-resistant!"));
            notice = true;
        }
    } else {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_RES_DARK)) {
            msg_print(_("暗黒の力に対する耐性が薄れた気がする。", "You feel less dark-resistant"));
            notice = true;
        }
    }

    creature.set_timed_effect(CreatureTimedEffect::TIM_RES_DARK, v);
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

bool set_tim_res_fear(CreatureEntity &creature, TIME_EFFECT v, bool do_dec)
{
    auto notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;
    if (creature.is_dead()) {
        return false;
    }
    if (v) {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_RES_FEAR) && !do_dec) {
            if (creature.get_timed_effect(CreatureTimedEffect::TIM_RES_FEAR) > v) {
                return false;
            }
        } else if (!creature.get_timed_effect(CreatureTimedEffect::TIM_RES_FEAR)) {
            msg_print(_("恐怖の力に対して耐性がついた気がする！", "You feel fear-resistant!"));
            notice = true;
        }
    } else {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_RES_FEAR)) {
            msg_print(_("恐怖の力に対する耐性が薄れた気がする。", "You feel less fear-resistant"));
            notice = true;
        }
    }
    creature.set_timed_effect(CreatureTimedEffect::TIM_RES_FEAR, v);
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

bool set_tim_res_time(CreatureEntity &creature, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;
    if (creature.is_dead()) {
        return false;
    }

    if (v) {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_RES_TIME) && !do_dec) {
            if (creature.get_timed_effect(CreatureTimedEffect::TIM_RES_TIME) > v) {
                return false;
            }
        } else if (!creature.get_timed_effect(CreatureTimedEffect::TIM_RES_TIME)) {
            msg_print(_("時間逆転の力に対して耐性がついた気がする！", "You feel time-resistant!"));
            notice = true;
        }
    } else {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_RES_TIME)) {
            msg_print(_("時間逆転の力に対する耐性が薄れた気がする。", "You feel less time-resistant"));
            notice = true;
        }
    }

    creature.set_timed_effect(CreatureTimedEffect::TIM_RES_TIME, v);
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

bool set_tim_imm_dark(CreatureEntity &creature, TIME_EFFECT v, bool do_dec)
{
    auto notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;
    if (creature.is_dead()) {
        return false;
    }
    if (v) {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_IMM_DARK) && !do_dec) {
            if (creature.get_timed_effect(CreatureTimedEffect::TIM_IMM_DARK) > v) {
                return false;
            }
        } else if (!creature.get_timed_effect(CreatureTimedEffect::TIM_IMM_DARK)) {
            msg_print(_("暗黒の力に対して完全な耐性がついた気がする！", "You feel dark-immunity!"));
            notice = true;
        }
    } else {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_IMM_DARK)) {
            msg_print(_("暗黒の力に対する完全な耐性を喪った気がする。", "You feel lose dark-immunity"));
            notice = true;
        }
    }
    creature.set_timed_effect(CreatureTimedEffect::TIM_IMM_DARK, v);
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
