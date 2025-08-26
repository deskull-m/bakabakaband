#include "alliance/alliance-jural.h"
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

int AllianceJural::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 2, 40);
    impression -= MonraceList::get_instance().get_monrace(MonraceId::ALIEN_JURAL).r_akills * 10;
    if (MonraceList::get_instance().get_monrace(MonraceId::JURAL_MONS).mob_num == 0) {
        impression -= 300;
    }
    if (MonraceList::get_instance().get_monrace(MonraceId::JURAL_WITCHKING).mob_num == 0) {
        impression -= 1230;
    }
    return impression;
}

bool AllianceJural::isAnnihilated()
{
    return MonraceList::get_instance().get_monrace(MonraceId::JURAL_WITCHKING).mob_num == 0;
}

void AllianceJural::panishment(PlayerType &player_ptr)
{
    auto impression = calcImpressionPoint(&player_ptr);
    if (isAnnihilated() || impression > -40) {
        return;
    }

    if (one_in_(20)) {
        Pos2D m_pos(player_ptr.get_position());
        m_pos = scatter(&player_ptr, m_pos, 12, PROJECT_NONE);
        const auto m_idx = place_monster_one(&player_ptr, m_pos.y, m_pos.x, MonraceId::ALIEN_JURAL, PM_ALLOW_GROUP | PM_JURAL);
        if (m_idx) {
            msg_print(_("「おーい、行ってみよう！」ジュラル星人があなたに報復すべく追跡してきた！", "\"Hey, let's go!\" Alien Jurals is chasing you for revenge!"));
            disturb(&player_ptr, true, true);
            for (int k = 0; k < 4; k++) {
                summon_specific(&player_ptr, m_pos.y, m_pos.x, std::max(player_ptr.current_floor_ptr->monster_level, 5), SUMMON_ALLIANCE, PM_ALLOW_GROUP, m_idx);
            }
        }
    }

    return;
}
