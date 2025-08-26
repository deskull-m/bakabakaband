#pragma once

#include "alliance/alliance.h"

class AllianceAryanFamily : public Alliance {
public:
    using Alliance::Alliance;
    AllianceAryanFamily() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AllianceAryanFamily() = default;
};
