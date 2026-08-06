#pragma once

/*!
 * @brief クリーチャー（プレイヤー・モンスター共通）の時限効果の種別
 * @details CreatureEntity::timed_effects の共通ストレージ、および PlayerType の TimedEffects
 * オブジェクト (STUN / CONFUSION / 等) と対応する。適用のないクリーチャーでは
 * get_timed_effect() が 0 を返し、set_timed_effect() は該当スロットに 0 を書き込むのみとなる。
 */
enum class CreatureTimedEffect {
    // --- モンスター・プレイヤー共通 ---
    STUN, /*!< 朦朧 / Stun */
    CONFUSION, /*!< 混乱 / Confusion */
    FEAR, /*!< 恐怖 / Fear */
    INVULNERABILITY, /*!< 無敵 / Invulnerability */
    ACCELERATION, /*!< 加速 / Acceleration (Fast) */
    DECELERATION, /*!< 減速 / Deceleration (Slow) */
    SLEEP_OR_PARALYSIS, /*!< 眠り・麻痺 / Sleep or Paralysis */

    // --- プレイヤー専用：状態異常系 ---
    BLINDNESS, /*!< 盲目 / Blindness */
    PARALYSIS, /*!< 麻痺 / Paralysis */
    HALLUCINATION, /*!< 幻覚 / Hallucination */
    CUT, /*!< 切り傷 / Cut */
    POISON, /*!< 毒 / Poison */
    PROTECTION, /*!< 対邪悪結界 / Protection from Evil */

    // --- プレイヤー専用：戦闘バフ系 ---
    HERO, /*!< 士気高揚 / Heroism */
    BERSERK, /*!< 狂戦士化 / Super Heroism (berserk) */
    SHIELD, /*!< 魔法の盾 / Shield Spell */
    BLESSED, /*!< 祝福 / Blessed */
    MAGICDEF, /*!< 魔法防御 / Magic Defense */
    ULTIMATE_RESISTANCE, /*!< 究極の耐性 / Ultimate Resistance */
    TSUYOSHI, /*!< 剛キャラ / Tsuyoshi Special */

    // --- プレイヤー専用：元素耐性系 ---
    OPPOSE_ACID, /*!< 酸耐性 / Oppose Acid */
    OPPOSE_ELEC, /*!< 電撃耐性 / Oppose Lightning */
    OPPOSE_FIRE, /*!< 火炎耐性 / Oppose Fire */
    OPPOSE_COLD, /*!< 冷気耐性 / Oppose Cold */
    OPPOSE_POIS, /*!< 毒耐性 / Oppose Poison */

    // --- プレイヤー専用：高級耐性系 ---
    RESIST_MAGIC, /*!< 魔法耐性 / Resist Magic */
    TIM_RES_NETHER, /*!< 地獄耐性 / Resist Nether */
    TIM_RES_LITE, /*!< 閃光耐性 / Resist Lite */
    TIM_RES_DARK, /*!< 暗黒耐性 / Resist Dark */
    TIM_RES_FEAR, /*!< 恐怖耐性 / Resist Fear */
    TIM_RES_TIME, /*!< 時間耐性 / Resist Time */
    TIM_IMM_DARK, /*!< 暗黒免疫 / Darkness Immunity */

    // --- プレイヤー専用：知覚系 ---
    TIM_ESP, /*!< テレパシー / Timed ESP */
    TIM_INVIS, /*!< 透明視 / See Invisible */
    TIM_INFRA, /*!< 赤外線視力 / Infravision */
    TIM_STEALTH, /*!< 隠密 / Stealth */

    // --- プレイヤー専用：身体強化系 ---
    TIM_REGEN, /*!< 回復力強化 / Regeneration */
    TIM_PASS_WALL, /*!< 壁抜け / Pass Wall */
    TIM_LEVITATION, /*!< 浮遊 / Levitation */
    TIM_REFLECT, /*!< 反射 / Reflect */
    LIGHTSPEED, /*!< 光速移動 / Light Speed */
    TSUBURERU, /*!< 圧殺 / Tsubureru (crushing) */

    // --- プレイヤー専用：オーラ系 ---
    TIM_SH_TOUKI, /*!< 闘気オーラ / Touki Aura */
    TIM_SH_FIRE, /*!< 火炎オーラ / Fire Aura */
    TIM_SH_HOLY, /*!< 聖なるオーラ / Holy Aura */
    TIM_EYEEYE, /*!< 目には目を / Eye for an Eye */

    // --- プレイヤー専用：形態変化系 ---
    TIM_MIMIC, /*!< 変身 / Mimic Duration */
    WRAITH_FORM, /*!< 幽体化 / Wraith Form */
    MULTISHADOW, /*!< 分身 / Multi-shadow */
    DUSTROBE, /*!< 塵のローブ / Dust Robe */

    // --- プレイヤー専用：元素攻撃系 ---
    ELE_ATTACK, /*!< 元素攻撃 / Elemental Attack */
    ELE_IMMUNE, /*!< 元素免疫 / Elemental Immune */

    // --- プレイヤー専用：聖戦系 ---
    TIM_EMISSION, /*!< 放射 / Player Emission */
    TIM_EXORCISM, /*!< 退魔 / Exorcism */

    // --- プレイヤー専用：世界操作系 ---
    WORD_RECALL, /*!< 帰還のカウントダウン / Word of Recall counter */
    ALTER_REALITY, /*!< 現実改変のカウントダウン / Alter Reality counter */

    MAX,
};
