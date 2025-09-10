#pragma once

#include "alliance/alliance.h"

class AllianceHakushinKarate : public Alliance {
public:
    using Alliance::Alliance;
    AllianceHakushinKarate() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AllianceHakushinKarate() = default;
};
