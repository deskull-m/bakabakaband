#include "alliance.h"

class AllianceJural : public Alliance {
public:
    using Alliance::Alliance;
    AllianceJural() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラsグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    bool isAnnihilated() override;
    virtual ~AllianceJural() = default;
};
