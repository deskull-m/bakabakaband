#include "alliance/alliance-court-of-chaos.h"
#include "alliance/alliance.h"
#include "system/player-type-definition.h"

AllianceCourtOfChaos::AllianceCourtOfChaos(AllianceType id, std::string tag, std::string name, int64_t base_power)
    : Alliance(id, tag, name, base_power)
{
}

int AllianceCourtOfChaos::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 10, 35);
    return impression;
}
