#include "alliance.h"

class AllianceOdio : public Alliance {
public:
    using Alliance::Alliance;
    AllianceOdio() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(const CreatureEntity &creature) const override;
    bool is_hostile_to(const CreatureEntity &creature_other, const MonraceDefinition &monrace) const override;
    virtual ~AllianceOdio() = default;
};
