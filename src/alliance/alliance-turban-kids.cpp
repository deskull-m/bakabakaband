#include "alliance/alliance-turban-kids.h"
#include "floor/floor-util.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "spell/summon-types.h"
#include "system/player-type-definition.h"

/**
 * @note ターバンのガキ共は印象値の正負を一切持たない。
 */
int AllianceTurbanKids::calcImpressionPoint([[maybe_unused]] const CreatureEntity &creature) const
{
    return 0;
}

/**
 * @note ターバンのガキ共は無条件に一定確率でプレイヤーを襲う。
 */
void AllianceTurbanKids::panishment(CreatureEntity &creature)
{
    auto *player_ptr = dynamic_cast<PlayerType *>(&creature);
    if (!player_ptr) {
        return;
    }
    if (one_in_(19)) {
        summon_specific(*player_ptr, player_ptr->y, player_ptr->x, 100, SUMMON_TURBAN_KID, PM_AMBUSH);
    }
    return;
}
