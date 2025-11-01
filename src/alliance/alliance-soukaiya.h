#pragma once

#include "alliance/alliance.h"

class AllianceSoukaiya : public Alliance {
public:
    using Alliance::Alliance;
    virtual ~AllianceSoukaiya() = default;
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    bool isAnnihilated() override;
    void panishment(PlayerType &player_ptr) override;
};
