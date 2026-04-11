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
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain/terrain-definition.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "view/object-describer.h"
#include "world/world.h"

/*!
 * @brief 探索コマンドのメインルーチン / Simple command to "search" for one turn
 */
void do_cmd_search(CreatureEntity &creature)
{
    if (command_arg) {
        command_rep = command_arg - 1;
        RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::ACTION);
        command_arg = 0;
    }

    PlayerEnergy(creature).set_player_turn_energy(100);
    search(creature);

    if (creature.action == ACTION_SEARCH) {
        search(creature);
    }
}

static bool exe_alter(CreatureEntity &creature)
{
    const auto dir = get_rep_dir(creature, true);
    if (!dir) {
        return false;
    }

    const auto pos = creature.get_neighbor(dir);
    const auto &grid = creature.get_floor()->get_grid(pos);
    const auto &terrain = grid.get_terrain(TerrainKind::MIMIC);
    PlayerEnergy(creature).set_player_turn_energy(100);
    if (grid.has_monster()) {
        do_cmd_attack(creature, pos.y, pos.x, HISSATSU_NONE);
        return false;
    }

    if (terrain.flags.has(TerrainCharacteristics::OPEN)) {
        return exe_open(creature, pos.y, pos.x);
    }

    if (terrain.flags.has(TerrainCharacteristics::BASH)) {
        return exe_bash(creature, pos.y, pos.x, dir);
    }

    if (terrain.flags.has(TerrainCharacteristics::TUNNEL)) {
        return exe_tunnel(creature, pos.y, pos.x);
    }

    if (terrain.flags.has(TerrainCharacteristics::CLOSE)) {
        return exe_close(creature, pos);
    }

    if (terrain.flags.has(TerrainCharacteristics::DISARM)) {
        return exe_disarm(creature, pos.y, pos.x, dir);
    }

    msg_print(_("何もない空中を攻撃した。", "You attack the empty air."));
    return false;
}

/*!
 * @brief 特定のマスに影響を及ぼすための汎用的コマンド / Manipulate an adjacent grid in some way
 * @details
 */
void do_cmd_alter(CreatureEntity &creature)
{
    CreatureClass(creature).break_samurai_stance({ SamuraiStanceType::MUSOU });

    if (command_arg) {
        command_rep = command_arg - 1;
        RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::ACTION);
        command_arg = 0;
    }

    if (!exe_alter(creature)) {
        disturb(creature, false, false);
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

static void accept_winner_message(CreatureEntity &creature)
{
    if (!AngbandWorld::get_instance().total_winner || !last_words) {
        return;
    }

    play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_WINNER);
    tl::optional<std::string> buf;
    while (true) {
        buf = input_string(_("*勝利*メッセージ: ", "*Winning* message: "), 1024);
        if (!buf.has_value()) {
            continue;
        }

        if (input_check_strict(creature, _("よろしいですか？", "Are you sure? "), UserCheck::NO_HISTORY)) {
            break;
        }
    }

    if (!buf->empty()) {
        creature.last_message = buf.value();
        msg_print(creature.last_message);
    }
}

/*!
 * @brief 自殺するコマンドのメインルーチン
 * commit suicide
 * @details
 */
void do_cmd_suicide(CreatureEntity &creature)
{
    flush();
    auto &world = AngbandWorld::get_instance();
    if (world.total_winner) {
        if (!input_check_strict(creature, _("虚無りますか? ", "Do you want to go to the Nihil War? "), UserCheck::NO_HISTORY)) {
            return;
        }
    } else {
        if (!input_check(_("何もかも諦めますか? ", "Do you give up everything? "))) {
            return;
        }
    }

    if (!decide_suicide()) {
        return;
    }

    creature.last_message = "";
    creature.playing = false;
    creature.is_dead_ = true;
    creature.leaving = true;
    if (world.total_winner) {
        accept_winner_message(creature);
        world.add_retired_class(creature.pclass);
    } else {
        play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_GAMEOVER);
        const auto &floor = *creature.current_floor_ptr;
        exe_write_diary(floor, DiaryKind::DESCRIPTION, 0, _("ダンジョンの探索に飽きて自殺した。", "got tired to commit suicide."));
        exe_write_diary(floor, DiaryKind::GAMESTART, 1, _("-------- ゲームオーバー --------", "--------   Game  Over   --------"));
        exe_write_diary(floor, DiaryKind::DESCRIPTION, 1, "\n\n\n\n");
    }

    creature.died_from = _("途中終了", "Quitting");
}

/*!
 * @brief 地形に説明を書き込むコマンド / Inscribe description on terrain
 */
void do_cmd_inscribe_terrain(CreatureEntity &creature)
{
    auto &floor = *creature.current_floor_ptr;
    auto &grid = floor.get_grid(creature.get_position());

    // 現在の説明を表示
    if (!grid.terrain_description.empty()) {
        msg_format(_("現在の書き込み: %s", "Current description: %s"), grid.terrain_description.c_str());
    } else {
        msg_print(_("この地形には何も書かれていない。", "This terrain has no description."));
    }

    // input_stringを使用した文字列入力
    tl::optional<std::string> desc_opt = input_string(_("書き込み: ", "Description: "),
        78, grid.terrain_description, false);

    if (!desc_opt) {
        msg_print(_("キャンセルしました。", "Cancelled."));
        return;
    }

    grid.terrain_description = desc_opt.value();

    if (desc_opt.value().empty()) {
        msg_print(_("説明を消去した。", "Description cleared."));
    } else {
        msg_format(_("「%s」と書き込んだ。", "Inscribed '%s'."), desc_opt.value().data());
    }
}
