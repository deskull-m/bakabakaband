#include "status/buff-setter.h"
#include "action/travel-execution.h"
#include "avatar/avatar.h"
#include "core/disturbance.h"
#include "core/speed-table.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "game-option/disturbance-options.h"
#include "monster/monster-status-setter.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "player/attack-defense-types.h"
#include "player/player-status.h"
#include "realm/realm-song-numbers.h"
#include "spell-realm/spells-song.h"
#include "status/base-status.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "system/creature-entity.h"
#include "system/redrawing-flags-updater.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"

/*!
 * @brief プレイヤーの全ての時限効果をリセットする。 / reset timed flags
 */
void reset_tim_flags(CreatureEntity &creature)
{
    auto effects = creature.effects();
    effects->acceleration().reset();
    creature.lightspeed = 0;
    effects->deceleration().reset();
    effects->blindness().reset();
    effects->paralysis().reset();
    effects->confusion().reset();
    effects->fear().reset();
    effects->hallucination().reset();
    effects->poison().reset();
    effects->cut().reset();
    effects->stun().reset();
    effects->protection().reset();

    creature.invuln = 0; /* Timed -- Invulnerable */
    creature.ult_res = 0;
    creature.hero = 0; /* Timed -- Heroism */
    creature.berserk = 0; /* Timed -- Super Heroism */
    creature.shield = 0; /* Timed -- Shield Spell */
    creature.blessed = 0; /* Timed -- Blessed */
    creature.tim_invis = 0; /* Timed -- Invisibility */
    creature.tim_infra = 0; /* Timed -- Infra Vision */
    creature.tim_regen = 0; /* Timed -- Regeneration */
    creature.tim_stealth = 0; /* Timed -- Stealth */
    creature.tim_esp = 0;
    creature.wraith_form = 0; /* Timed -- Wraith Form */
    creature.tim_levitation = 0;
    creature.tim_sh_touki = 0;
    creature.tim_sh_fire = 0;
    creature.tim_sh_holy = 0;
    creature.tim_eyeeye = 0;
    creature.magicdef = 0;
    creature.resist_magic = 0;
    creature.tsuyoshi = 0;
    creature.tim_pass_wall = 0;
    creature.tim_res_nether = 0;
    creature.tim_res_lite = 0;
    creature.tim_res_dark = 0;
    creature.tim_res_fear = 0;
    creature.tim_res_time = 0;
    creature.tim_mimic = 0;
    creature.mimic_form = MimicKindType::NONE;
    creature.tim_reflect = 0;
    creature.multishadow = 0;
    creature.dustrobe = 0;
    creature.tim_emission = 0;
    creature.tim_exorcism = 0;
    creature.tim_imm_dark = 0;
    creature.action = ACTION_NONE;

    creature.oppose_acid = 0; /* Timed -- oppose acid */
    creature.oppose_elec = 0; /* Timed -- oppose lightning */
    creature.oppose_fire = 0; /* Timed -- oppose heat */
    creature.oppose_cold = 0; /* Timed -- oppose cold */
    creature.oppose_pois = 0; /* Timed -- oppose poison */

    creature.word_recall = 0;
    creature.alter_reality = 0;
    creature.sutemi = false;
    creature.counter = false;
    creature.ele_attack = 0;
    creature.ele_immune = 0;
    creature.special_attack = 0L;
    creature.special_defense = 0L;

    while (creature.get_energy_need() < 0) {
        creature.set_energy_need(creature.get_energy_need() + ENERGY_NEED());
    }

    creature.timewalk = false;

    if (creature.riding) {
        (void)set_monster_fast(*creature.current_floor_ptr, creature.riding, 0);
        (void)set_monster_slow(*creature.current_floor_ptr, creature.riding, 0);
        (void)set_monster_invulner(*creature.current_floor_ptr, creature.riding, 0, false);
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

    auto &acceleration = creature.effects()->acceleration();
    if (v) {
        if (acceleration.is_fast() && !do_dec) {
            if (acceleration.current() > v) {
                return false;
            }
        } else if (!creature.is_fast() && !creature.lightspeed) {
            msg_print(_("素早く動けるようになった！", "You feel yourself moving much faster!"));
            notice = true;
            chg_virtue(creature, Virtue::PATIENCE, -1);
            chg_virtue(creature, Virtue::DILIGENCE, 1);
        }
    } else {
        if (acceleration.is_fast() && !creature.lightspeed) {
            auto is_singing = music_singing(creature, MUSIC_SPEED);
            is_singing |= music_singing(creature, MUSIC_SHERO);
            if (!is_singing) {
                msg_print(_("動きの素早さがなくなったようだ。", "You feel yourself slow down."));
                notice = true;
            }
        }
    }

    acceleration.set(v);
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
    return set_acceleration(creature, creature.effects()->acceleration().current() + v, do_dec);
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
        if (creature.shield && !do_dec) {
            if (creature.shield > v) {
                return false;
            }
        } else if (!creature.shield) {
            msg_print(_("肌が石になった。", "Your skin turns to stone."));
            notice = true;
        }
    } else {
        if (creature.shield) {
            msg_print(_("肌が元に戻った。", "Your skin returns to normal."));
            notice = true;
        }
    }

    creature.shield = v;
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
        if (creature.magicdef && !do_dec) {
            if (creature.magicdef > v) {
                return false;
            }
        } else if (!creature.magicdef) {
            msg_print(_("魔法の防御力が増したような気がする。", "You feel more resistant to magic."));
            notice = true;
        }
    } else {
        if (creature.magicdef) {
            msg_print(_("魔法の防御力が元に戻った。", "You feel less resistant to magic."));
            notice = true;
        }
    }

    creature.magicdef = v;
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
        if (creature.blessed && !do_dec) {
            if (creature.blessed > v) {
                return false;
            }
        } else if (!is_blessed(creature)) {
            msg_print(_("高潔な気分になった！", "You feel righteous!"));
            notice = true;
        }
    } else {
        if (creature.blessed && !music_singing(creature, MUSIC_BLESS)) {
            msg_print(_("高潔な気分が消え失せた。", "The prayer has expired."));
            notice = true;
        }
    }

    creature.blessed = v;
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
        if (creature.hero && !do_dec) {
            if (creature.hero > v) {
                return false;
            }
        } else if (!is_hero(creature)) {
            msg_print(_("ヒーローになった気がする！", "You feel like a hero!"));
            notice = true;
        }
    } else {
        if (creature.hero && !music_singing(creature, MUSIC_HERO) && !music_singing(creature, MUSIC_SHERO)) {
            msg_print(_("ヒーローの気分が消え失せた。", "The heroism wears off."));
            notice = true;
        }
    }

    creature.hero = v;
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
        if (creature.tim_mimic && (creature.mimic_form == mimic_race_idx) && !do_dec) {
            if (creature.tim_mimic > v) {
                return false;
            }
        } else if ((!creature.tim_mimic) || (creature.mimic_form != mimic_race_idx)) {
            msg_print(_("自分の体が変わってゆくのを感じた。", "You feel that your body changes."));
            creature.mimic_form = mimic_race_idx;
            notice = true;
        }
    }

    else {
        if (creature.tim_mimic) {
            msg_print(_("変身が解けた。", "You are no longer transformed."));
            if (creature.mimic_form == MimicKindType::DEMON) {
                set_oppose_fire(creature, 0, true);
            }
            creature.mimic_form = MimicKindType::NONE;
            notice = true;
            mimic_race_idx = MimicKindType::NONE;
        }
    }

    creature.tim_mimic = v;
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
        if (creature.berserk && !do_dec) {
            if (creature.berserk > v) {
                return false;
            }
        } else if (!creature.berserk) {
            msg_print(_("殺戮マシーンになった気がする！", "You feel like a killing machine!"));
            notice = true;
        }
    } else {
        if (creature.berserk) {
            msg_print(_("野蛮な気持ちが消え失せた。", "You feel less berserk."));
            notice = true;
        }
    }

    creature.berserk = v;
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
        if (creature.wraith_form && !do_dec) {
            if (creature.wraith_form > v) {
                return false;
            }
        } else if (!creature.wraith_form) {
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
        if (creature.wraith_form) {
            msg_print(_("不透明になった感じがする。", "You feel opaque."));
            notice = true;
            rfu.set_flag(MainWindowRedrawingFlag::MAP);
            rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
            rfu.set_flags(flags_swrf);
        }
    }

    creature.wraith_form = v;
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
        if (creature.tsuyoshi && !do_dec) {
            if (creature.tsuyoshi > v) {
                return false;
            }
        } else if (!creature.tsuyoshi) {
            msg_print(_("「オクレ兄さん！」", "Brother OKURE!"));
            notice = true;
            chg_virtue(creature, Virtue::VITALITY, 2);
        }
    } else {
        if (creature.tsuyoshi) {
            msg_print(_("肉体が急速にしぼんでいった。", "Your body has quickly shriveled."));

            (void)dec_stat(creature, A_CON, 20, true);
            (void)dec_stat(creature, A_STR, 20, true);

            notice = true;
            chg_virtue(creature, Virtue::VITALITY, -3);
        }
    }

    creature.tsuyoshi = v;
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
