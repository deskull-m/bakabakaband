#include "status/buff-setter.h"
#include "action/travel-execution.h"
#include "avatar/avatar.h"
#include "core/disturbance.h"
#include "core/speed-table.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "game-option/disturbance-options.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster/monster-status-setter.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "player/attack-defense-types.h"
#include "realm/realm-song-numbers.h"
#include "spell-realm/spells-song.h"
#include "status/base-status.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "system/creature-entity.h"
#include "system/creature-timed-effect-types.h"
#include "system/redrawing-flags-updater.h"
#include "view/display-messages.h"

/*!
 * @brief プレイヤーの全ての時限効果をリセットする。 / reset timed flags
 */
void reset_tim_flags(CreatureEntity &creature)
{
    creature.set_timed_effect(CreatureTimedEffect::ACCELERATION, 0);
    creature.set_timed_effect(CreatureTimedEffect::DECELERATION, 0);
    creature.set_timed_effect(CreatureTimedEffect::BLINDNESS, 0);
    creature.set_timed_effect(CreatureTimedEffect::PARALYSIS, 0);
    creature.set_timed_effect(CreatureTimedEffect::CONFUSION, 0);
    creature.set_timed_effect(CreatureTimedEffect::FEAR, 0);
    creature.set_timed_effect(CreatureTimedEffect::HALLUCINATION, 0);
    creature.set_timed_effect(CreatureTimedEffect::POISON, 0);
    creature.set_timed_effect(CreatureTimedEffect::CUT, 0);
    creature.set_timed_effect(CreatureTimedEffect::STUN, 0);
    creature.set_timed_effect(CreatureTimedEffect::PROTECTION, 0);

    creature.set_timed_effect(CreatureTimedEffect::INVULNERABILITY, 0);
    creature.set_timed_effect(CreatureTimedEffect::ULTIMATE_RESISTANCE, 0);
    creature.set_timed_effect(CreatureTimedEffect::HERO, 0);
    creature.set_timed_effect(CreatureTimedEffect::BERSERK, 0);
    creature.set_timed_effect(CreatureTimedEffect::SHIELD, 0);
    creature.set_timed_effect(CreatureTimedEffect::BLESSED, 0);
    creature.set_timed_effect(CreatureTimedEffect::TIM_INVIS, 0);
    creature.set_timed_effect(CreatureTimedEffect::TIM_INFRA, 0);
    creature.set_timed_effect(CreatureTimedEffect::TIM_REGEN, 0);
    creature.set_timed_effect(CreatureTimedEffect::TIM_STEALTH, 0);
    creature.set_timed_effect(CreatureTimedEffect::TIM_ESP, 0);
    creature.set_timed_effect(CreatureTimedEffect::WRAITH_FORM, 0);
    creature.set_timed_effect(CreatureTimedEffect::TIM_LEVITATION, 0);
    creature.set_timed_effect(CreatureTimedEffect::TIM_SH_TOUKI, 0);
    creature.set_timed_effect(CreatureTimedEffect::TIM_SH_FIRE, 0);
    creature.set_timed_effect(CreatureTimedEffect::TIM_SH_HOLY, 0);
    creature.set_timed_effect(CreatureTimedEffect::TIM_EYEEYE, 0);
    creature.set_timed_effect(CreatureTimedEffect::MAGICDEF, 0);
    creature.set_timed_effect(CreatureTimedEffect::RESIST_MAGIC, 0);
    creature.set_timed_effect(CreatureTimedEffect::TSUYOSHI, 0);
    creature.set_timed_effect(CreatureTimedEffect::TIM_PASS_WALL, 0);
    creature.set_timed_effect(CreatureTimedEffect::TIM_RES_NETHER, 0);
    creature.set_timed_effect(CreatureTimedEffect::TIM_RES_LITE, 0);
    creature.set_timed_effect(CreatureTimedEffect::TIM_RES_DARK, 0);
    creature.set_timed_effect(CreatureTimedEffect::TIM_RES_FEAR, 0);
    creature.set_timed_effect(CreatureTimedEffect::TIM_RES_TIME, 0);
    creature.set_timed_effect(CreatureTimedEffect::TIM_MIMIC, 0);
    creature.set_mimic_form(MimicKindType::NONE);
    creature.set_timed_effect(CreatureTimedEffect::TIM_REFLECT, 0);
    creature.set_timed_effect(CreatureTimedEffect::MULTISHADOW, 0);
    creature.set_timed_effect(CreatureTimedEffect::DUSTROBE, 0);
    creature.set_timed_effect(CreatureTimedEffect::TIM_EMISSION, 0);
    creature.set_timed_effect(CreatureTimedEffect::TIM_EXORCISM, 0);
    creature.set_timed_effect(CreatureTimedEffect::TIM_IMM_DARK, 0);
    creature.set_timed_effect(CreatureTimedEffect::LIGHTSPEED, 0);
    creature.set_timed_effect(CreatureTimedEffect::ELE_ATTACK, 0);
    creature.set_timed_effect(CreatureTimedEffect::ELE_IMMUNE, 0);
    creature.action = ACTION_NONE;

    creature.set_timed_effect(CreatureTimedEffect::OPPOSE_ACID, 0);
    creature.set_timed_effect(CreatureTimedEffect::OPPOSE_ELEC, 0);
    creature.set_timed_effect(CreatureTimedEffect::OPPOSE_FIRE, 0);
    creature.set_timed_effect(CreatureTimedEffect::OPPOSE_COLD, 0);
    creature.set_timed_effect(CreatureTimedEffect::OPPOSE_POIS, 0);

    creature.set_timed_effect(CreatureTimedEffect::WORD_RECALL, 0);
    creature.set_timed_effect(CreatureTimedEffect::ALTER_REALITY, 0);

    // 非 CreatureTimedEffect のリセット
    creature.sutemi = false;
    creature.counter = false;
    creature.special_attack = 0L;
    creature.special_defense = 0L;

    while (creature.get_energy_need() < 0) {
        creature.set_energy_need(creature.get_energy_need() + ENERGY_NEED());
    }

    creature.timewalk = false;

    if (creature.get_riding()) {
        (void)set_monster_fast(*creature.get_floor(), creature.get_riding(), 0);
        (void)set_monster_slow(*creature.get_floor(), creature.get_riding(), 0);
        (void)set_monster_invulner(*creature.get_floor(), creature.get_riding(), 0, false);
    }

    if (CreatureClass(creature).equals(PlayerClassType::BARD)) {
        set_singing_song_effect(creature, MUSIC_NONE);
        set_singing_song_id(creature, 0);
    }
}

