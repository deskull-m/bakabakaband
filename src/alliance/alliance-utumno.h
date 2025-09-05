#pragma once

#include "alliance/alliance.h"

class PlayerType;

class AllianceUtumno : public Alliance {
public:
    AllianceUtumno(AllianceType id, std::string tag, std::string name, int64_t base_power);
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
};
