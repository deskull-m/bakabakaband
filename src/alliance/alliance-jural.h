#include "alliance.h"
#include <vector>

enum class MonraceId : int16_t;

class AllianceJural : public Alliance {
public:
    using Alliance::Alliance;
    AllianceJural() = delete;
    EnumClassFlagGroup<alliance_flags> alliFlags; //!< 陣営特性フラsグ
    int calcImpressionPoint(PlayerType *creature_ptr) const override;
    void panishment(PlayerType &player_ptr) override;
    bool isAnnihilated() override;
    std::vector<MonraceId> get_ambush_monsters(PlayerType *player_ptr, int impression_point) const override;
    std::string get_ambush_message() const override;
    virtual ~AllianceJural() = default;
};
