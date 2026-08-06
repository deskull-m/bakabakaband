#include "status/sight-setter.h"
#include "core/disturbance.h"
#include "core/stuff-handler.h"
#include "game-option/disturbance-options.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "realm/realm-song-numbers.h"
#include "spell-realm/spells-song.h"
#include "system/creature-entity.h"
#include "system/redrawing-flags-updater.h"
#include "view/display-messages.h"

static bool update_sight(CreatureEntity &creature, const bool notice)
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(creature, false, false);
    }

    static constexpr auto flags = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::MONSTER_STATUSES,
    };
    rfu.set_flags(flags);
    handle_stuff(creature);
    return true;
}

/*!
 * @brief 時限ESPの継続時間をセットする / Set "tim_esp", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_esp(CreatureEntity &creature, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (creature.is_dead()) {
        return false;
    }

    if (v) {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_ESP) && !do_dec) {
            if (creature.get_timed_effect(CreatureTimedEffect::TIM_ESP) > v) {
                return false;
            }
        } else if (!creature.is_time_limit_esp()) {
            msg_print(_("意識が広がった気がする！", "You feel your consciousness expand!"));
            notice = true;
        }
    } else {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_ESP) && !music_singing(creature, MUSIC_MIND)) {
            msg_print(_("意識は元に戻った。", "Your consciousness contracts again."));
            sound(SoundKind::BUFF_EXPIRE);
            notice = true;
        }
    }

    creature.set_timed_effect(CreatureTimedEffect::TIM_ESP, v);
    return update_sight(creature, notice);
}

/*!
 * @brief 時限透明視の継続時間をセットする / Set "tim_invis", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_invis(CreatureEntity &creature, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (creature.is_dead()) {
        return false;
    }

    if (v) {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_INVIS) && !do_dec) {
            if (creature.get_timed_effect(CreatureTimedEffect::TIM_INVIS) > v) {
                return false;
            }
        } else if (!creature.get_timed_effect(CreatureTimedEffect::TIM_INVIS)) {
            msg_print(_("目が非常に敏感になった気がする！", "Your eyes feel very sensitive!"));
            notice = true;
        }
    } else {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_INVIS)) {
            msg_print(_("目の敏感さがなくなったようだ。", "Your eyes feel less sensitive."));
            sound(SoundKind::BUFF_EXPIRE);
            notice = true;
        }
    }

    creature.set_timed_effect(CreatureTimedEffect::TIM_INVIS, v);
    return update_sight(creature, notice);
}

/*!
 * @brief 時限赤外線視力の継続時間をセットする / Set "tim_infra", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_infra(CreatureEntity &creature, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (creature.is_dead()) {
        return false;
    }

    if (v) {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_INFRA) && !do_dec) {
            if (creature.get_timed_effect(CreatureTimedEffect::TIM_INFRA) > v) {
                return false;
            }
        } else if (!creature.get_timed_effect(CreatureTimedEffect::TIM_INFRA)) {
            msg_print(_("目がランランと輝き始めた！", "Your eyes begin to tingle!"));
            notice = true;
        }
    } else {
        if (creature.get_timed_effect(CreatureTimedEffect::TIM_INFRA)) {
            msg_print(_("目の輝きがなくなった。", "Your eyes stop tingling."));
            sound(SoundKind::BUFF_EXPIRE);
            notice = true;
        }
    }

    creature.set_timed_effect(CreatureTimedEffect::TIM_INFRA, v);
    return update_sight(creature, notice);
}
