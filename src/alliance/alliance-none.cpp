#include "alliance/alliance-none.h"
#include "system/player-type-definition.h"

int AllianceNone::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    return 0;
}
