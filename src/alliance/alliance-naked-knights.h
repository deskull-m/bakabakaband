#pragma once

#include "alliance/alliance.h"

class AllianceNakedKnights : public Alliance {
public:
    using Alliance::Alliance;
    AllianceNakedKnights() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AllianceNakedKnights() = default;
};
