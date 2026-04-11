#include "cmd-io/cmd-save.h"
#include "cmd-io/cmd-dump.h"
#include "core/disturbance.h"
#include "core/stuff-handler.h"
#include "io/signal-handlers.h"
#include "io/write-diary.h"
#include "monster/monster-status.h" // 違和感。要調査.
#include "save/save.h"
#include "system/creature-entity.h"
#include "term/screen-processor.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief セーブするコマンドのメインルーチン
 * Save the game
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param is_autosave オートセーブ中の処理ならばTRUE
 * @details
 */
void do_cmd_save_game(CreatureEntity &creature, int is_autosave)
{
    if (is_autosave) {
        msg_print(_("自動セーブ中", "Autosaving the game..."));
    } else {
        disturb(creature, true, true);
    }

    msg_erase();
    handle_stuff(creature);
    prt(_("ゲームをセーブしています...", "Saving game..."), 0, 0);
    term_fresh();
    creature.died_from = _("(セーブ)", "(saved)");
    signals_ignore_tstp();
    if (save_player(creature, SaveType::CONTINUE_GAME)) {
        prt(_("ゲームをセーブしています... 終了", "Saving game... done."), 0, 0);
    } else {
        prt(_("ゲームをセーブしています... 失敗！", "Saving game... failed!"), 0, 0);
    }

    signals_handle_tstp();
    term_fresh();
    creature.died_from = _("(元気に生きている)", "(alive and well)");
}

/*!
 * @brief セーブ後にゲーム中断フラグを立てる/
 * Save the game and exit
 * @details
 */
void do_cmd_save_and_exit(CreatureEntity &creature)
{
    creature.playing = false;
    creature.leaving = true;
    exe_write_diary(*creature.get_floor(), DiaryKind::GAMESTART, 0, _("----ゲーム中断----", "--- Saved and Exited Game ---"));
}
