#pragma once

/*!
 * @brief クリーチャー（プレイヤー・モンスター共通）の時限効果の種別
 * @details PlayerType の TimedEffects および MonsterEntity の mtimed と対応する
 */
enum class CreatureTimedEffect {
    STUN, /*!< 朦朧 / Stun */
    CONFUSION, /*!< 混乱 / Confusion */
    FEAR, /*!< 恐怖 / Fear */
    INVULNERABILITY, /*!< 無敵 / Invulnerability */
    ACCELERATION, /*!< 加速 / Acceleration (Fast) */
    DECELERATION, /*!< 減速 / Deceleration (Slow) */
    SLEEP_OR_PARALYSIS, /*!< 眠り・麻痺 / Sleep or Paralysis */
    BLINDNESS, /*!< 盲目 / Blindness (プレイヤーのみ、モンスターは常に0) */
    PARALYSIS, /*!< 麻痺 / Paralysis (プレイヤーのみ、モンスターは SLEEP_OR_PARALYSIS で代替) */
    MAX,
};
