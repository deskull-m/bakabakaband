#include "object-use/zapwand-execution.h"
#include "action/action-limited.h"
#include "avatar/avatar.h"
#include "cmd-item/cmd-zapwand.h" // 相互依存。暫定的措置、後で何とかする.
#include "core/window-redrawer.h"
#include "floor/floor-object.h"
#include "game-option/disturbance-options.h"
#include "game-option/input-options.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object-enchant/special-object-flags.h"
#include "object-use/item-use-checker.h"
#include "object/object-info.h"
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player-status/player-energy.h"
#include "status/experience.h"
#include "sv-definition/sv-wand-types.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "system/redrawing-flags-updater.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "view/object-describer.h"

ObjectZapWandEntity::ObjectZapWandEntity(CreatureEntity &creature)
    : creature(creature)
{
}

/*!
 * @brief 魔法棒を使うコマンドのサブルーチン /
 * @param i_idx 使うオブジェクトの所持品ID
 */
void ObjectZapWandEntity::execute(INVENTORY_IDX i_idx)
{
    auto old_target_pet = target_pet;
    auto item = ref_item(this->creature, i_idx);
    if ((i_idx < 0) && (item->number > 1)) {
        msg_print(_("まずは魔法棒を拾わなければ。", "You must first pick up the wands."));
        return;
    }

    const auto sval = item->bi_key.sval();
    if (item->is_aware() && (sval == SV_WAND_HEAL_MONSTER || sval == SV_WAND_HASTE_MONSTER)) {
        target_pet = true;
    }

    const auto dir = get_aim_dir(this->creature);
    if (!dir) {
        target_pet = old_target_pet;
        return;
    }

    target_pet = old_target_pet;
    PlayerEnergy(this->creature).set_player_turn_energy(100);
    if (!this->check_can_zap()) {
        return;
    }

    auto item_level = item->get_baseitem_level();
    if (item_level > 50) {
        item_level = 50 + (item_level - 50) / 2;
    }

    auto chance = this->creature.get_skill_device();
    if (this->creature.is_confused()) {
        chance = chance / 2;
    }

    chance = chance - item_level;
    if ((chance < USE_DEVICE) && one_in_(USE_DEVICE - chance + 1)) {
        chance = USE_DEVICE;
    }

    if ((chance < USE_DEVICE) || (randint1(chance) < USE_DEVICE) || CreatureClass(this->creature).equals(PlayerClassType::BERSERKER)) {
        if (flush_failure) {
            flush();
        }

        msg_print(_("魔法棒をうまく使えなかった。", "You failed to use the wand properly."));
        sound(SoundKind::FAIL);
        return;
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (item->pval <= 0) {
        if (flush_failure) {
            flush();
        }

        msg_print(_("この魔法棒にはもう魔力が残っていない。", "The wand has no charges left."));
        item->ident.set(IdentificationFlag::EMPTY);
        static constexpr auto flags = {
            StatusRecalculatingFlag::COMBINATION,
            StatusRecalculatingFlag::REORDER,
        };
        rfu.set_flags(flags);
        rfu.set_flag(SubWindowRedrawingFlag::INVENTORY);
        return;
    }

    sound(SoundKind::ZAP);
    auto ident = wand_effect(this->creature, *sval, dir, false, false);
    using Srf = StatusRecalculatingFlag;
    EnumClassFlagGroup<Srf> flags_srf = { Srf::COMBINATION, Srf::REORDER };
    if (rfu.has(Srf::AUTO_DESTRUCTION)) {
        flags_srf.set(Srf::AUTO_DESTRUCTION);
    }

    rfu.reset_flags(flags_srf);
    if (!item->is_aware()) {
        chg_virtue(this->creature, Virtue::PATIENCE, -1);
        chg_virtue(this->creature, Virtue::CHANCE, 1);
        chg_virtue(this->creature, Virtue::KNOWLEDGE, -1);
    }

    item->mark_as_tried();
    if (ident && !item->is_aware()) {
        object_aware(this->creature, *item);
        gain_exp(this->creature, (item_level + (this->creature.get_level() >> 1)) / this->creature.get_level());
    }

    rfu.set_item_related_sub_window_flags();
    rfu.set_flags(flags_srf);
    item->pval--;
    if (i_idx >= 0) {
        inven_item_charges(*this->creature.inventory[i_idx]);
        return;
    }

    floor_item_charges(*this->creature.get_floor(), 0 - i_idx);
}

bool ObjectZapWandEntity::check_can_zap() const
{
    if (cmd_limit_time_walk(this->creature)) {
        return false;
    }

    return ItemUseChecker(this->creature).check_stun(_("朦朧としていて魔法棒を振れなかった！", "You are too stunned to zap it!"));
}
