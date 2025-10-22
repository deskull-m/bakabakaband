#include "alliance/alliance-utumno.h"
#include "alliance/alliance.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"

AllianceUtumno::AllianceUtumno(AllianceType id, std::string tag, std::string name, int64_t base_power)
    : Alliance(id, tag, name, base_power)
{
}

int AllianceUtumno::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(*creature_ptr, 10, 30);
    return impression;
}

/*!
 * @brief ウトゥムノのアライアンスが壊滅したかどうか判定する
 * @return 壊滅フラグ
 * @details 冥王『メルコール』が存在しない場合に壊滅する
 */
bool AllianceUtumno::isAnnihilated()
{
    // 冥王『メルコール』が存在しない場合、ウトゥムノは壊滅する
    return MonraceList::get_instance().get_monrace(MonraceId::MORGOTH).mob_num == 0;
}
