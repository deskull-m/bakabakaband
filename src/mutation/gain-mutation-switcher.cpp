#include "mutation/gain-mutation-switcher.h"
#include "mutation/mutation-data.h"
#include "mutation/mutation-flag-types.h"
#include "mutation/mutation-util.h"
#include "player-base/player-class.h"
#include "system/creature-entity.h"
#include "system/player-type-definition.h"

void switch_gain_mutation(CreatureEntity &creature, glm_type *glm_ptr)
{
    auto &player = static_cast<PlayerType &>(creature);
    PlayerClass pc(&player);

    // 変異の決定
    PlayerMutationType mutation_type;

    if (glm_ptr->choose_mut) {
        // デバッグ等で変異が指定されている場合
        mutation_type = static_cast<PlayerMutationType>(glm_ptr->choose_mut);
    } else if (pc.equals(PlayerClassType::BERSERKER)) {
        // バーサーカーは有害な変異を受けやすい
        mutation_type = MutationDataManager::get_random_mutation(2);
    } else {
        // 通常のランダム変異
        mutation_type = MutationDataManager::get_random_mutation(0);
    }

    // 変異データの取得と設定
    const MutationData &data = MutationDataManager::get_mutation_data(mutation_type);
    glm_ptr->muta_which = data.type;
    glm_ptr->muta_desc = _(data.gain_msg_jp.c_str(), data.gain_msg_en.c_str());

    // 特殊処理
    switch (mutation_type) {
    case PlayerMutationType::CHAOS_GIFT:
        // カオス・ウォリアーは既にカオスの加護を持っているため、このミュテーションは無効
        if (pc.equals(PlayerClassType::CHAOS_WARRIOR)) {
            glm_ptr->muta_which = PlayerMutationType::MAX;
        }
        break;

    case PlayerMutationType::BAD_LUCK:
        // 幸運な性格の場合、このミュテーションは無効
        if (player.ppersonality == PERSONALITY_LUCKY) {
            glm_ptr->muta_which = PlayerMutationType::MAX;
        }
        break;

    case PlayerMutationType::ATT_PERVERT:
        // メスガキ性格の場合、このミュテーションは無効
        if (player.ppersonality == PERSONALITY_MESUGAKI) {
            glm_ptr->muta_which = PlayerMutationType::MAX;
        }
        break;

    default:
        // 特別な処理が不要なミュテーション
        break;
    }
}
