#pragma once

#include "mutation/mutation-flag-types.h"
#include <string>

struct MutationData {
    PlayerMutationType type;
    std::string gain_msg_jp;
    std::string gain_msg_en;
    int probability_weight; /*!< 変異確率の重み (1-10, 高いほど出やすい) */
};

/*!
 * @brief 突然変異の短い表示名
 * @details 一覧や選択 UI で使う。獲得時のメッセージ (MutationData) とは別に持つ。
 */
struct MutationName {
    PlayerMutationType type;
    std::string name_jp;
    std::string name_en;
};

class MutationDataManager {
public:
    static const MutationData &get_mutation_data(PlayerMutationType type);
    static const std::string &get_mutation_name(PlayerMutationType type);
    static PlayerMutationType get_random_mutation(int class_modifier = 0);

private:
    static const MutationData mutation_table[];
    static const MutationName mutation_name_table[];
};