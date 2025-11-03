#include "alliance/alliance-yeek-kingdom.h"
#include "floor/floor-util.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "spell/summon-types.h"
#include "system/player-type-definition.h"

/**
 * @brief イークの王国の印象値を計算する
 * @param creature_ptr プレイヤーへの参照ポインタ
 * @return 印象値
 * @note イークを多く倒しているほど印象が悪くなる
 */
int AllianceYeekKingdom::calcImpressionPoint(PlayerType *creature_ptr) const
{
    int impression = 0;

    // イーク種族を倒した数に応じて印象が悪化
    const std::string yeek_kill_key = "KILL/RACE/YEEK";
    if (creature_ptr->incident_tree.count(yeek_kill_key)) {
        impression -= creature_ptr->incident_tree[yeek_kill_key] * 10;
    }

    // イークの王国アライアンスを倒した数に応じて印象が悪化
    const std::string alliance_kill_key = "KILL_ALLIANCE/YEEK_KINGDOM";
    if (creature_ptr->incident_tree.count(alliance_kill_key)) {
        impression -= creature_ptr->incident_tree[alliance_kill_key] * 20;
    }

    return impression;
}

/**
 * @brief イークの王国の制裁処理
 * @param player_ptr プレイヤーへの参照
 * @note 印象が悪いほど襲撃される確率が高まる
 */
void AllianceYeekKingdom::panishment([[maybe_unused]] PlayerType &player_ptr)
{
    /*
    int impression = this->calcImpressionPoint(&player_ptr);
    // 印象が-100以下で襲撃開始
    if (impression <= -100) {
        int attack_chance = (-impression - 100) / 50 + 1;
        if (one_in_(std::max(1, 20 - attack_chance))) {
            summon_specific(&player_ptr, player_ptr.y, player_ptr.x, player_ptr.current_floor_ptr->dun_level + 10, SUMMON_YEEK, PM_AMBUSH);
        }
    }
    */
    return;
}
