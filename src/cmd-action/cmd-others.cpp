/*!
 * @brief その他の小さなコマンド処理群 (探索、汎用グリッド処理、自殺/引退/切腹)
 * @date 2014/01/02
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

#include "cmd-action/cmd-others.h"
#include "action/open-close-execution.h"
#include "action/tunnel-execution.h"
#include "cmd-action/cmd-attack.h"
#include "core/asking-player.h"
#include "core/disturbance.h"
#include "floor/geometry.h"
#include "game-option/game-play-options.h"
#include "grid/grid.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "io/write-diary.h"
#include "main/music-definitions-table.h"
#include "main/sound-of-music.h"
#include "player-base/player-class.h"
#include "player-info/samurai-data-type.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/player-move.h"
#include "player/special-defense-types.h"
#include "status/action-setter.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain-type-definition.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief 探索コマンドのメインルーチン / Simple command to "search" for one turn
 */
void do_cmd_search(PlayerType *player_ptr)
{
    if (command_arg) {
        command_rep = command_arg - 1;
        RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::ACTION);
        command_arg = 0;
    }

    PlayerEnergy(player_ptr).set_player_turn_energy(100);
    search(player_ptr);

    if (player_ptr->action == ACTION_SEARCH) {
        search(player_ptr);
    }
}

static bool exe_alter(PlayerType *player_ptr)
{
    DIRECTION dir;
    if (!get_rep_dir(player_ptr, &dir, true)) {
        return false;
    }

    const auto pos = player_ptr->get_neighbor(dir);
    const auto &grid = player_ptr->current_floor_ptr->get_grid(pos);
    const auto &terrain = grid.get_terrain_mimic();
    PlayerEnergy(player_ptr).set_player_turn_energy(100);
    if (grid.has_monster()) {
        do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_NONE);
        return false;
    }

    if (terrain.flags.has(TerrainCharacteristics::OPEN)) {
        return exe_open(player_ptr, pos.y, pos.x);
    }

    if (terrain.flags.has(TerrainCharacteristics::BASH)) {
        return exe_bash(player_ptr, pos.y, pos.x, dir);
    }

    if (terrain.flags.has(TerrainCharacteristics::TUNNEL)) {
        return exe_tunnel(player_ptr, pos.y, pos.x);
    }

    if (terrain.flags.has(TerrainCharacteristics::CLOSE)) {
        return exe_close(player_ptr, pos.y, pos.x);
    }

    if (terrain.flags.has(TerrainCharacteristics::DISARM)) {
        return exe_disarm(player_ptr, pos.y, pos.x, dir);
    }

    msg_print(_("何もない空中を攻撃した。", "You attack the empty air."));
    return false;
}

/*!
 * @brief 特定のマスに影響を及ぼすための汎用的コマンド / Manipulate an adjacent grid in some way
 * @details
 */
void do_cmd_alter(PlayerType *player_ptr)
{
    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU });

    if (command_arg) {
        command_rep = command_arg - 1;
        RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::ACTION);
        command_arg = 0;
    }

    if (!exe_alter(player_ptr)) {
        disturb(player_ptr, false, false);
    }
}

/*!
 * @brief 自殺/引退/切腹の確認
 * @param なし
 * @return 自殺/引退/切腹を実施するならTRUE、キャンセルならFALSE
 */
static bool decide_suicide()
{
    if (AngbandWorld::get_instance().noscore) {
        return true;
    }

    prt(_("確認のため '@' を押して下さい。", "Please verify SUICIDE by typing the '@' sign: "), 0, 0);
    flush();
    int i = inkey();
    prt("", 0, 0);
    return i == '@';
}

static void accept_winner_message(PlayerType *player_ptr)
{
    if (!AngbandWorld::get_instance().total_winner || !last_words) {
        return;
    }

    play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_WINNER);
    std::optional<std::string> buf;
    while (true) {
        buf = input_string(_("*勝利*メッセージ: ", "*Winning* message: "), 1024);
        if (!buf.has_value()) {
            continue;
        }

        if (input_check_strict(player_ptr, _("よろしいですか？", "Are you sure? "), UserCheck::NO_HISTORY)) {
            break;
        }
    }

    if (!buf->empty()) {
        player_ptr->last_message = buf.value();
        msg_print(player_ptr->last_message);
    }
}

/*!
 * @brief 自殺するコマンドのメインルーチン
 * commit suicide
 * @details
 */
void do_cmd_suicide(PlayerType *player_ptr)
{
    char i;
    flush();
    auto &world = AngbandWorld::get_instance();
    if (world.total_winner) {
        if (!input_check_strict(player_ptr, _("虚無りますか? ", "Do you want to go to the Nihil War? "), UserCheck::NO_HISTORY)) {
            return;
        }
    } else {
        if (!input_check(_("何もかも諦めますか? ", "Do you give up everything? "))) {
            return;
        }
    }

    /* Special Verification for suicide */
    prt(_("確認のため '@' を押して下さい。", "Please verify SUICIDE by typing the '@' sign: "), 0, 0);

    flush();
    i = inkey();
    prt("", 0, 0);
    if (i != '@') {
        return;
    }

    if (!decide_suicide()) {
        return;
    }

    player_ptr->last_message = "";
    player_ptr->playing = false;
    player_ptr->is_dead = true;
    player_ptr->leaving = true;
    if (world.total_winner) {
        accept_winner_message(player_ptr);
        world.add_retired_class(player_ptr->pclass);
    } else {
        play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_GAMEOVER);
        const auto &floor = *player_ptr->current_floor_ptr;
        exe_write_diary(floor, DiaryKind::DESCRIPTION, 0, _("ダンジョンの探索に飽きて自殺した。", "got tired to commit suicide."));
        exe_write_diary(floor, DiaryKind::GAMESTART, 1, _("-------- ゲームオーバー --------", "--------   Game  Over   --------"));
        exe_write_diary(floor, DiaryKind::DESCRIPTION, 1, "\n\n\n\n");
    }

    player_ptr->died_from = _("途中終了", "Quitting");
}
