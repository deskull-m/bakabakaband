#include "alliance/alliance.h"

class AllianceMegadeth : public Alliance {
public:
    using Alliance::Alliance;
    AllianceMegadeth() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラsグ
    int calcImpressionPoint(const CreatureEntity &creature) const override;
    bool isAnnihilated() override;
    virtual ~AllianceMegadeth() = default;
};
