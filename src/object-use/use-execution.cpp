/*!
 * @brief 杖を振る処理
 * @date 2021/09/25
 * @author Hourier
 */
#include "object-use/use-execution.h"
#include "action/action-limited.h"
#include "avatar/avatar.h"
#include "cmd-item/cmd-usestaff.h"
#include "core/window-redrawer.h"
#include "floor/floor-object.h"
#include "game-option/disturbance-options.h"
#include "inventory/inventory-object.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object-enchant/special-object-flags.h"
#include "object-use/item-use-checker.h"
#include "object/object-info.h"
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player-status/player-energy.h"
#include "status/experience.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "view/object-describer.h"

/*!
 * @brief コンストラクタ
 * @param creature クリーチャーへの参照
 * @param i_idx 使うオブジェクトの所持品ID
 */
ObjectUseEntity::ObjectUseEntity(CreatureEntity &creature, INVENTORY_IDX i_idx)
    : creature_ptr(&creature)
    , i_idx(i_idx)
{
}

/*!
 * @brief 杖を使う
 */
void ObjectUseEntity::execute()
{
    auto &creature = *this->creature_ptr;
    auto use_charge = true;
    auto item = ref_item(creature, this->i_idx);
    if ((this->i_idx < 0) && (item->number > 1)) {
        msg_print(_("まずは杖を拾わなければ。", "You must first pick up the staffs."));
        return;
    }

    PlayerEnergy(creature).set_player_turn_energy(100);
    if (!this->check_can_use()) {
        return;
    }

    auto item_level = item->get_baseitem_level();
    if (item_level > 50) {
        item_level = 50 + (item_level - 50) / 2;
    }

    auto chance = creature.get_skill_device();
    if (creature.is_confused()) {
        chance = chance / 2;
    }

    chance = chance - item_level;
    if ((chance < USE_DEVICE) && one_in_(USE_DEVICE - chance + 1)) {
        chance = USE_DEVICE;
    }

    if ((chance < USE_DEVICE) || (randint1(chance) < USE_DEVICE) || CreatureClass(creature).equals(PlayerClassType::BERSERKER)) {
        if (flush_failure) {
            flush();
        }

        msg_print(_("杖をうまく使えなかった。", "You failed to use the staff properly."));
        sound(SoundKind::FAIL);
        return;
    }

    if (item->pval <= 0) {
        if (flush_failure) {
            flush();
        }

        msg_print(_("この杖にはもう魔力が残っていない。", "The staff has no charges left."));
        item->ident |= IDENT_EMPTY;
        auto &rfu = RedrawingFlagsUpdater::get_instance();
        static constexpr auto flags = {
            StatusRecalculatingFlag::COMBINATION,
            StatusRecalculatingFlag::REORDER,
        };
        rfu.set_flags(flags);
        rfu.set_flag(SubWindowRedrawingFlag::INVENTORY);
        return;
    }

    sound(SoundKind::ZAP);
    auto ident = staff_effect(creature, *item->bi_key.sval(), &use_charge, false, false, item->is_aware());
    if (!(item->is_aware())) {
        chg_virtue(creature, Virtue::PATIENCE, -1);
        chg_virtue(creature, Virtue::CHANCE, 1);
        chg_virtue(creature, Virtue::KNOWLEDGE, -1);
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    using Srf = StatusRecalculatingFlag;
    EnumClassFlagGroup<Srf> flags_srf = { Srf::COMBINATION, Srf::REORDER };
    if (rfu.has(Srf::AUTO_DESTRUCTION)) {
        flags_srf.set(Srf::AUTO_DESTRUCTION);
    }

    rfu.reset_flags(flags_srf);
    item->mark_as_tried();
    if (ident && !item->is_aware()) {
        object_aware(creature, *item);
        gain_exp(creature, (item_level + (creature.level >> 1)) / creature.level);
    }

    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::INVENTORY,
        SubWindowRedrawingFlag::EQUIPMENT,
        SubWindowRedrawingFlag::PLAYER,
        SubWindowRedrawingFlag::FLOOR_ITEMS,
        SubWindowRedrawingFlag::FOUND_ITEMS,
    };
    rfu.set_flags(flags_swrf);
    rfu.set_flags(flags_srf);
    if (!use_charge) {
        return;
    }

    item->pval--;
    if ((this->i_idx >= 0) && (item->number > 1)) {
        auto used_item = item->clone();
        used_item.number = 1;
        item->pval++;
        item->number--;
        this->i_idx = creature.store_item(used_item);
        msg_print(_("杖をまとめなおした。", "You unstack your staff."));
    }

    if (this->i_idx >= 0) {
        inven_item_charges(*creature.inventory[this->i_idx]);
    } else {
        floor_item_charges(*creature.get_floor(), 0 - this->i_idx);
    }
}

bool ObjectUseEntity::check_can_use()
{
    auto &creature = *this->creature_ptr;
    if (cmd_limit_time_walk(creature)) {
        return false;
    }

    return ItemUseChecker(creature).check_stun(_("朦朧としていて杖を振れなかった！", "You are too stunned to use it!"));
}
