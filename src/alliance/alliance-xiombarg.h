#pragma once

#include "alliance/alliance.h"

class AllianceXiombarg : public Alliance {
public:
    using Alliance::Alliance;
    AllianceXiombarg() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AllianceXiombarg() = default;
};
