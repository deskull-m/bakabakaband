#include "status/element-resistance.h"
#include "action/travel-execution.h"
#include "core/disturbance.h"
#include "core/stuff-handler.h"
#include "game-option/disturbance-options.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/race-info.h"
#include "player-info/samurai-data-type.h"
#include "player/special-defense-types.h"
#include "realm/realm-song-numbers.h"
#include "spell-realm/spells-song.h"
#include "system/creature-entity.h"
#include "system/redrawing-flags-updater.h"
#include "view/display-messages.h"

/*!
 * @brief 一時的酸耐性の継続時間をセットする / Set "oppose_acid", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_oppose_acid(CreatureEntity &creature, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;
    if (creature.is_dead()) {
        return false;
    }

    if (v) {
        if (creature.get_timed_effect(CreatureTimedEffect::OPPOSE_ACID) && !do_dec) {
            if (creature.get_timed_effect(CreatureTimedEffect::OPPOSE_ACID) > v) {
                return false;
            }
        } else if (!is_oppose_acid(creature)) {
            msg_print(_("酸への耐性がついた気がする！", "You feel resistant to acid!"));
            notice = true;
        }
    } else {
        if (creature.get_timed_effect(CreatureTimedEffect::OPPOSE_ACID) && !music_singing(creature, MUSIC_RESIST) &&
            !CreatureClass(creature).samurai_stance_is(SamuraiStanceType::MUSOU)) {
            msg_print(_("酸への耐性が薄れた気がする。", "You feel less resistant to acid."));
            sound(SoundKind::BUFF_EXPIRE);
            notice = true;
        }
    }

    creature.set_timed_effect(CreatureTimedEffect::OPPOSE_ACID, v);
    if (!notice) {
        return false;
    }

    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (disturb_state || Travel::get_instance().is_ongoing()) {
        disturb(creature, false, true);
    }

    handle_stuff(creature);
    return true;
}

/*!
 * @brief 一時的電撃耐性の継続時間をセットする / Set "oppose_elec", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_oppose_elec(CreatureEntity &creature, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (creature.is_dead()) {
        return false;
    }

    if (v) {
        if (creature.get_timed_effect(CreatureTimedEffect::OPPOSE_ELEC) && !do_dec) {
            if (creature.get_timed_effect(CreatureTimedEffect::OPPOSE_ELEC) > v) {
                return false;
            }
        } else if (!is_oppose_elec(creature)) {
            msg_print(_("電撃への耐性がついた気がする！", "You feel resistant to electricity!"));
            notice = true;
        }
    } else {
        if (creature.get_timed_effect(CreatureTimedEffect::OPPOSE_ELEC) && !music_singing(creature, MUSIC_RESIST) &&
            !CreatureClass(creature).samurai_stance_is(SamuraiStanceType::MUSOU)) {
            msg_print(_("電撃への耐性が薄れた気がする。", "You feel less resistant to electricity."));
            sound(SoundKind::BUFF_EXPIRE);
            notice = true;
        }
    }

    creature.set_timed_effect(CreatureTimedEffect::OPPOSE_ELEC, v);
    if (!notice) {
        return false;
    }

    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (disturb_state || Travel::get_instance().is_ongoing()) {
        disturb(creature, false, true);
    }

    handle_stuff(creature);
    return true;
}

/*!
 * @brief 一時的火炎耐性の継続時間をセットする / Set "oppose_fire", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_oppose_fire(CreatureEntity &creature, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;
    if (creature.is_dead()) {
        return false;
    }

    if ((CreatureRace(&creature).equals(PlayerRaceType::BALROG) && (creature.level > 44)) || (creature.get_mimic_form() == MimicKindType::DEMON)) {
        v = 1;
    }
    if (v) {
        if (creature.get_timed_effect(CreatureTimedEffect::OPPOSE_FIRE) && !do_dec) {
            if (creature.get_timed_effect(CreatureTimedEffect::OPPOSE_FIRE) > v) {
                return false;
            }
        } else if (!is_oppose_fire(creature)) {
            msg_print(_("火への耐性がついた気がする！", "You feel resistant to fire!"));
            notice = true;
        }
    } else {
        if (creature.get_timed_effect(CreatureTimedEffect::OPPOSE_FIRE) && !music_singing(creature, MUSIC_RESIST) &&
            !CreatureClass(creature).samurai_stance_is(SamuraiStanceType::MUSOU)) {
            msg_print(_("火への耐性が薄れた気がする。", "You feel less resistant to fire."));
            sound(SoundKind::BUFF_EXPIRE);
            notice = true;
        }
    }

    creature.set_timed_effect(CreatureTimedEffect::OPPOSE_FIRE, v);
    if (!notice) {
        return false;
    }

    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (disturb_state || Travel::get_instance().is_ongoing()) {
        disturb(creature, false, true);
    }

    handle_stuff(creature);
    return true;
}

/*!
 * @brief 一時的冷気耐性の継続時間をセットする / Set "oppose_cold", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_oppose_cold(CreatureEntity &creature, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;
    if (creature.is_dead()) {
        return false;
    }

    if (v) {
        if (creature.get_timed_effect(CreatureTimedEffect::OPPOSE_COLD) && !do_dec) {
            if (creature.get_timed_effect(CreatureTimedEffect::OPPOSE_COLD) > v) {
                return false;
            }
        } else if (!is_oppose_cold(creature)) {
            msg_print(_("冷気への耐性がついた気がする！", "You feel resistant to cold!"));
            notice = true;
        }
    } else {
        if (creature.get_timed_effect(CreatureTimedEffect::OPPOSE_COLD) && !music_singing(creature, MUSIC_RESIST) &&
            !CreatureClass(creature).samurai_stance_is(SamuraiStanceType::MUSOU)) {
            msg_print(_("冷気への耐性が薄れた気がする。", "You feel less resistant to cold."));
            sound(SoundKind::BUFF_EXPIRE);
            notice = true;
        }
    }

    creature.set_timed_effect(CreatureTimedEffect::OPPOSE_COLD, v);
    if (!notice) {
        return false;
    }

    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (disturb_state || Travel::get_instance().is_ongoing()) {
        disturb(creature, false, true);
    }

    handle_stuff(creature);
    return true;
}

/*!
 * @brief 一時的毒耐性の継続時間をセットする / Set "oppose_pois", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_oppose_pois(CreatureEntity &creature, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;
    CreatureClass pc(creature);
    if (pc.has_poison_resistance()) {
        v = 1;
    }
    if (creature.is_dead()) {
        return false;
    }

    if (v) {
        if (creature.get_timed_effect(CreatureTimedEffect::OPPOSE_POIS) && !do_dec) {
            if (creature.get_timed_effect(CreatureTimedEffect::OPPOSE_POIS) > v) {
                return false;
            }
        } else if (!is_oppose_pois(creature)) {
            msg_print(_("毒への耐性がついた気がする！", "You feel resistant to poison!"));
            notice = true;
        }
    } else {
        if (creature.get_timed_effect(CreatureTimedEffect::OPPOSE_POIS) && !music_singing(creature, MUSIC_RESIST) &&
            !pc.samurai_stance_is(SamuraiStanceType::MUSOU)) {
            msg_print(_("毒への耐性が薄れた気がする。", "You feel less resistant to poison."));
            sound(SoundKind::BUFF_EXPIRE);
            notice = true;
        }
    }

    creature.set_timed_effect(CreatureTimedEffect::OPPOSE_POIS, v);
    if (!notice) {
        return false;
    }

    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (disturb_state || Travel::get_instance().is_ongoing()) {
        disturb(creature, false, true);
    }

    handle_stuff(creature);
    return true;
}

bool is_oppose_acid(CreatureEntity &creature)
{
    auto can_oppose_acid = creature.get_timed_effect(CreatureTimedEffect::OPPOSE_ACID) != 0;
    can_oppose_acid |= music_singing(creature, MUSIC_RESIST);
    can_oppose_acid |= CreatureClass(creature).samurai_stance_is(SamuraiStanceType::MUSOU);
    return can_oppose_acid;
}

bool is_oppose_elec(CreatureEntity &creature)
{
    auto can_oppose_elec = creature.get_timed_effect(CreatureTimedEffect::OPPOSE_ELEC) != 0;
    can_oppose_elec |= music_singing(creature, MUSIC_RESIST);
    can_oppose_elec |= CreatureClass(creature).samurai_stance_is(SamuraiStanceType::MUSOU);
    return can_oppose_elec;
}

bool is_oppose_fire(CreatureEntity &creature)
{
    auto can_oppose_fire = creature.get_timed_effect(CreatureTimedEffect::OPPOSE_FIRE) != 0;
    can_oppose_fire |= music_singing(creature, MUSIC_RESIST);
    can_oppose_fire |= CreatureClass(creature).samurai_stance_is(SamuraiStanceType::MUSOU);
    can_oppose_fire |= creature.get_mimic_form() == MimicKindType::DEMON;
    can_oppose_fire |= (CreatureRace(&creature).equals(PlayerRaceType::BALROG) && creature.level > 44);
    return can_oppose_fire;
}

bool is_oppose_cold(CreatureEntity &creature)
{
    auto can_oppose_cold = creature.get_timed_effect(CreatureTimedEffect::OPPOSE_COLD) != 0;
    can_oppose_cold |= music_singing(creature, MUSIC_RESIST);
    can_oppose_cold |= CreatureClass(creature).samurai_stance_is(SamuraiStanceType::MUSOU);
    return can_oppose_cold;
}

bool is_oppose_pois(CreatureEntity &creature)
{
    CreatureClass pc(creature);
    auto can_oppose_pois = creature.get_timed_effect(CreatureTimedEffect::OPPOSE_POIS) != 0;
    can_oppose_pois |= music_singing(creature, MUSIC_RESIST);
    can_oppose_pois |= pc.samurai_stance_is(SamuraiStanceType::MUSOU);
    can_oppose_pois |= pc.has_poison_resistance();
    return can_oppose_pois;
}
