#include "alliance.h"

class AllianceOdio : public Alliance {
public:
    using Alliance::Alliance;
    AllianceOdio() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    virtual ~AllianceOdio() = default;
};
