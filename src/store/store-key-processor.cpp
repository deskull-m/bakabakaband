#include "store/store-key-processor.h"
#include "autopick/autopick-pref-processor.h"
#include "cmd-action/cmd-mind.h"
#include "cmd-action/cmd-spell.h"
#include "cmd-io/cmd-diary.h"
#include "cmd-io/cmd-dump.h"
#include "cmd-io/cmd-gameoption.h"
#include "cmd-io/cmd-help.h"
#include "cmd-io/cmd-knowledge.h"
#include "cmd-io/cmd-lore.h"
#include "cmd-io/cmd-macro.h"
#include "cmd-io/cmd-process-screen.h"
#include "cmd-item/cmd-destroy.h"
#include "cmd-item/cmd-equipment.h"
#include "cmd-item/cmd-item.h"
#include "cmd-item/cmd-magiceat.h"
#include "cmd-visual/cmd-draw.h"
#include "cmd-visual/cmd-visuals.h"
#include "game-option/birth-options.h"
#include "game-option/input-options.h"
#include "io/command-repeater.h"
#include "io/input-key-requester.h"
#include "mind/mind-elementalist.h"
#include "mind/mind-sniper.h"
#include "mind/mind-weaponsmith.h"
#include "player-base/player-class.h"
#include "store/home.h"
#include "store/museum.h"
#include "store/purchase-order.h"
#include "store/sell-order.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "view/display-store.h"
#include "window/display-sub-windows.h"

/* Set this to leave the store */
bool leave_store = false;

/*!
 * @brief 店舗処理コマンド選択のメインルーチン /
 * Process a command in a store
 * @param player_ptr プレイヤーへの参照ポインタ
 * @note
 * <pre>
 * Note that we must allow the use of a few "special" commands
 * in the stores which are not allowed in the dungeon, and we
 * must disable some commands which are allowed in the dungeon
 * but not in the stores, to prevent chaos.
 * </pre>
 */
void store_process_command(CreatureEntity &creature, StoreSaleType store_num)
{
    repeat_check();
    if (rogue_like_commands && (command_cmd == 'l')) {
        command_cmd = 'x';
    }

    switch (command_cmd) {
    case ESCAPE: {
        leave_store = true;
        break;
    }
    case '-': {
        /* 日本語版追加 */
        /* 1 ページ戻るコマンド: 我が家のページ数が多いので重宝するはず By BUG */
        if (st_ptr->stock_num <= store_bottom) {
            msg_print(_("これで全部です。", "Entire inventory is shown."));
        } else {
            store_top -= store_bottom;
            if (store_top < 0) {
                store_top = ((st_ptr->stock_num - 1) / store_bottom) * store_bottom;
            }

            if ((store_num == StoreSaleType::HOME) && !powerup_home) {
                if (store_top >= store_bottom) {
                    store_top = store_bottom;
                }
            }

            display_store_inventory(creature, store_num);
        }

        break;
    }
    case ' ': {
        if (st_ptr->stock_num <= store_bottom) {
            msg_print(_("これで全部です。", "Entire inventory is shown."));
        } else {
            store_top += store_bottom;

            /*
             * 隠しオプション(powerup_home)がセットされていないときは
             * 我が家では 2 ページまでしか表示しない
             */
            auto inven_max = store_get_stock_max(store_num, powerup_home);
            if (store_top >= st_ptr->stock_num || store_top >= inven_max) {
                store_top = 0;
            }

            display_store_inventory(creature, store_num);
        }

        break;
    }
    case KTRL('R'): {
        do_cmd_redraw(creature);
        display_store(creature, store_num);
        break;
    }
    case 'g': {
        store_purchase(creature, store_num);
        break;
    }
    case 'd': {
        store_sell(creature, store_num);
        break;
    }
    case 'x': {
        store_examine(creature, store_num);
        break;
    }
    case '\r': {
        break;
    }
    case 'w': {
        do_cmd_wield(creature);
        break;
    }
    case 't': {
        do_cmd_takeoff(creature);
        break;
    }
    case 'k': {
        do_cmd_destroy(creature);
        break;
    }
    case 'e': {
        do_cmd_equip(creature);
        break;
    }
    case 'i': {
        do_cmd_inven(creature);
        break;
    }
    case 'I': {
        do_cmd_observe(creature);
        break;
    }
    case KTRL('I'): {
        toggle_inventory_equipment();
        break;
    }
    case 'b': {
        CreatureClass pc(creature);
        if (pc.can_browse()) {
            do_cmd_mind_browse(creature);
        } else if (pc.equals(PlayerClassType::ELEMENTALIST)) {
            do_cmd_element_browse(creature);
        } else if (pc.equals(PlayerClassType::SMITH)) {
            do_cmd_kaji(creature, true);
        } else if (pc.equals(PlayerClassType::MAGIC_EATER)) {
            do_cmd_magic_eater(creature, true, false);
        } else if (pc.equals(PlayerClassType::SNIPER)) {
            do_cmd_snipe_browse(creature);
        } else {
            do_cmd_browse(creature);
        }

        break;
    }
    case '{': {
        do_cmd_inscribe(creature);
        break;
    }
    case '}': {
        do_cmd_uninscribe(creature);
        break;
    }
    case '?': {
        do_cmd_help(creature);
        break;
    }
    case '/': {
        do_cmd_query_symbol(creature);
        break;
    }
    case 'C': {
        creature.town_num = old_town_num;
        do_cmd_player_status(&creature);
        creature.town_num = inner_town_num;
        display_store(creature, store_num);
        break;
    }
    case '!':
        term_user();
        break;
    case '"': {
        creature.town_num = old_town_num;
        do_cmd_pref(creature);
        creature.town_num = inner_town_num;
        break;
    }
    case '@': {
        creature.town_num = old_town_num;
        do_cmd_macros(creature);
        creature.town_num = inner_town_num;
        break;
    }
    case '%': {
        creature.town_num = old_town_num;
        do_cmd_visuals(creature);
        creature.town_num = inner_town_num;
        break;
    }
    case '&': {
        creature.town_num = old_town_num;
        do_cmd_colors(creature);
        creature.town_num = inner_town_num;
        break;
    }
    case '=': {
        do_cmd_options(creature);
        (void)combine_and_reorder_home(creature, StoreSaleType::HOME);
        do_cmd_redraw(creature);
        display_store(creature, store_num);
        break;
    }
    case ':': {
        do_cmd_note();
        break;
    }
    case 'V': {
        do_cmd_version();
        break;
    }
    case KTRL('F'): {
        do_cmd_feeling(creature);
        break;
    }
    case KTRL('O'): {
        do_cmd_message_one();
        break;
    }
    case KTRL('P'): {
        do_cmd_messages(0);
        break;
    }
    case '|': {
        do_cmd_diary(creature);
        break;
    }
    case '~': {
        do_cmd_knowledge(creature);
        break;
    }
    case '(': {
        do_cmd_load_screen();
        break;
    }
    case ')': {
        do_cmd_save_screen(creature);
        break;
    }
    default: {
        if ((store_num == StoreSaleType::MUSEUM) && (command_cmd == 'r')) {
            museum_remove_object(creature);
        } else {
            msg_print(_("そのコマンドは店の中では使えません。", "That command does not work in stores."));
        }

        break;
    }
    }
}
