/*!
 * @brief 知識の石を発動させる処理
 * @date 2021/09/24
 * @author Hourier
 */

#include "specific-object/stone-of-lore.h"
#include "player/player-damage.h"
#include "spell-kind/spells-perception.h"
#include "status/bad-status-setter.h"
#include "system/creature-entity.h"
#include "system/redrawing-flags-updater.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

StoneOfLore::StoneOfLore(CreatureEntity &creature)
    : creature_ptr(&creature)
{
}

/*!
 * @brief 知識の石の発動を実行する / Do activation of the stone of lore.
 * @param creature クリーチャーへの参照
 * @return 実行したらTRUE、しなかったらFALSE
 * @details
 * 鑑定を実行した後HPを消費する。1/5で混乱し、1/20で追加ダメージ。
 * MPがある場合はさらにMPを20消費する。不足する場合は麻痺及び混乱。
 */
bool StoneOfLore::perilous_secrets()
{
    msg_print(_("石が隠された秘密を写し出した．．．", "The stone reveals hidden mysteries..."));
    if (!ident_spell(*this->creature_ptr, false)) {
        return false;
    }

    this->consume_mp();
    auto dam_source = _("危険な秘密", "perilous secrets");
    take_hit(*this->creature_ptr, DAMAGE_LOSELIFE, Dice::roll(1, 12), dam_source);
    if (one_in_(5)) {
        (void)BadStatusSetter(*this->creature_ptr).mod_confusion(randnum1<short>(10));
    }

    if (one_in_(20)) {
        take_hit(*this->creature_ptr, DAMAGE_LOSELIFE, Dice::roll(4, 10), dam_source);
    }

    return true;
}

void StoneOfLore::consume_mp()
{
    if (this->creature_ptr->get_max_mp() <= 0) {
        return;
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (this->creature_ptr->get_current_mp() >= 20) {
        this->creature_ptr->sub_current_mp(20);
        rfu.set_flag(MainWindowRedrawingFlag::MP);
        return;
    }

    auto oops = 20 - this->creature_ptr->get_current_mp();
    this->creature_ptr->set_current_mp(0);
    this->creature_ptr->current_mp_frac = 0;
    msg_print(_("石を制御できない！", "You are too weak to control the stone!"));
    BadStatusSetter bss(*this->creature_ptr);
    (void)bss.mod_paralysis(randnum1<short>(5 * oops + 1));
    (void)bss.mod_confusion(randnum1<short>(5 * oops + 1));
    rfu.set_flag(MainWindowRedrawingFlag::MP);
}
