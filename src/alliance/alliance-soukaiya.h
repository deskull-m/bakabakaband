#pragma once

#include "alliance/alliance.h"

class AllianceSoukaiya : public Alliance {
public:
    using Alliance::Alliance;
    virtual ~AllianceSoukaiya() = default;
    int calcImpressionPoint(const CreatureEntity &creature) const override;
    bool isAnnihilated() override;
    void panishment(CreatureEntity &creature) override;
};
