#include "alliance.h"

class AllianceTophamHatt : public Alliance {
public:
    using Alliance::Alliance;
    AllianceTophamHatt() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラsグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    bool isAnnihilated() override;
    virtual ~AllianceTophamHatt() = default;
};
