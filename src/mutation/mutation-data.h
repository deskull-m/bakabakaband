#pragma once

#include "mutation/mutation-flag-types.h"
#include <string>

struct MutationData {
    PlayerMutationType type;
    std::string gain_msg_jp;
    std::string gain_msg_en;
    int probability_weight; /*!< 変異確率の重み (1-10, 高いほど出やすい) */
};

class MutationDataManager {
public:
    static const MutationData &get_mutation_data(PlayerMutationType type);
    static PlayerMutationType get_random_mutation(int class_modifier = 0);

private:
    static const MutationData mutation_table[];
};