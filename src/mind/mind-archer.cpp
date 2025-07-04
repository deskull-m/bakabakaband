#include "mind/mind-archer.h"
#include "action/action-limited.h"
#include "autopick/autopick.h"
#include "core/asking-player.h"
#include "flavor/flavor-describer.h"
#include "floor/floor-object.h"
#include "floor/geometry.h"
#include "grid/grid.h"
#include "inventory/inventory-object.h"
#include "io/command-repeater.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/item-magic-applier.h"
#include "object-enchant/object-boost.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "perception/object-perception.h"
#include "system/angband.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain/terrain-definition.h"
#include "target/target-getter.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include <string>

enum ammo_creation_type {
    AMMO_NONE = 0,
    AMMO_SHOT = 1,
    AMMO_ARROW = 2,
    AMMO_BOLT = 3,
};

static bool select_ammo_creation_type(ammo_creation_type &type, PLAYER_LEVEL plev)
{
    const auto code = repeat_pull();
    if (code) {
        type = i2enum<ammo_creation_type>(*code);
        switch (type) {
        case AMMO_SHOT:
        case AMMO_ARROW:
        case AMMO_BOLT:
            return true;
        case AMMO_NONE:
        default:
            break;
        }
    }

    std::string prompt;
    if (plev >= 20) {
        prompt = _("[S]弾, [A]矢, [B]クロスボウの矢 :", "Create [S]hots, Create [A]rrow or Create [B]olt ?");
    } else if (plev >= 10) {
        prompt = _("[S]弾, [A]矢:", "Create [S]hots or Create [A]rrow ?");
    } else {
        prompt = _("[S]弾:", "Create [S]hots ?");
    }

    while (type == AMMO_NONE) {
        const auto ch = input_command(prompt, true);
        if (!ch) {
            return false;
        }

        if (ch == 'S' || ch == 's') {
            type = AMMO_SHOT;
            break;
        }

        if ((ch == 'A' || ch == 'a') && (plev >= 10)) {
            type = AMMO_ARROW;
            break;
        }

        if ((ch == 'B' || ch == 'b') && (plev >= 20)) {
            type = AMMO_BOLT;
            break;
        }
    }

    repeat_push(static_cast<COMMAND_CODE>(type));
    return true;
}

/*!
 * @brief「弾/矢の製造」処理 / do_cmd_cast calls this function if the player's class is 'archer'.
 * Hook to determine if an object is contertible in an arrow/bolt
 * @return 製造を実際に行ったらTRUE、キャンセルしたらFALSEを返す
 */
bool create_ammo(PlayerType *player_ptr)
{
    if (cmd_limit_confused(player_ptr) || cmd_limit_blind(player_ptr)) {
        return false;
    }

    ammo_creation_type ext = AMMO_NONE;

    if (!select_ammo_creation_type(ext, player_ptr->lev)) {
        return false;
    }

    switch (ext) {
    case AMMO_SHOT: {
        const auto dir = get_rep_dir(player_ptr);
        if (!dir) {
            return false;
        }

        const auto pos = player_ptr->get_neighbor(dir);
        const auto &grid = player_ptr->current_floor_ptr->get_grid(pos);
        if (grid.get_terrain(TerrainKind::MIMIC).flags.has_not(TerrainCharacteristics::CAN_DIG)) {
            msg_print(_("そこには岩石がない。", "You need a pile of rubble."));
            return false;
        }

        if (!grid.has(TerrainCharacteristics::CAN_DIG) || !grid.has(TerrainCharacteristics::HURT_ROCK)) {
            msg_print(_("硬すぎて崩せなかった。", "You failed to make ammo."));
            return true;
        }

        ItemEntity item({ ItemKindType::SHOT, m_bonus(1, player_ptr->lev) + 1 });
        item.number = rand_range(15, 30);
        object_aware(player_ptr, item);
        item.mark_as_known();
        ItemMagicApplier(player_ptr, &item, player_ptr->lev, AM_NO_FIXED_ART).execute();
        item.discount = 99;
        int16_t slot = store_item_to_inventory(player_ptr, &item);
        const auto item_name = describe_flavor(player_ptr, item, 0);
        msg_print(_(format("%sを作った。", item_name.data()), "You make some ammo."));
        if (slot >= 0) {
            autopick_alter_item(player_ptr, slot, false);
        }

        cave_alter_feat(player_ptr, pos.y, pos.x, TerrainCharacteristics::HURT_ROCK);
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::FLOW);
        return true;
    }
    case AMMO_ARROW: {
        constexpr auto q = _("どのアイテムから作りますか？ ", "Convert which item? ");
        constexpr auto s = _("材料を持っていない。", "You have no item to convert.");
        short i_idx;
        auto *q_ptr = choose_object(player_ptr, &i_idx, q, s, USE_INVEN | USE_FLOOR, FuncItemTester(&ItemEntity::is_convertible));
        if (!q_ptr) {
            return false;
        }
        ItemEntity ammo({ ItemKindType::ARROW, m_bonus(1, player_ptr->lev) + 1 });
        ammo.number = rand_range(5, 10);
        object_aware(player_ptr, ammo);
        ammo.mark_as_known();
        ItemMagicApplier(player_ptr, &ammo, player_ptr->lev, AM_NO_FIXED_ART).execute();
        ammo.discount = 99;
        const auto item_name = describe_flavor(player_ptr, ammo, 0);
        msg_print(_(format("%sを作った。", item_name.data()), "You make some ammo."));
        vary_item(player_ptr, i_idx, -1);
        int16_t slot = store_item_to_inventory(player_ptr, &ammo);
        if (slot >= 0) {
            autopick_alter_item(player_ptr, slot, false);
        }

        return true;
    }
    case AMMO_BOLT: {
        constexpr auto q = _("どのアイテムから作りますか？ ", "Convert which item? ");
        constexpr auto s = _("材料を持っていない。", "You have no item to convert.");
        short i_idx;
        auto *q_ptr = choose_object(player_ptr, &i_idx, q, s, (USE_INVEN | USE_FLOOR), FuncItemTester(&ItemEntity::is_convertible));
        if (!q_ptr) {
            return false;
        }

        ItemEntity ammo({ ItemKindType::BOLT, m_bonus(1, player_ptr->lev) + 1 });
        ammo.number = rand_range(4, 8);
        object_aware(player_ptr, ammo);
        ammo.mark_as_known();
        ItemMagicApplier(player_ptr, &ammo, player_ptr->lev, AM_NO_FIXED_ART).execute();
        ammo.discount = 99;
        const auto item_name = describe_flavor(player_ptr, ammo, 0);
        msg_print(_(format("%sを作った。", item_name.data()), "You make some ammo."));
        vary_item(player_ptr, i_idx, -1);
        int16_t slot = store_item_to_inventory(player_ptr, &ammo);
        if (slot >= 0) {
            autopick_alter_item(player_ptr, slot, false);
        }

        return true;
    }
    default:
        return true;
    }
}
