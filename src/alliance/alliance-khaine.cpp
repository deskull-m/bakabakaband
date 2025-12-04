#include "alliance/alliance-khaine.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "system/player-type-definition.h"

#include "game-option/birth-options.h"
/*!
 * @brief カインのアライアンス印象値を計算する
 * パトロンリストでのboost_statがA_STRなので、STRベースの印象値計算を行う
 * @param creature_ptr プレイヤーへの参照ポインタ
 * @return 印象値
 */
int AllianceKhaine::calcImpressionPoint(PlayerType *creature_ptr) const
{
    // bias = 15, level = 30 (Khaineは戦闘と殺戮の神なので、攻撃的な傾向)
    int bias = 15;
    int level = 30;

    // STRベースでパトロンの傾向を反映した印象値計算
    int impression = creature_ptr->stat_max[A_STR] + bias + creature_ptr->lev / level;
    if (impression < 1) {
        impression = 1;
    }

    return impression;
}

/*!
 * @brief カインのアライアンス懲罰処理
 * @param player_ptr プレイヤーへの参照
 */
void AllianceKhaine::panishment([[maybe_unused]] PlayerType &player_ptr)
{
    // カインの懲罰処理 - 戦闘の神なので体力を減らす
    // 基本的な懲罰処理を実装
}

/*!
 * @brief カインのアライアンスが壊滅したかどうか判定する
 * @return 壊滅フラグ
 */
bool AllianceKhaine::isAnnihilated()
{
    // カインは戦闘と殺戮の神なので、簡単には壊滅しない
    return false;
}
