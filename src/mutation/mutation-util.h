#pragma once

#include "system/angband.h"
#include <tl/optional.hpp>

enum class PlayerMutationType;

struct glm_type {
    PlayerMutationType muta_which;
    concptr muta_desc;
    bool muta_chosen;

    /*!
     * @brief 指定された突然変異のID。tl::nullopt でランダム選択
     * @details 獲得側 (switch_gain_mutation) では PlayerMutationType の enum 値、
     * 喪失側 (switch_lose_mutation) では重み付き抽選表の case 番号を意味する
     * (両者の番号体系は一致しない)。公開 API の gain_mutation() / lose_mutation()
     * は PlayerMutationType で受け取り、喪失側はその内部で case 番号へ変換する。
     */
    tl::optional<int> choose_mut;
};

glm_type *initialize_glm_type(glm_type *gm_ptr, tl::optional<int> choose_mut);
