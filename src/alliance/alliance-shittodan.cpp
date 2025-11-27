#include "alliance/alliance-shittodan.h"
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

#include "game-option/birth-options.h"
int AllianceShittoDan::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    return 0;
}

bool AllianceShittoDan::isAnnihilated()
{
    return MonraceList::get_instance().get_monrace(MonraceId::SHITTO_MASK).mob_num == 0;
}

void AllianceShittoDan::panishment(PlayerType &player_ptr)
{
    auto impression = calcImpressionPoint(&player_ptr);
    if (isAnnihilated() || impression > -50) {
        return;
    }

    if (one_in_(20)) {
        Pos2D m_pos(player_ptr.get_position());
        m_pos = scatter(&player_ptr, m_pos, 8, PROJECT_NONE);

        const auto m_idx = place_monster_one(&player_ptr, m_pos.y, m_pos.x, MonraceId::SHITTO_MASK, PM_ALLOW_GROUP);
        if (m_idx) {
            msg_print(_("「アベックもリア充も死にさらせええ！」しっと団の襲撃だ！",
                "\"Death to couples and people with fulfilling social lives!\" It's an attack by the Shitto Dan!"));
            disturb(&player_ptr, true, true);
            for (int k = 0; k < 3; k++) {
                summon_specific(&player_ptr, m_pos.y, m_pos.x, std::max(player_ptr.current_floor_ptr->monster_level, 5), SUMMON_ALLIANCE, PM_ALLOW_GROUP, m_idx);
            }
        }
    }
}