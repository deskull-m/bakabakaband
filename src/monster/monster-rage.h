#pragma once

#include "system/angband.h"
#include "util/point-2d.h"

class CreatureEntity;

/*!
 * @brief 植木鉢くんへの激怒 (ENRAGED_BY_BROKEN_POT) による常時ボーナス (Issue #180)
 * @details 既存の怒り (ANGER、`aggravate_monsters()` が付ける一時加速) より大きく、
 *          フロアを離れるまで続く。バランス調整はこの 5 定数で行う。
 */
constexpr auto RAGE_SPEED_BONUS = 15;
constexpr auto RAGE_MELEE_HIT_BONUS = 20;
constexpr auto RAGE_MELEE_DAMAGE_BONUS = 10;
constexpr auto RAGE_AC_BONUS = 30;
constexpr auto RAGE_SAVE_BONUS = 10;

/*!
 * @brief 激怒が届く距離 (グリッド)
 * @details 見通しは要求しない (無残な姿が見えなくとも「やった」ことは伝わる)。
 */
constexpr auto RAGE_BROKEN_POT_RADIUS = 12;

/*!
 * @brief 激怒した人型が罵る確率の逆数 (1/N)
 * @details 通常の喋り (`process_speak()` の `chance_speak`) と同じ桁に揃えてある。
 */
constexpr auto RAGE_SPEECH_CHANCE = 8;

/*!
 * @brief 植木鉢くんを壊した罪悪感 (BROKEN_POT_GUILT) の継続時間 (Issue #180)
 * @details `BASE + 1d DICE` ゲームターン分。回復手段は時間経過のみなので、
 *          長すぎると詰みに近付く。バランス調整はこの 2 定数で行う。
 */
constexpr auto BROKEN_POT_GUILT_DURATION_BASE = 200;
constexpr auto BROKEN_POT_GUILT_DURATION_DICE = 200;

/*!
 * @brief 罪悪感による AC 低下量
 * @details `calc_to_ac()` から減算する。表示値 (dis_to_a) にも効く
 *          (後ろめたさは自分で分かるため)。
 */
constexpr auto BROKEN_POT_GUILT_AC_PENALTY = 20;

/*!
 * @brief 罪悪感による被クリティカル率の上昇量
 * @details `SUPERHURT` 打撃のクリティカル判定 `randint1(rlev * 2 + 300) > ac + 200`
 *          のダイス面数に加算する (面数が増えるほど AC の壁を越えやすくなる)。
 */
constexpr auto BROKEN_POT_GUILT_CRITICAL_SIDES = 300;

void enrage_monsters_by_broken_pot(CreatureEntity &creature, MONSTER_IDX pot_m_idx, bool killed_by_player);
bool speak_rage_by_broken_pot(CreatureEntity &creature, MONSTER_IDX m_idx, const Pos2D &pos, bool aware);
