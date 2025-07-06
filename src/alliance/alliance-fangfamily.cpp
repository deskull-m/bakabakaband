#include "alliance/alliance-fangfamily.h"
#include "alliance/alliance.h"
#include "core/disturbance.h"
#include "effect/effect-characteristics.h"
#include "floor/floor-util.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/one-monster-placer.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/race-flags1.h"
#include "spell/summon-types.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

int AllianceFangFamily::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 5, 10);
    impression -= MonraceList::get_instance().get_monrace(MonraceId::FANG_FAMILY).r_akills * 5;
    if (MonraceList::get_instance().get_monrace(MonraceId::KING_FANG_FAMILY).mob_num == 0) {
        impression -= 300;
    }
    return impression;
}

bool AllianceFangFamily::isAnnihilated()
{
    return MonraceList::get_instance().get_monrace(MonraceId::KING_FANG_FAMILY).mob_num == 0;
}

void AllianceFangFamily::panishment(PlayerType &player_ptr)
{
    auto impression = calcImpressionPoint(&player_ptr);
    if (isAnnihilated() || impression > -30) {
        return;
    }

    if (one_in_(25)) {
        Pos2D m_pos(player_ptr.get_position());
        m_pos = scatter(&player_ptr, m_pos, 10, PROJECT_NONE);
        MonraceId avenger_id;
        if (impression < -200) {
            avenger_id = MonraceId::KING_FANG_FAMILY;
        } else {
            avenger_id = MonraceId::FANG_FAMILY;
        }
        const auto m_idx = place_monster_one(&player_ptr, m_pos.y, m_pos.x, avenger_id, PM_ALLOW_GROUP);
        if (m_idx) {
            if (avenger_id == MonraceId::KING_FANG_FAMILY) {
                msg_print(_("「一族の復讐を受けよ！」族長があなたを始末しに現れた！",
                    "\"Feel the revenge of Fang Family!\" The chief appears to eliminate you!"));
            } else {
                msg_print(_("「華山群狼拳！」牙一族があなたに報復してきた！",
                    "\"For the honor of Fang Family!\" A Fang family member is chasing you for revenge!"));
            }

            disturb(&player_ptr, true, true);
            for (int k = 0; k < 4; k++) {
                summon_specific(&player_ptr, m_pos.y, m_pos.x, std::max(player_ptr.current_floor_ptr->monster_level, 5), SUMMON_ALLIANCE, PM_ALLOW_GROUP, m_idx);
            }
        }
    }
}
