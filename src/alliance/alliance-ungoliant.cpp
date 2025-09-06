#include "alliance/alliance-ungoliant.h"
#include "alliance/alliance.h"
#include "system/player-type-definition.h"

AllianceUngoliant::AllianceUngoliant(AllianceType id, std::string tag, std::string name, int64_t base_power)
    : Alliance(id, tag, name, base_power)
{
}

int AllianceUngoliant::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 8, 30);
    return impression;
}
