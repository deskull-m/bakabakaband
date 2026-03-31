/*!
 * @brief ロッドを振る処理
 * @date 2021/09/25
 * @author Hourier
 */
#include "object-use/zaprod-execution.h"
#include "action/action-limited.h"
#include "avatar/avatar.h"
#include "cmd-item/cmd-zaprod.h" // 相互依存。暫定的措置、後で何とかする.
#include "core/window-redrawer.h"
#include "game-option/disturbance-options.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object-use/item-use-checker.h"
#include "object/object-info.h"
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player-status/player-energy.h"
#include "status/experience.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-rod-types.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief コンストラクタ
 * @param creature プレイヤーへの参照
 * @param item 使うオブジェクトの所持品ID
 */
ObjectZapRodEntity::ObjectZapRodEntity(CreatureEntity &creature)
    : creature_ptr(&creature)
{
}

/*!
 * @brief ロッドを使う
 */
void ObjectZapRodEntity::execute(INVENTORY_IDX i_idx)
{
    auto &player = static_cast<PlayerType &>(*this->creature_ptr);
    auto use_charge = true;
    auto *o_ptr = ref_item(player, i_idx);
    if ((i_idx < 0) && (o_ptr->number > 1)) {
        msg_print(_("まずはロッドを拾わなければ。", "You must first pick up the rods."));
        return;
    }

    auto dir = Direction::none();
    if (o_ptr->is_aiming_rod() || !o_ptr->is_aware()) {
        dir = get_aim_dir(player);
        if (!dir) {
            return;
        }
    }

    PlayerEnergy(player).set_player_turn_energy(100);
    if (!this->check_can_zap()) {
        return;
    }

    const auto item_level = o_ptr->get_baseitem_level();
    auto chance = player.skill_dev;
    if (player.is_confused()) {
        chance = chance / 2;
    }

    auto fail = item_level + 5;
    if (chance > fail) {
        fail -= (chance - fail) * 2;
    } else {
        chance -= (fail - chance) * 2;
    }

    if (fail < USE_DEVICE) {
        fail = USE_DEVICE;
    }

    if (chance < USE_DEVICE) {
        chance = USE_DEVICE;
    }

    bool success;
    if (CreatureClass(player).equals(PlayerClassType::BERSERKER)) {
        success = false;
    } else if (chance > fail) {
        success = randint0(chance * 2) >= fail;
    } else {
        success = randint0(fail * 2) < chance;
    }

    if (!success) {
        if (flush_failure) {
            flush();
        }

        msg_print(_("うまくロッドを使えなかった。", "You failed to use the rod properly."));
        sound(SoundKind::FAIL);
        return;
    }

    const auto base_pval = o_ptr->get_baseitem_pval();
    if ((o_ptr->number == 1) && (o_ptr->timeout)) {
        if (flush_failure) {
            flush();
        }

        msg_print(_("このロッドはまだ魔力を充填している最中だ。", "The rod is still charging."));
        return;
    } else if ((o_ptr->number > 1) && (o_ptr->timeout > base_pval * (o_ptr->number - 1))) {
        if (flush_failure) {
            flush();
        }

        msg_print(_("そのロッドはまだ充填中です。", "The rods are all still charging."));
        return;
    }

    sound(SoundKind::ZAP);
    auto ident = rod_effect(player, *o_ptr->bi_key.sval(), dir, &use_charge, false);
    if (use_charge) {
        o_ptr->timeout += base_pval;
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::COMBINATION,
        StatusRecalculatingFlag::REORDER,
    };
    rfu.set_flags(flags_srf);
    if (!(o_ptr->is_aware())) {
        chg_virtue(player, Virtue::PATIENCE, -1);
        chg_virtue(player, Virtue::CHANCE, 1);
        chg_virtue(player, Virtue::KNOWLEDGE, -1);
    }

    o_ptr->mark_as_tried();
    if ((ident != 0) && !o_ptr->is_aware()) {
        object_aware(player, *o_ptr);
        gain_exp(player, (item_level + (player.level >> 1)) / player.level);
    }

    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::INVENTORY,
        SubWindowRedrawingFlag::EQUIPMENT,
        SubWindowRedrawingFlag::PLAYER,
        SubWindowRedrawingFlag::FLOOR_ITEMS,
        SubWindowRedrawingFlag::FOUND_ITEMS,
    };
    rfu.set_flags(flags_swrf);
}

bool ObjectZapRodEntity::check_can_zap()
{
    auto &player = static_cast<PlayerType &>(*this->creature_ptr);
    if (cmd_limit_time_walk(player)) {
        return false;
    }

    return ItemUseChecker(player).check_stun(_("朦朧としていてロッドを振れなかった！", "You are too stunned to zap it!"));
}
