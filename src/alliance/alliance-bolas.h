#pragma once

#include "alliance/alliance.h"
#include <vector>

enum class MonraceId : int16_t;
class MonraceDefinition;

class AllianceBolas : public Alliance {
public:
    using Alliance::Alliance;
    AllianceBolas() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラグ
    int calcImpressionPoint(const CreatureEntity &creature) const override;
    void panishment(CreatureEntity &creature) override;
    bool isAnnihilated() override;
    bool is_hostile_to(const CreatureEntity &creature_other, const MonraceDefinition &monrace) const override;
    std::vector<MonraceId> get_ambush_monsters(CreatureEntity &creature, int impression_point) const override;
    std::string get_ambush_message() const override;
    virtual ~AllianceBolas() = default;
};
