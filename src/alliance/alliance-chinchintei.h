#pragma once

#include "alliance/alliance.h"

class AllianceChinChinTei : public Alliance {
public:
    using Alliance::Alliance;
    AllianceChinChinTei() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    bool isAnnihilated() override;
    virtual ~AllianceChinChinTei() = default;
};