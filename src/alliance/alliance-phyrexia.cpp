#include "alliance/alliance-phyrexia.h"
#include "system/monrace/monrace-definition.h"
#include "system/player-type-definition.h"
int AlliancePhyrexia::calcImpressionPoint([[maybe_unused]] PlayerType *creature_ptr) const
{
    int impression = 0;
    impression += calcIronmanHostilityPenalty();

    impression += Alliance::calcPlayerPower(*creature_ptr, 15, 21);
    return impression;
}

/*!
 * @brief ファイレクシアは同アライアンス以外の全てのモンスターを敵対対象とする
 * @param monster_other 敵意を抱くか調べる他のモンスターの参照
 * @param monrace モンスター種族情報の参照
 * @return 同アライアンスでなければtrue
 */
bool AlliancePhyrexia::is_hostile_to([[maybe_unused]] const MonsterEntity &monster_other, const MonraceDefinition &monrace) const
{
    // 同アライアンス以外は全て敵
    return monrace.alliance_idx != AllianceType::PHYREXIA;
}
