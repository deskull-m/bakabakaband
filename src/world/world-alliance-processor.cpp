#include "world/world-alliance-processor.h"
#include "alliance/alliance.h"
#include "system/player-type-definition.h"

/*!
 * @brief 全アライアンスの自然回復処理を実行する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details 10ゲームターンごとに呼び出され、各アライアンスのbase_powerをnatural_recovery分回復させる
 */
void process_alliance_recovery(PlayerType *player_ptr)
{
    (void)player_ptr; // 現在は未使用だが、将来的に使用する可能性があるため引数として保持

    // 全アライアンスの自然回復を処理
    for (const auto &[alliance_type, alliance_ptr] : alliance_list) {
        if (alliance_type == AllianceType::NONE) {
            continue; // 無所属は回復しない
        }

        // 自然回復量が設定されている場合のみ回復
        if (alliance_ptr->natural_recovery != 0) {
            alliance_ptr->base_power += alliance_ptr->natural_recovery;

            // base_powerが負の値にならないように制限
            if (alliance_ptr->base_power < 0) {
                alliance_ptr->base_power = 0;
            }
        }
    }
}
