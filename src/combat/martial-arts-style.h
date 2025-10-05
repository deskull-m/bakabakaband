#pragma once

#include "system/angband.h"

/**
 * @brief 武術流派の種類
 */
enum class MartialArtsStyleType {
    TRADITIONAL = 0, // 伝統派（現在のma_blows）
    STREET_FIGHTING = 1, // ストリートファイト
    KARATE = 2, // 空手
    KUNG_FU = 3, // カンフー
    MAX
};

constexpr int MAX_MARTIAL_ARTS_STYLES = static_cast<int>(MartialArtsStyleType::MAX);

// 流派名の取得
const char *get_martial_arts_style_name(MartialArtsStyleType style);

// 流派の説明の取得
const char *get_martial_arts_style_desc(MartialArtsStyleType style);