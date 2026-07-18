#include "alliance/alliance-gaichi.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"

int AllianceGaichi::calcImpressionPoint([[maybe_unused]] const CreatureEntity &creature) const
{
    int impression = 0;
    impression += Alliance::calcPlayerPower(creature, 30, 4);
    impression += calcIronmanHostilityPenalty();
    impression -= MonraceList::get_instance().get_monrace(MonraceId::BIO_CORE).r_pkills * 1500;
    impression -= MonraceList::get_instance().get_monrace(MonraceId::GAICHI_MOA).r_pkills * 1200;
    impression -= MonraceList::get_instance().get_monrace(MonraceId::SS_80_X_ATRAS).r_pkills * 20;
    impression -= MonraceList::get_instance().get_monrace(MonraceId::GX_77_X_CHARLIE).r_pkills * 5;
    impression -= MonraceList::get_instance().get_monrace(MonraceId::GX_87_X_KILROY).r_pkills * 6;
    impression -= MonraceList::get_instance().get_monrace(MonraceId::M_77_X_TRIGGER).r_pkills * 2;
    return impression;
}

/*!
 * @brief ガイチ軍のアライアンスが壊滅したかどうか判定する
 * @return 壊滅フラグ
 */
bool AllianceGaichi::isAnnihilated()
{
    return all_monraces_extinct({ MonraceId::BIO_CORE, MonraceId::GAICHI_MOA });
}
