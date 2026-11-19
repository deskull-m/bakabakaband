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
#include "store/store-screen.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "view/display-store.h"
#include "window/display-sub-windows.h"

/*!
 * @brief 店舗処理コマンド選択のメインルーチン /
 * Process a command in a store
 * @param creature クリーチャーへの参照
 * @param store コマンドの対象となる店舗
 * @return 店から出るならtrue
 * @note
 * <pre>
 * Note that we must allow the use of a few "special" commands
 * in the stores which are not allowed in the dungeon, and we
 * must disable some commands which are allowed in the dungeon
 * but not in the stores, to prevent chaos.
 * </pre>
 */
bool store_process_command(CreatureEntity &creature, StoreScreen &screen)
{
    const auto store_num = screen.get_store().get_sale_type();
    repeat_check();
    if (rogue_like_commands && (command_cmd == 'l')) {
        command_cmd = 'x';
    }

    switch (command_cmd) {
    case ESCAPE: {
        return true;
    }
    case '-': {
        /* 日本語版追加 */
        /* 1 ページ戻るコマンド: 我が家のページ数が多いので重宝するはず By BUG */
        if (!screen.has_multiple_pages()) {
            msg_print(_("これで全部です。", "Entire inventory is shown."));
        } else {
            screen.turn_page_backward();
            display_store_inventory(creature, screen);
        }

        return false;
    }
    case ' ': {
        if (!screen.has_multiple_pages()) {
            msg_print(_("これで全部です。", "Entire inventory is shown."));
        } else {
            screen.turn_page_forward();
            display_store_inventory(creature, screen);
        }

        return false;
    }
    case KTRL('R'): {
        do_cmd_redraw(creature);
        display_store(creature, screen);
        return false;
    }
    case 'g': {
        store_purchase(creature, screen);
        return false;
    }
    case 'd': {
        store_sell(creature, screen);
        return false;
    }
    case 'x': {
        store_examine(creature, screen);
        return false;
    }
    case '\r': {
        return false;
    }
    case 'w': {
        do_cmd_wield(creature);
        return false;
    }
    case 't': {
        do_cmd_takeoff(creature);
        return false;
    }
    case 'k': {
        do_cmd_destroy(creature);
        return false;
    }
    case 'e': {
        do_cmd_equip(creature);
        return false;
    }
    case 'i': {
        do_cmd_inven(creature);
        return false;
    }
    case 'I': {
        do_cmd_observe(creature);
        return false;
    }
    case KTRL('I'): {
        toggle_inventory_equipment();
        return false;
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

        return false;
    }
    case '{': {
        do_cmd_inscribe(creature);
        return false;
    }
    case '}': {
        do_cmd_uninscribe(creature);
        return false;
    }
    case '?': {
        do_cmd_help(creature);
        return false;
    }
    case '/': {
        do_cmd_query_symbol(creature);
        return false;
    }
    case 'C': {
        do_cmd_player_status(creature);
        display_store(creature, screen);
        return false;
    }
    case '!':
        term_user();
        return false;
    case '"': {
        do_cmd_pref(creature);
        return false;
    }
    case '@': {
        do_cmd_macros(creature);
        return false;
    }
    case '%': {
        do_cmd_visuals(creature);
        return false;
    }
    case '&': {
        do_cmd_colors(creature);
        return false;
    }
    case '=': {
        do_cmd_options(creature);
        (void)combine_and_reorder_home(creature, StoreSaleType::HOME);
        do_cmd_redraw(creature);
        display_store(creature, screen);
        return false;
    }
    case ':': {
        do_cmd_note();
        return false;
    }
    case 'V': {
        do_cmd_version();
        return false;
    }
    case KTRL('F'): {
        do_cmd_feeling(creature);
        return false;
    }
    case KTRL('O'): {
        do_cmd_message_one();
        return false;
    }
    case KTRL('P'): {
        do_cmd_messages(0);
        return false;
    }
    case '|': {
        do_cmd_diary(creature);
        return false;
    }
    case '~': {
        do_cmd_knowledge(creature);
        return false;
    }
    case '(': {
        do_cmd_load_screen();
        return false;
    }
    case ')': {
        do_cmd_save_screen(creature);
        return false;
    }
    default: {
        if ((store_num == StoreSaleType::MUSEUM) && (command_cmd == 'r')) {
            museum_remove_object(creature, screen);
        } else {
            msg_print(_("そのコマンドは店の中では使えません。", "That command does not work in stores."));
        }

        return false;
    }
    }
}
