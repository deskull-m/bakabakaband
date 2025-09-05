#include "alliance/alliance-utumno.h"
#include "alliance/alliance.h"
#include "system/player-type-definition.h"

AllianceUtumno::AllianceUtumno(AllianceType id, std::string tag, std::string name, int64_t base_power)
    : Alliance(id, tag, name, base_power)
{
}

int AllianceUtumno::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 10, 30);
    return impression;
}
