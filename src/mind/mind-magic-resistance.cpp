#include "mind/mind-magic-resistance.h"
#include "action/travel-execution.h"
#include "core/disturbance.h"
#include "core/stuff-handler.h"
#include "game-option/disturbance-options.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "status/status-change-notice.h"
#include "system/creature-entity.h"
#include "system/redrawing-flags-updater.h"
#include "view/display-messages.h"

/*!
 * @brief 連奇術師の耐魔法防御 / 鏡使いの水鏡の盾 の継続時間をセットする / Set "resist_magic", notice observable changes
 * @param creature クリーチャーへの参照
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_resist_magic(CreatureEntity &creature, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (creature.is_dead()) {
        return false;
    }

    if (v) {
        if (creature.get_timed_effect(CreatureTimedEffect::RESIST_MAGIC) && !do_dec) {
            if (creature.get_timed_effect(CreatureTimedEffect::RESIST_MAGIC) > v) {
                return false;
            }
        } else if (!creature.get_timed_effect(CreatureTimedEffect::RESIST_MAGIC)) {
            msg_print(_("魔法への耐性がついた。", "You have been protected from magic!"));
            notice = true;
        }
    } else {
        if (creature.get_timed_effect(CreatureTimedEffect::RESIST_MAGIC)) {
            msg_print(_("魔法に弱くなった。", "You are no longer protected from magic."));
            sound(SoundKind::BUFF_EXPIRE);
            notice = true;
        }
    }

    creature.set_timed_effect(CreatureTimedEffect::RESIST_MAGIC, v);
    return notice_bonus_status_change(creature, notice);
}
