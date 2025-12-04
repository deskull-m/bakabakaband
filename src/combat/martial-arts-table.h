#pragma once

#include "combat/martial-arts-style.h"
#include "system/angband.h"
#include "util/dice.h"

#define MAX_MA 17 /*!< 修行僧マーシャルアーツの技数 / Monk martial arts... */
#define MA_KNEE 1 /*!< 金的効果ID */
#define MA_SLOW 2 /*!< 膝蹴り効果ID */

/* For Monk martial arts */
struct martial_arts {
    concptr desc; /* A verbose attack description */
    PLAYER_LEVEL min_level; /* Minimum level to use */
    int chance; /* Chance of 'success' */
    Dice damage_dice; /* Damage dice */
    int effect; /* Special effects */
};

extern const martial_arts ma_blows[MAX_MA];
extern const martial_arts ma_street_fighting[MAX_MA];
extern const martial_arts ma_karate[MAX_MA];
extern const martial_arts ma_kung_fu[MAX_MA];

// 流派別武術テーブル
extern const martial_arts *get_martial_arts_table(MartialArtsStyleType style);

// 特定の流派のテーブルから技を取得
const martial_arts *get_martial_arts_technique(MartialArtsStyleType style, int level);
