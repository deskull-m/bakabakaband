#pragma once

#include "alliance/alliance.h"

class AlliancePureMirrodin : public Alliance {
public:
    using Alliance::Alliance;
    AlliancePureMirrodin() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AlliancePureMirrodin() = default;
};
