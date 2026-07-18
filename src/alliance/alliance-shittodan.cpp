#include "alliance/alliance-shittodan.h"
#include "alliance/alliance.h"
#include "core/disturbance.h"
#include "effect/effect-characteristics.h"
#include "floor/floor-util.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/one-monster-placer.h"
#include "monster-floor/place-monster-types.h"
#include "spell/summon-types.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

int AllianceShittoDan::calcImpressionPoint([[maybe_unused]] const CreatureEntity &creature) const
{
    return 0;
}

bool AllianceShittoDan::isAnnihilated()
{
    return all_monraces_extinct({ MonraceId::SHITTO_MASK });
}

void AllianceShittoDan::panishment(CreatureEntity &creature)
{
    auto impression = calcImpressionPoint(creature);
    if (isAnnihilated() || impression > -50) {
        return;
    }

    if (!creature.is_player()) {
        return;
    }
    if (one_in_(20)) {
        Pos2D m_pos(creature.get_position());
        m_pos = scatter(*creature.get_floor(), m_pos, 8, PROJECT_NONE);

        const auto m_idx = place_monster_one(creature, m_pos.y, m_pos.x, MonraceId::SHITTO_MASK, PM_ALLOW_GROUP);
        if (m_idx) {
            msg_print(_("「アベックもリア充も死にさらせええ！」しっと団の襲撃だ！",
                "\"Death to couples and people with fulfilling social lives!\" It's an attack by the Shitto Dan!"));
            disturb(creature, true, true);
            for (int k = 0; k < 3; k++) {
                summon_specific(creature, m_pos.y, m_pos.x, 5, SUMMON_ALLIANCE, PM_ALLOW_GROUP, m_idx);
            }
        }
    }
}
