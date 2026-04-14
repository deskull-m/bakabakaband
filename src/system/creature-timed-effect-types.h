#pragma once

/*!
 * @brief クリーチャー（プレイヤー・モンスター共通）の時限効果の種別
 * @details PlayerType の TimedEffects および MonsterProfile の mtimed と対応する
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
    HERO, /*!< 士気高揚 / Heroism (プレイヤーのみ、モンスターは常に0) */
    BERSERK, /*!< 狂戦士化 / Super Heroism / Berserk (プレイヤーのみ、モンスターは常に0) */
    BLESSED, /*!< 祝福 / Blessed (プレイヤーのみ、モンスターは常に0) */
    SHIELD, /*!< 魔法の盾 / Shield spell (プレイヤーのみ、モンスターは常に0) */
    ULTIMATE_RESISTANCE, /*!< 究極耐性 / Ultimate Resistance (プレイヤーのみ、モンスターは常に0) */
    WRAITH_FORM, /*!< 幽体化 / Wraith form (プレイヤーのみ、モンスターは常に0) */
    TIM_ESP, /*!< 一時テレパシー / Timed ESP (プレイヤーのみ、モンスターは常に0) */
    TIM_STEALTH, /*!< 一時隠密 / Timed Stealth (プレイヤーのみ、モンスターは常に0) */
    TIM_REGEN, /*!< 一時再生 / Timed Regeneration (プレイヤーのみ、モンスターは常に0) */
    TSUYOSHI, /*!< つよし特殊 / Tsuyoshi Special (プレイヤーのみ、モンスターは常に0) */
    TIM_INVIS, /*!< 一時透明視 / Timed See Invisible (プレイヤーのみ、モンスターは常に0) */
    TIM_INFRA, /*!< 一時赤外線視 / Timed Infra Vision (プレイヤーのみ、モンスターは常に0) */
    OPPOSE_ACID, /*!< 一時酸耐性 / Timed Oppose Acid (プレイヤーのみ、モンスターは常に0) */
    OPPOSE_ELEC, /*!< 一時電撃耐性 / Timed Oppose Lightning (プレイヤーのみ、モンスターは常に0) */
    OPPOSE_FIRE, /*!< 一時火炎耐性 / Timed Oppose Fire (プレイヤーのみ、モンスターは常に0) */
    OPPOSE_COLD, /*!< 一時冷気耐性 / Timed Oppose Cold (プレイヤーのみ、モンスターは常に0) */
    OPPOSE_POIS, /*!< 一時毒耐性 / Timed Oppose Poison (プレイヤーのみ、モンスターは常に0) */
    MAX,
};
