#include "spell-kind/spells-curse-removal.h"
#include "core/window-redrawer.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/trc-types.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "system/redrawing-flags-updater.h"
#include "view/display-messages.h"

/*!
 * @brief 装備の解呪処理 / Removes curses from items in inventory
 * @param creature クリーチャーへの参照
 * @param all 軽い呪いまでの解除ならば0
 * @return 解呪されたアイテムの数
 * @details 永遠の呪いは解呪できない
 */
static int exe_curse_removal(CreatureEntity &creature, int all)
{
    auto count = 0;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        auto *o_ptr = creature.inventory[i].get();
        if (!o_ptr->is_valid() || !o_ptr->is_cursed()) {
            continue;
        }

        if (!all && o_ptr->curse_flags.has(CurseTraitType::HEAVY_CURSE)) {
            continue;
        }

        if (o_ptr->curse_flags.has(CurseTraitType::PERMA_CURSE)) {
            o_ptr->curse_flags &= { CurseTraitType::CURSED, CurseTraitType::HEAVY_CURSE, CurseTraitType::PERMA_CURSE };
            continue;
        }

        o_ptr->curse_flags.clear();
        o_ptr->ident.set(IdentificationFlag::SENSE);
        o_ptr->feeling = FEEL_NONE;
        rfu.set_flag(StatusRecalculatingFlag::BONUS);
        rfu.set_flag(SubWindowRedrawingFlag::EQUIPMENT);
        count++;
    }

    if (count > 0) {
        msg_print(_("誰かに見守られているような気がする。", "You feel as if someone is watching over you."));
    }

    return count;
}

/*!
 * @brief 装備の軽い呪い解呪処理 /
 * Remove most curses
 * @param creature クリーチャーへの参照
 * @return 解呪に成功した装備数
 */
int remove_curse(CreatureEntity &creature)
{
    return exe_curse_removal(creature, false);
}

/*!
 * @brief 装備の重い呪い解呪処理 /
 * Remove all curses
 * @return 解呪に成功した装備数
 */
int remove_all_curse(CreatureEntity &creature)
{
    return exe_curse_removal(creature, true);
}
