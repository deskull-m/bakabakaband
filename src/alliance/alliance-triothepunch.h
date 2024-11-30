#include "alliance.h"

class AllianceTrioThePunch : public Alliance {
public:
    using Alliance::Alliance;
    AllianceTrioThePunch() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラsグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    bool isAnnihilated() override;
    virtual ~AllianceTrioThePunch() = default;
};
