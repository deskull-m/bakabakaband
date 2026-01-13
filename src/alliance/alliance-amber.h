#pragma once

#include "alliance/alliance.h"
#include <vector>

enum class MonraceId : int16_t;
class PlayerType;

class AllianceAmber : public Alliance {
public:
    using Alliance::Alliance;
    AllianceAmber() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(const CreatureEntity &creature) const override;
    bool isAnnihilated() override;
    std::vector<MonraceId> get_ambush_monsters(CreatureEntity &creature, int impression_point) const override;
    std::string get_ambush_message() const override;
    virtual ~AllianceAmber() = default;
};
