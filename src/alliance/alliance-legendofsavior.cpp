#include "alliance/alliance-legendofsavior.h"
#include "alliance/alliance.h"
#include "core/disturbance.h"
#include "effect/effect-characteristics.h"
#include "floor/floor-util.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/one-monster-placer.h"
#include "monster-floor/place-monster-types.h"
#include "spell/summon-types.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

int AllianceLegendOfSavior::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    auto impression = 0;
    if (MonraceList::get_instance().get_monrace(MonraceId::MISUMI).mob_num == 0) {
        impression = -10000;
    }
    return impression;
}

bool AllianceLegendOfSavior::isAnnihilated()
{
    return MonraceList::get_instance().get_monrace(MonraceId::KENSHIROU).mob_num == 0;
}

void AllianceLegendOfSavior::panishment(PlayerType &player_ptr)
{
    auto impression = calcImpressionPoint(&player_ptr);
    if (isAnnihilated() || impression > -100) {
        return;
    }

    if (one_in_(30)) {
        Pos2D m_pos(player_ptr.get_position());
        m_pos = scatter(&player_ptr, m_pos, 6, PROJECT_NONE);
        if (summon_named_creature(&player_ptr, 0, m_pos.y, m_pos.x, MonraceId::KENSHIROU, 0)) {
            msg_print(_("「てめえに今日を生きる資格はねえ！」", "You don't deserve to live today!"));
            msg_print(_("ケンシロウはあなたがミスミ老人を殺したことに義憤を覚えて襲ってきた！", "Kenshiro attacked you because you killed old man Misumi!"));
        }
    }

    return;
}
