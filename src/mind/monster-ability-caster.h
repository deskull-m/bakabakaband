/*!
 * @file monster-ability-caster.h
 * @brief モンスター化したプレイヤーが種族固有能力 (ability_flags) を行使する処理
 */

#pragma once

class CreatureEntity;

/*!
 * @brief モンスター化プレイヤーの固有能力を選択・発動する
 * @param creature クリーチャーへの参照 (PlayerType を想定)
 * @return 行動を消費したら true、キャンセルした場合 false
 * @details 青魔法 (cast_learned_spell) の発動基盤を流用し、プレイヤーの
 *          現在の monrace (get_monrace().ability_flags) から発動可能な
 *          能力を選択して行使する。
 */
bool do_cmd_use_monster_ability(CreatureEntity &creature);

/*!
 * @brief 当該クリーチャーがモンスター固有能力を行使可能か判定する
 * @return r_idx が PLAYER 以外 (= モンスター化) かつ発動可能な ability を
 *         1 つ以上持っていれば true
 */
bool can_use_monster_ability(const CreatureEntity &creature);
