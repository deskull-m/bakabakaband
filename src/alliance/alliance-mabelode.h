#pragma once

#include "alliance/alliance.h"

class AllianceMabelode : public Alliance {
public:
    using Alliance::Alliance;
    AllianceMabelode() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AllianceMabelode() = default;
};
