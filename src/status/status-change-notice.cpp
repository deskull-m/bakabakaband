#include "status/status-change-notice.h"
#include "action/travel-execution.h"
#include "core/disturbance.h"
#include "core/stuff-handler.h"
#include "game-option/disturbance-options.h"
#include "system/creature-entity.h"
#include "system/deferred-update-requests.h"

bool notice_bonus_status_change(CreatureEntity &creature, bool notice)
{
    auto &dur = DeferredUpdateRequests::get_instance();
    dur.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    dur.set_flag(StatusRecalculatingFlag::BONUS);
    finish_timed_effect_change(creature);
    return true;
}
