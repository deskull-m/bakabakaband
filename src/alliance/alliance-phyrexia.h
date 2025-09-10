#pragma once

#include "alliance/alliance.h"

class AlliancePhyrexia : public Alliance {
public:
    using Alliance::Alliance;
    AlliancePhyrexia() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AlliancePhyrexia() = default;
};
