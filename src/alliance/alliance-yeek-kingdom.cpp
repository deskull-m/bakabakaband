#include "alliance/alliance-yeek-kingdom.h"
#include "floor/floor-util.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "spell/summon-types.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"

/**
 * @brief イークの王国の印象値を計算する
 * @param creature クリーチャーへの参照
 * @return 印象値
 * @note イークを多く倒しているほど印象が悪くなる
 * @note ボルドールやオルファックスを殺害した場合は大幅なペナルティ
 */
int AllianceYeekKingdom::calcImpressionPoint([[maybe_unused]] const CreatureEntity &creature) const
{
    int impression = 0;
    impression += calcIronmanHostilityPenalty();

    // ボルドールとオルファックスの殺害状況を確認
    const auto &monrace_list = MonraceList::get_instance();
    const auto boldor_kills = monrace_list.get_monrace(MonraceId::BOLDOR).r_akills;
    const auto orfax_kills = monrace_list.get_monrace(MonraceId::ORFAX).r_akills;

    // ボルドールまたはオルファックスを殺害した場合、大幅に印象値を下げる
    if (boldor_kills > 0) {
        impression -= 1000; // ボルドール殺害による大幅なペナルティ
    }
    if (orfax_kills > 0) {
        impression -= 1000; // オルファックス殺害による大幅なペナルティ
    }

    /*
    // イーク種族を倒した数に応じて印象が悪化
    const std::string yeek_kill_key = "KILL/RACE/YEEK";
    if (creature.incident_tree.count(yeek_kill_key)) {
        impression -= creature.incident_tree[yeek_kill_key] * 10;
    }

    // イークの王国アライアンスを倒した数に応じて印象が悪化
    const std::string alliance_kill_key = "KILL_ALLIANCE/YEEK_KINGDOM";
    if (creature.incident_tree.count(alliance_kill_key)) {
        impression -= creature.incident_tree[alliance_kill_key] * 20;
    }
    */

    return impression;
}

/**
 * @brief イークの王国が壊滅しているかを判定する
 * @return ボルドールとオルファックスの両方が死亡している場合にtrue
 */
bool AllianceYeekKingdom::isAnnihilated()
{
    // ボルドールとオルファックスの両方が死亡した場合、同盟は壊滅
    const auto &monrace_list = MonraceList::get_instance();
    const auto boldor_dead = monrace_list.get_monrace(MonraceId::BOLDOR).r_akills > 0;
    const auto orfax_dead = monrace_list.get_monrace(MonraceId::ORFAX).r_akills > 0;

    return boldor_dead && orfax_dead;
}

/**
 * @brief イークの王国の制裁処理
 * @param creature クリーチャーへの参照
 * @note 印象が悪いほど襲撃される確率が高まる
 */
void AllianceYeekKingdom::panishment([[maybe_unused]] CreatureEntity &creature)
{
    /*
    int impression = this->calcImpressionPoint(creature);
    // 印象が-100以下で襲撃開始
    if (impression <= -100) {
        int attack_chance = (-impression - 100) / 50 + 1;
        if (one_in_(std::max(1, 20 - attack_chance))) {
            summon_specific(&creature, creature.y, creature.x, creature.get_floor()->dun_level + 10, SUMMON_YEEK, PM_AMBUSH);
        }
    }
    */
    return;
}
