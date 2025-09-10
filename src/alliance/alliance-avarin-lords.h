#pragma once

#include "alliance/alliance.h"

class AllianceAvarinLords : public Alliance {
public:
    using Alliance::Alliance;
    AllianceAvarinLords() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AllianceAvarinLords() = default;
};
