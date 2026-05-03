#pragma once

#include "system/angband.h"

/*
 * Player regeneration constants
 */
#define PY_REGEN_NORMAL 197 /* Regen factor*2^16 when full */
#define PY_REGEN_WEAK 98 /* Regen factor*2^16 when weak */
#define PY_REGEN_FAINT 33 /* Regen factor*2^16 when fainting */
#define PY_REGEN_HPBASE 1442 /* Min amount hp regen*2^16 */
#define PY_REGEN_MNBASE 524 /* Min amount mana regen*2^16 */

extern int wild_regen;

class CreatureEntity;

/*!
 * @brief 共通の自然回復量 (regen_amount) を算出する。
 * @details
 * プレイヤーの基準計算を CreatureEntity 上に統一したヘルパ。
 * - 食料・スタンス・呪い・ミュータント体質等のプレイヤー固有要因は is_player() ガードで分岐
 * - 再生種族フラグ・毒・切り傷・行動・地形衛生等は両者で共通に効く
 * 戻り値は HP/MP 回復共通の係数 (1/2^16 単位) で、
 * regenhp() / regenmana() / display 用 calculate_*_regen_rate() で同じ値を使う。
 */
int compute_regen_amount(CreatureEntity &creature);

void regenhp(CreatureEntity &creature, int percent);
void regenmana(CreatureEntity &creature, MANA_POINT upkeep_factor, MANA_POINT regen_amount);
void regenmagic(CreatureEntity &creature, int regen_amount);
void regenerate_monsters(CreatureEntity &creature);
void regenerate_captured_monsters(CreatureEntity &creature);
