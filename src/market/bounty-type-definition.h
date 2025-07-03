#pragma once

#include "system/angband.h"

enum class MonraceId : int16_t;

struct bounty_type {
    MonraceId r_idx; //!< 賞金首の対象のモンスター種族ID
    bool is_achieved; //!< 死体を渡して報酬を獲得したかどうか
};
