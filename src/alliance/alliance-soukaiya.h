#pragma once

#include "alliance/alliance.h"

class AllianceSoukaiya : public Alliance {
public:
    using Alliance::Alliance;
    AllianceSoukaiya() = delete;
    AllianceSoukaiya(const AllianceSoukaiya &) = default;
    AllianceSoukaiya(AllianceSoukaiya &&) = default;
    AllianceSoukaiya &operator=(const AllianceSoukaiya &) = delete;
    AllianceSoukaiya &operator=(AllianceSoukaiya &&) = delete;
    virtual ~AllianceSoukaiya() = default;
    int calcImpressionPoint(const CreatureEntity &creature) const override;
    bool isAnnihilated() override;
    void panishment(CreatureEntity &creature) override;
};
