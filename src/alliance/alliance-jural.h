#include "alliance/alliance.h"
#include <vector>

enum class MonraceId : int16_t;

class AllianceJural : public Alliance {
public:
    using Alliance::Alliance;
    AllianceJural() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラsグ
    int calcImpressionPoint(const CreatureEntity &creature) const override;
    void panishment(CreatureEntity &creature) override;
    bool isAnnihilated() override;
    std::vector<MonraceId> get_ambush_monsters(CreatureEntity &creature, int impression_point) const override;
    std::string get_ambush_message() const override;
    virtual ~AllianceJural() = default;
};