/*!
 * @brief 加速の継続時間をセットする / Set "fast", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_acceleration(CreatureEntity &creature, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (creature.is_dead()) {
        return false;
    }

    const auto acceleration = creature.get_timed_effect(CreatureTimedEffect::ACCELERATION);
    const auto is_fast = creature.is_fast();
    if (v) {
        if ((acceleration > 0) && !do_dec) {
            if (acceleration > v) {
                return false;
            }
        } else if (!is_fast && !creature.get_timed_effect(CreatureTimedEffect::LIGHTSPEED)) {
            msg_print(_("素早く動けるようになった！", "You feel yourself moving much faster!"));
            notice = true;
            chg_virtue(creature, Virtue::PATIENCE, -1);
            chg_virtue(creature, Virtue::DILIGENCE, 1);
        }
    } else {
        if ((acceleration > 0) && !creature.get_timed_effect(CreatureTimedEffect::LIGHTSPEED)) {
            auto is_singing = music_singing(creature, MUSIC_SPEED);
            is_singing |= music_singing(creature, MUSIC_SHERO);
            if (!is_singing) {
                msg_print(_("動きの素早さがなくなったようだ。", "You feel yourself slow down."));
                sound(SoundKind::BUFF_EXPIRE);
                notice = true;
            }
        }
    }

    creature.set_timed_effect(CreatureTimedEffect::ACCELERATION, v);
    if (!notice) {
        return false;
    }

    if (disturb_state || Travel::get_instance().is_ongoing()) {
        disturb(creature, false, true);
    }
    RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(creature);
    return true;
}

bool mod_acceleration(CreatureEntity &creature, const TIME_EFFECT v, const bool do_dec)
{
    return set_acceleration(creature, creature.get_timed_effect(CreatureTimedEffect::ACCELERATION) + v, do_dec);
}

/*!
 * @brief 肌石化の継続時間をセットする / Set "shield", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_shield(CreatureEntity &creature, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (creature.is_dead()) {
        return false;
    }

    if (v) {
        if (creature.get_timed_effect(CreatureTimedEffect::SHIELD) && !do_dec) {
            if (creature.get_timed_effect(CreatureTimedEffect::SHIELD) > v) {
                return false;
            }
        } else if (!creature.get_timed_effect(CreatureTimedEffect::SHIELD)) {
            msg_print(_("肌が石になった。", "Your skin turns to stone."));
            notice = true;
        }
    } else {
        if (creature.get_timed_effect(CreatureTimedEffect::SHIELD)) {
            msg_print(_("肌が元に戻った。", "Your skin returns to normal."));
            sound(SoundKind::BUFF_EXPIRE);
            notice = true;
        }
    }

    creature.set_timed_effect(CreatureTimedEffect::SHIELD, v);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state || Travel::get_instance().is_ongoing()) {
        disturb(creature, false, true);
    }

    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(creature);
    return true;
}

/*!
 * @brief 魔法の鎧の継続時間をセットする / Set "magicdef", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_magicdef(CreatureEntity &creature, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (creature.is_dead()) {
        return false;
    }

    if (v) {
        if (creature.get_timed_effect(CreatureTimedEffect::MAGICDEF) && !do_dec) {
            if (creature.get_timed_effect(CreatureTimedEffect::MAGICDEF) > v) {
                return false;
            }
        } else if (!creature.get_timed_effect(CreatureTimedEffect::MAGICDEF)) {
            msg_print(_("魔法の防御力が増したような気がする。", "You feel more resistant to magic."));
            notice = true;
        }
    } else {
        if (creature.get_timed_effect(CreatureTimedEffect::MAGICDEF)) {
            msg_print(_("魔法の防御力が元に戻った。", "You feel less resistant to magic."));
            sound(SoundKind::BUFF_EXPIRE);
            notice = true;
        }
    }

    creature.set_timed_effect(CreatureTimedEffect::MAGICDEF, v);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state || Travel::get_instance().is_ongoing()) {
        disturb(creature, false, true);
    }

    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(creature);
    return true;
}

/*!
 * @brief 祝福の継続時間をセットする / Set "blessed", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_blessed(CreatureEntity &creature, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (creature.is_dead()) {
        return false;
    }

    if (v) {
        if (creature.get_timed_effect(CreatureTimedEffect::BLESSED) && !do_dec) {
            if (creature.get_timed_effect(CreatureTimedEffect::BLESSED) > v) {
                return false;
            }
        } else if (!creature.is_blessed()) {
            msg_print(_("高潔な気分になった！", "You feel righteous!"));
            notice = true;
        }
    } else {
        if (creature.get_timed_effect(CreatureTimedEffect::BLESSED) && !music_singing(creature, MUSIC_BLESS)) {
            msg_print(_("高潔な気分が消え失せた。", "The prayer has expired."));
            sound(SoundKind::BUFF_EXPIRE);
            notice = true;
        }
    }

    creature.set_timed_effect(CreatureTimedEffect::BLESSED, v);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state || Travel::get_instance().is_ongoing()) {
        disturb(creature, false, true);
    }

    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(creature);
    return true;
}

/*!
 * @brief 士気高揚の継続時間をセットする / Set "hero", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_hero(CreatureEntity &creature, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (creature.is_dead()) {
        return false;
    }

    if (v) {
        if (creature.get_timed_effect(CreatureTimedEffect::HERO) && !do_dec) {
            if (creature.get_timed_effect(CreatureTimedEffect::HERO) > v) {
                return false;
            }
        } else if (!creature.is_hero()) {
            msg_print(_("ヒーローになった気がする！", "You feel like a hero!"));
            notice = true;
        }
    } else {
        if (creature.get_timed_effect(CreatureTimedEffect::HERO) && !music_singing(creature, MUSIC_HERO) && !music_singing(creature, MUSIC_SHERO)) {
            msg_print(_("ヒーローの気分が消え失せた。", "The heroism wears off."));
            sound(SoundKind::BUFF_EXPIRE);
            notice = true;
        }
    }

    creature.set_timed_effect(CreatureTimedEffect::HERO, v);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state || Travel::get_instance().is_ongoing()) {
        disturb(creature, false, true);
    }

    static constexpr auto flags = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::HP,
    };
    RedrawingFlagsUpdater::get_instance().set_flags(flags);
    handle_stuff(creature);
    return true;
}

/*!
 * @brief 変身効果の継続時間と変身先をセットする / Set "tim_mimic", and "mimic_form", notice observable changes
 * @param v 継続時間
 * @param mimic_race_idx 変身内容
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_mimic(CreatureEntity &creature, TIME_EFFECT v, MimicKindType mimic_race_idx, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (creature.is_dead()) {
        return false;
    }

    if (v) {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_MIMIC) && (creature.get_mimic_form() == mimic_race_idx) && !do_dec) {
            if (creature.get_timed_effect(CreatureTimedEffect::TIM_MIMIC) > v) {
                return false;
            }
        } else if ((!creature.get_timed_effect(CreatureTimedEffect::TIM_MIMIC)) || (creature.get_mimic_form() != mimic_race_idx)) {
            msg_print(_("自分の体が変わってゆくのを感じた。", "You feel that your body changes."));
            creature.set_mimic_form(mimic_race_idx);
            notice = true;
        }
    }

    else {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_MIMIC)) {
            msg_print(_("変身が解けた。", "You are no longer transformed."));
            sound(SoundKind::BUFF_EXPIRE);
            if (creature.get_mimic_form() == MimicKindType::DEMON) {
                set_oppose_fire(creature, 0, true);
            }
            creature.set_mimic_form(MimicKindType::NONE);
            notice = true;
            mimic_race_idx = MimicKindType::NONE;
        }
    }

    creature.set_timed_effect(CreatureTimedEffect::TIM_MIMIC, v);
    if (!notice) {
        return false;
    }

    if (disturb_state || Travel::get_instance().is_ongoing()) {
        disturb(creature, false, true);
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_mwrf = {
        MainWindowRedrawingFlag::BASIC,
        MainWindowRedrawingFlag::TIMED_EFFECT,
    };
    rfu.set_flags(flags_mwrf);
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::HP,
    };
    rfu.set_flags(flags_srf);
    handle_stuff(creature);
    return true;
}

/*!
 * @brief 狂戦士化の継続時間をセットする / Set "shero", notice observable changes
 * @param v 継続時間/ 0ならば無条件にリセット
 * @param do_dec FALSEの場合現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_berserk(CreatureEntity &creature, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (creature.is_dead()) {
        return false;
    }

    if (CreatureClass(creature).equals(PlayerClassType::BERSERKER)) {
        v = 1;
        return false;
    }

    if (v) {
        if (creature.get_timed_effect(CreatureTimedEffect::BERSERK) && !do_dec) {
            if (creature.get_timed_effect(CreatureTimedEffect::BERSERK) > v) {
                return false;
            }
        } else if (!creature.get_timed_effect(CreatureTimedEffect::BERSERK)) {
            msg_print(_("殺戮マシーンになった気がする！", "You feel like a killing machine!"));
            notice = true;
        }
    } else {
        if (creature.get_timed_effect(CreatureTimedEffect::BERSERK)) {
            sound(SoundKind::BUFF_EXPIRE);
            msg_print(_("野蛮な気持ちが消え失せた。", "You feel less berserk."));
            notice = true;
        }
    }

    creature.set_timed_effect(CreatureTimedEffect::BERSERK, v);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state || Travel::get_instance().is_ongoing()) {
        disturb(creature, false, true);
    }

    static constexpr auto flags = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::HP,
    };
    RedrawingFlagsUpdater::get_instance().set_flags(flags);
    handle_stuff(creature);
    return true;
}

/*!
 * @brief 幽体化の継続時間をセットする / Set "wraith_form", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_wraith_form(CreatureEntity &creature, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (creature.is_dead()) {
        return false;
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::OVERHEAD,
        SubWindowRedrawingFlag::DUNGEON,
    };
    if (v) {
        if (creature.get_timed_effect(CreatureTimedEffect::WRAITH_FORM) && !do_dec) {
            if (creature.get_timed_effect(CreatureTimedEffect::WRAITH_FORM) > v) {
                return false;
            }
        } else if (!creature.get_timed_effect(CreatureTimedEffect::WRAITH_FORM)) {
            msg_print(_("物質界を離れて幽鬼のような存在になった！", "You leave the physical world and turn into a wraith-being!"));
            notice = true;
            chg_virtue(creature, Virtue::UNLIFE, 3);
            chg_virtue(creature, Virtue::HONOUR, -2);
            chg_virtue(creature, Virtue::SACRIFICE, -2);
            chg_virtue(creature, Virtue::VALOUR, -5);
            rfu.set_flag(MainWindowRedrawingFlag::MAP);
            rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
            rfu.set_flags(flags_swrf);
        }
    } else {
        if (creature.get_timed_effect(CreatureTimedEffect::WRAITH_FORM)) {
            msg_print(_("不透明になった感じがする。", "You feel opaque."));
            sound(SoundKind::BUFF_EXPIRE);
            notice = true;
            rfu.set_flag(MainWindowRedrawingFlag::MAP);
            rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
            rfu.set_flags(flags_swrf);
        }
    }

    creature.set_timed_effect(CreatureTimedEffect::WRAITH_FORM, v);
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state || Travel::get_instance().is_ongoing()) {
        disturb(creature, false, true);
    }

    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(creature);
    return true;
}

/*!
 * @brief オクレ兄さんの継続時間をセットする / Set "tsuyoshi", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tsuyoshi(CreatureEntity &creature, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (creature.is_dead()) {
        return false;
    }

    if (v) {
        if (creature.get_timed_effect(CreatureTimedEffect::TSUYOSHI) && !do_dec) {
            if (creature.get_timed_effect(CreatureTimedEffect::TSUYOSHI) > v) {
                return false;
            }
        } else if (!creature.get_timed_effect(CreatureTimedEffect::TSUYOSHI)) {
            msg_print(_("「オクレ兄さん！」", "Brother OKURE!"));
            notice = true;
            chg_virtue(creature, Virtue::VITALITY, 2);
        }
    } else {
        if (creature.get_timed_effect(CreatureTimedEffect::TSUYOSHI)) {
            sound(SoundKind::BUFF_EXPIRE);
            msg_print(_("肉体が急速にしぼんでいった。", "Your body has quickly shriveled."));

            (void)dec_stat(creature, A_CON, 20, true);
            (void)dec_stat(creature, A_STR, 20, true);

            notice = true;
            chg_virtue(creature, Virtue::VITALITY, -3);
        }
    }

    creature.set_timed_effect(CreatureTimedEffect::TSUYOSHI, v);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state || Travel::get_instance().is_ongoing()) {
        disturb(creature, false, true);
    }

    static constexpr auto flags = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::HP,
    };
    rfu.set_flags(flags);
    handle_stuff(creature);
    return true;
}
