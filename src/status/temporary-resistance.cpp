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
        if (creature.tim_levitation && !do_dec) {
            if (creature.tim_levitation > v) {
                return false;
            }
        } else if (!creature.tim_levitation) {
            msg_print(_("体が宙に浮き始めた。", "You begin to fly!"));
            notice = true;
        }
    } else {
        if (creature.tim_levitation) {
            msg_print(_("もう宙に浮かべなくなった。", "You stop flying."));
            notice = true;
        }
    }

    creature.tim_levitation = v;
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
        if (creature.ult_res && !do_dec) {
            if (creature.ult_res > v) {
                return false;
            }
        } else if (!creature.ult_res) {
            msg_print(_("あらゆることに対して耐性がついた気がする！", "You feel resistant!"));
            notice = true;
        }
    } else {
        if (creature.ult_res) {
            msg_print(_("あらゆることに対する耐性が薄れた気がする。", "You feel less resistant"));
            notice = true;
        }
    }

    creature.ult_res = v;
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
        if (creature.tim_res_nether && !do_dec) {
            if (creature.tim_res_nether > v) {
                return false;
            }
        } else if (!creature.tim_res_nether) {
            msg_print(_("地獄の力に対して耐性がついた気がする！", "You feel nether-resistant!"));
            notice = true;
        }
    }

    else {
        if (creature.tim_res_nether) {
            msg_print(_("地獄の力に対する耐性が薄れた気がする。", "You feel less nether-resistant"));
            notice = true;
        }
    }

    creature.tim_res_nether = v;
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
        if (creature.tim_res_lite && !do_dec) {
            if (creature.tim_res_lite > v) {
                return false;
            }
        } else if (!creature.tim_res_lite) {
            msg_print(_("閃光の力に対して耐性がついた気がする！", "You feel lite-resistant!"));
            notice = true;
        }
    } else {
        if (creature.tim_res_lite) {
            msg_print(_("閃光の力に対する耐性が薄れた気がする。", "You feel less lite-resistant"));
            notice = true;
        }
    }

    creature.tim_res_lite = v;
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
        if (creature.tim_res_dark && !do_dec) {
            if (creature.tim_res_dark > v) {
                return false;
            }
        } else if (!creature.tim_res_dark) {
            msg_print(_("暗黒の力に対して耐性がついた気がする！", "You feel dark-resistant!"));
            notice = true;
        }
    } else {
        if (creature.tim_res_dark) {
            msg_print(_("暗黒の力に対する耐性が薄れた気がする。", "You feel less dark-resistant"));
            notice = true;
        }
    }

    creature.tim_res_dark = v;
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
        if (creature.tim_res_fear && !do_dec) {
            if (creature.tim_res_fear > v) {
                return false;
            }
        } else if (!creature.tim_res_fear) {
            msg_print(_("恐怖の力に対して耐性がついた気がする！", "You feel fear-resistant!"));
            notice = true;
        }
    } else {
        if (creature.tim_res_fear) {
            msg_print(_("恐怖の力に対する耐性が薄れた気がする。", "You feel less fear-resistant"));
            notice = true;
        }
    }
    creature.tim_res_fear = v;
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
        if (creature.tim_res_time && !do_dec) {
            if (creature.tim_res_time > v) {
                return false;
            }
        } else if (!creature.tim_res_time) {
            msg_print(_("時間逆転の力に対して耐性がついた気がする！", "You feel time-resistant!"));
            notice = true;
        }
    } else {
        if (creature.tim_res_time) {
            msg_print(_("時間逆転の力に対する耐性が薄れた気がする。", "You feel less time-resistant"));
            notice = true;
        }
    }

    creature.tim_res_time = v;
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
        if (creature.tim_imm_dark && !do_dec) {
            if (creature.tim_imm_dark > v) {
                return false;
            }
        } else if (!creature.tim_imm_dark) {
            msg_print(_("暗黒の力に対して完全な耐性がついた気がする！", "You feel dark-immunity!"));
            notice = true;
        }
    } else {
        if (creature.tim_imm_dark) {
            msg_print(_("暗黒の力に対する完全な耐性を喪った気がする。", "You feel lose dark-immunity"));
            notice = true;
        }
    }
    creature.tim_imm_dark = v;
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
