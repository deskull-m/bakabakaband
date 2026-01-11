#pragma once

#include "alliance/alliance.h"

class AlliancePureMirrodin : public Alliance {
public:
    using Alliance::Alliance;
    AlliancePureMirrodin() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(const CreatureEntity &creature) const override;
    virtual ~AlliancePureMirrodin() = default;
};
