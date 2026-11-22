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
 * @brief 一定ターン毎の自動セーブを要求されていて、まだセーブしていないか
 */
static bool is_auto_save_requested = false;

/*!
 * @brief セーブするコマンドのメインルーチン
 * Save the game
 * @param creature クリーチャーへの参照
 * @param is_autosave オートセーブ中の処理ならばTRUE
 * @details
 */
void do_cmd_save_game(CreatureEntity &creature, int is_autosave)
{
    // どのセーブでも、要求されていた自動セーブの分を兼ねる
    is_auto_save_requested = false;
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
    creature.set_playing(false);
    creature.set_leaving(true);
    exe_write_diary(*creature.get_floor(), DiaryKind::GAMESTART, 0, _("----ゲーム中断----", "--- Saved and Exited Game ---"));
}

/*!
 * @brief 一定ターン毎の自動セーブを要求する
 * @details 次にコマンドの入力を待つところで do_requested_auto_save() がセーブする
 */
void request_auto_save()
{
    is_auto_save_requested = true;
}

/*!
 * @brief 要求されている一定ターン毎の自動セーブを行う
 * @param creature クリーチャーへの参照
 * @details コマンドの入力を待つところで呼ぶ (process_player())。読み込むと、^S でセーブしたときと同じく手番の途中から再開する。
 */
void do_requested_auto_save(CreatureEntity &creature)
{
    if (!is_auto_save_requested) {
        return;
    }

    do_cmd_save_game(creature, true);
}
