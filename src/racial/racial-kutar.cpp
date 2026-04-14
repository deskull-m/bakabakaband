#include "racial/racial-kutar.h"
#include "action/travel-execution.h"
#include "core/disturbance.h"
#include "core/stuff-handler.h"
#include "game-option/disturbance-options.h"
#include "system/creature-entity.h"
#include "system/redrawing-flags-updater.h"
#include "view/display-messages.h"

/*!
 * @brief つぶれるの継続時間をセットする / Set "tsubureru", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_leveling(CreatureEntity &creature, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (creature.is_dead()) {
        return false;
    }

    if (v) {
        if (creature.get_timed_effect(CreatureTimedEffect::TSUBURERU) && !do_dec) {
            if (creature.get_timed_effect(CreatureTimedEffect::TSUBURERU) > v) {
                return false;
            }
        } else if (!creature.get_timed_effect(CreatureTimedEffect::TSUBURERU)) {
            msg_print(_("横に伸びた。", "Your body expands horizontally."));
            notice = true;
        }
    } else {
        if (creature.get_timed_effect(CreatureTimedEffect::TSUBURERU)) {
            msg_print(_("もう横に伸びていない。", "Your body returns to normal."));
            notice = true;
        }
    }

    creature.set_timed_effect(CreatureTimedEffect::TSUBURERU, v);
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
