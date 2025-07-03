#include "cmd-action/cmd-move.h"
#include "action/action-limited.h"
#include "action/movement-execution.h"
#include "action/run-execution.h"
#include "avatar/avatar.h"
#include "cmd-io/cmd-save.h"
#include "core/asking-player.h"
#include "core/disturbance.h"
#include "core/stuff-handler.h"
#include "dungeon/quest.h"
#include "floor/floor-mode-changer.h"
#include "floor/geometry.h"
#include "floor/wild.h"
#include "game-option/birth-options.h"
#include "game-option/input-options.h"
#include "game-option/map-screen-options.h"
#include "game-option/play-record-options.h"
#include "game-option/special-options.h"
#include "info-reader/fixed-map-parser.h"
#include "io/input-key-requester.h"
#include "io/write-diary.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-ninja.h"
#include "player-base/player-class.h"
#include "player-info/samurai-data-type.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/player-move.h"
#include "player/player-status.h"
#include "player/special-defense-types.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-song.h"
#include "status/action-setter.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/floor/floor-info.h"
#include "system/floor/wilderness-grid.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/services/dungeon-service.h"
#include "system/terrain/terrain-definition.h"
#include "target/target-getter.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <algorithm>

/*!
 * @brief フロア脱出時に出戻りが不可能だった場合に警告を加える処理
 * @param down_stair TRUEならば階段を降りる処理、FALSEなら階段を昇る処理による内容
 * @return フロア移動を実際に行うならTRUE、キャンセルする場合はFALSE
 */
static bool confirm_leave_level(PlayerType *player_ptr, bool down_stair)
{
    const auto &quests = QuestList::get_instance();
    const auto &quest = quests.get_quest(player_ptr->current_floor_ptr->quest_number);

    auto caution_in_tower = any_bits(quest.flags, QUEST_FLAG_TOWER);
    caution_in_tower &= quest.status != QuestStatusType::STAGE_COMPLETED || (down_stair && (quests.get_quest(QuestId::TOWER1).status != QuestStatusType::COMPLETED));

    auto caution_in_quest = quest.type == QuestKindType::RANDOM;
    caution_in_quest |= quest.flags & QUEST_FLAG_ONCE && quest.status != QuestStatusType::COMPLETED;
    caution_in_quest |= caution_in_tower;

    if (confirm_quest && player_ptr->current_floor_ptr->is_in_quest() && caution_in_quest) {
        msg_print(_("この階を一度去ると二度と戻って来られません。", "You can't come back here once you leave this floor."));
        return input_check(_("本当にこの階を去りますか？", "Really leave this floor? "));
    }

    return true;
}

/*!
 * @brief 階段を使って階層を昇る処理 / Go up one level
 */
void do_cmd_go_up(PlayerType *player_ptr)
{
    auto &quests = QuestList::get_instance();
    auto &floor = *player_ptr->current_floor_ptr;
    const auto &grid = floor.get_grid({ player_ptr->y, player_ptr->x });
    const auto &terrain = grid.get_terrain();
    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU });

    if (terrain.flags.has_not(TerrainCharacteristics::LESS)) {
        msg_print(_("ここには上り階段が見当たらない。", "I see no up staircase here."));
        return;
    }

    if (terrain.flags.has(TerrainCharacteristics::QUEST)) {
        if (!confirm_leave_level(player_ptr, false)) {
            return;
        }

        if (is_echizen(player_ptr)) {
            msg_print(_("なんだこの階段は！", "What's this STAIRWAY!"));
        } else {
            msg_print(_("上の階に登った。", "You enter the up staircase."));
        }

        sound(SoundKind::STAIRWAY);

        leave_quest_check(player_ptr);
        floor.quest_number = i2enum<QuestId>(grid.special);
        const auto quest_id = floor.quest_number;
        auto &quest = quests.get_quest(quest_id);
        if (quest.status == QuestStatusType::UNTAKEN) {
            if (quest.type != QuestKindType::RANDOM) {
                init_flags = INIT_ASSIGN;
                parse_fixed_map(player_ptr, QUEST_DEFINITION_LIST, 0, 0, 0, 0);
            }

            quest.status = QuestStatusType::TAKEN;
        }

        if (!inside_quest(quest_id)) {
            floor.dun_level = 0;
            player_ptr->word_recall = 0;
        }

        player_ptr->leaving = true;
        player_ptr->oldpx = 0;
        player_ptr->oldpy = 0;
        PlayerEnergy(player_ptr).set_player_turn_energy(100);
        return;
    }

    auto go_up = false;
    if (!floor.is_underground()) {
        go_up = true;
    } else {
        go_up = confirm_leave_level(player_ptr, false);
    }

    if (!go_up) {
        return;
    }

    PlayerEnergy(player_ptr).set_player_turn_energy(100);

    if (autosave_l) {
        do_cmd_save_game(player_ptr, true);
    }

    const auto quest_number = floor.quest_number;
    auto &quest = quests.get_quest(quest_number);

    if (inside_quest(quest_number) && quest.type == QuestKindType::RANDOM) {
        leave_quest_check(player_ptr);
        floor.quest_number = QuestId::NONE;
    }

    auto up_num = 0;
    if (inside_quest(quest_number) && quest.type != QuestKindType::RANDOM) {
        leave_quest_check(player_ptr);
        floor.quest_number = i2enum<QuestId>(grid.special);
        floor.dun_level = 0;
        up_num = 0;
    } else {
        auto &fcms = FloorChangeModesStore::get_instace();
        fcms->set({ FloorChangeMode::SAVE_FLOORS, FloorChangeMode::UP });
        up_num = 1;
        if (terrain.flags.has(TerrainCharacteristics::SHAFT)) {
            fcms->set(FloorChangeMode::SHAFT);
            up_num *= 2;
        }

        if (floor.dun_level - up_num < floor.get_dungeon_definition().mindepth) {
            up_num = floor.dun_level;
        }
    }

    if (record_stair) {
        exe_write_diary(floor, DiaryKind::STAIR, 0 - up_num, _("階段を上った", "climbed up the stairs to"));
    }

    if (up_num == floor.dun_level) {
        if (is_echizen(player_ptr)) {
            msg_print(_("なんだこの階段は！", "What's this STAIRWAY!"));
        } else {
            msg_print(_("地上に戻った。", "You go back to the surface."));
        }
        player_ptr->word_recall = 0;
    } else {
        if (is_echizen(player_ptr)) {
            msg_print(_("なんだこの階段は！", "What's this STAIRWAY!"));
        } else {
            msg_print(_("階段を上って新たなる迷宮へと足を踏み入れた。", "You enter a maze of up staircases."));
        }
    }

    sound(SoundKind::STAIRWAY);

    player_ptr->leaving = true;
}

/*!
 * @brief 階段を使って階層を降りる処理 / Go down one level
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_go_down(PlayerType *player_ptr)
{
    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU });

    auto &floor = *player_ptr->current_floor_ptr;
    auto &grid = floor.grid_array[player_ptr->y][player_ptr->x];
    auto &terrain = grid.get_terrain();
    if (terrain.flags.has_not(TerrainCharacteristics::MORE)) {
        msg_print(_("ここには下り階段が見当たらない。", "I see no down staircase here."));
        return;
    }

    const auto is_fall_trap = terrain.flags.has(TerrainCharacteristics::TRAP);
    if (terrain.flags.has(TerrainCharacteristics::QUEST_ENTER)) {
        do_cmd_quest(player_ptr);
        return;
    }

    if (terrain.flags.has(TerrainCharacteristics::QUEST)) {
        if (!confirm_leave_level(player_ptr, true)) {
            return;
        }

        if (is_echizen(player_ptr)) {
            msg_print(_("なんだこの階段は！", "What's this STAIRWAY!"));
        } else {
            msg_print(_("下の階に降りた。", "You enter the down staircase."));
        }

        sound(SoundKind::STAIRWAY);

        leave_quest_check(player_ptr);
        leave_tower_check(player_ptr);
        floor.quest_number = i2enum<QuestId>(grid.special);

        auto &quests = QuestList::get_instance();
        auto &quest = quests.get_quest(floor.quest_number);
        if (quest.status == QuestStatusType::UNTAKEN) {
            if (quest.type != QuestKindType::RANDOM) {
                init_flags = INIT_ASSIGN;
                parse_fixed_map(player_ptr, QUEST_DEFINITION_LIST, 0, 0, 0, 0);
            }

            quest.status = QuestStatusType::TAKEN;
        }

        if (!floor.is_in_quest()) {
            floor.dun_level = 0;
            player_ptr->word_recall = 0;
        }

        player_ptr->leaving = true;
        player_ptr->oldpx = 0;
        player_ptr->oldpy = 0;
        PlayerEnergy(player_ptr).set_player_turn_energy(100);
        return;
    }

    auto dungeon_id = DungeonId::WILDERNESS;
    auto &fcms = FloorChangeModesStore::get_instace();
    if (!floor.is_underground()) {
        dungeon_id = terrain.flags.has(TerrainCharacteristics::ENTRANCE) ? i2enum<DungeonId>(grid.special) : DungeonId::ANGBAND;
        if (ironman_downward && (dungeon_id != DungeonId::ANGBAND)) {
            msg_print(_("ダンジョンの入口は塞がれている！", "The entrance of this dungeon is closed!"));
            return;
        }

        const auto mes_entrance = DungeonService::check_first_entrance(dungeon_id);
        if (mes_entrance) {
            msg_print(*mes_entrance);
            if (!input_check(_("本当にこのダンジョンに入りますか？", "Do you really get in this dungeon? "))) {
                return;
            }
        }

        player_ptr->oldpx = player_ptr->x;
        player_ptr->oldpy = player_ptr->y;
        floor.set_dungeon_index(dungeon_id);
        fcms->set(FloorChangeMode::FIRST_FLOOR);
    }

    PlayerEnergy(player_ptr).set_player_turn_energy(100);
    if (autosave_l) {
        do_cmd_save_game(player_ptr, true);
    }

    auto down_num = 0;
    if (terrain.flags.has(TerrainCharacteristics::SHAFT)) {
        down_num += 2;
    } else {
        down_num += 1;
    }

    const auto &dungeon = floor.get_dungeon_definition();
    if (!floor.is_underground()) {
        floor.enter_dungeon(true);
        down_num = dungeon.mindepth;
    }

    if (record_stair && !floor.is_in_quest()) {
        const auto note = is_fall_trap ? _("落とし戸に落ちた", "fell through a trap door") : _("階段を下りた", "climbed down the stairs to");
        exe_write_diary(floor, DiaryKind::STAIR, down_num, note);
    }

    if (is_fall_trap) {
        msg_print(_("わざと落とし戸に落ちた。", "You deliberately jump through the trap door."));
        if (floor.is_in_quest()) {
            msg_print(_("しかし何も起こらなかった。", "But, nothing happens."));
            return;
        }
    } else {
        if (dungeon_id > DungeonId::WILDERNESS) {
            msg_format(_("%sへ入った。", "You entered %s."), dungeon.text.data());
        } else {
            if (is_echizen(player_ptr)) {
                msg_print(_("なんだこの階段は！", "What's this STAIRWAY!"));
            } else {
                msg_print(_("階段を下りて新たなる迷宮へと足を踏み入れた。", "You enter a maze of down staircases."));
            }
        }

        sound(SoundKind::STAIRWAY);
    }

    player_ptr->leaving = true;
    if (is_fall_trap) {
        fcms->set({ FloorChangeMode::SAVE_FLOORS, FloorChangeMode::DOWN, FloorChangeMode::RANDOM_PLACE, FloorChangeMode::RANDOM_CONNECT });
        return;
    }

    fcms->set({ FloorChangeMode::SAVE_FLOORS, FloorChangeMode::DOWN });
    if (terrain.flags.has(TerrainCharacteristics::SHAFT)) {
        fcms->set(FloorChangeMode::SHAFT);
    }
}

/*!
 * @brief 「歩く」動作コマンドのメインルーチン /
 * Support code for the "Walk" and "Jump" commands
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pickup アイテムの自動拾いを行うならTRUE
 */
void do_cmd_walk(PlayerType *player_ptr, bool pickup)
{
    if (command_arg) {
        command_rep = command_arg - 1;
        RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::ACTION);
        command_arg = 0;
    }

    auto more = false;
    const auto is_wild_mode = AngbandWorld::get_instance().is_wild_mode();
    if (const auto dir = get_rep_dir(player_ptr)) {
        PlayerEnergy energy(player_ptr);
        energy.set_player_turn_energy(100);
        if (dir.has_direction()) {
            PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU });
        }

        if (is_wild_mode) {
            energy.mul_player_turn_energy((MAX_HGT + MAX_WID) / 2);
        }

        if (player_ptr->action == ACTION_HAYAGAKE) {
            auto energy_use = (ENERGY)(player_ptr->energy_use * (45 - (player_ptr->lev / 2)) / 100);
            energy.set_player_turn_energy(energy_use);
        }

        exe_movement(player_ptr, dir, pickup, false);
        more = true;
    }

    const auto &floor = *player_ptr->current_floor_ptr;
    const auto p_pos = player_ptr->get_position();
    if (is_wild_mode && !floor.has_terrain_characteristics(p_pos, TerrainCharacteristics::TOWN)) {
        const auto wild_level = WildernessGrids::get_instance().get_player_grid().get_level();
        auto tmp = 120 + player_ptr->lev * 10 - wild_level + 5;
        if (tmp < 1) {
            tmp = 1;
        }

        if (((wild_level + 5) > (player_ptr->lev / 2)) && randint0(tmp) < (21 - player_ptr->skill_stl)) {
            msg_print(_("襲撃だ！", "You are ambushed !"));
            player_ptr->oldpy = randint1(MAX_HGT - 2);
            player_ptr->oldpx = randint1(MAX_WID - 2);
            change_wild_mode(player_ptr, true);
            PlayerEnergy(player_ptr).set_player_turn_energy(100);
        }
    }

    if (!more) {
        disturb(player_ptr, false, false);
    }
}

/*!
 * @brief 「走る」動作コマンドのメインルーチン /
 * Start running.
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_run(PlayerType *player_ptr)
{
    if (cmd_limit_confused(player_ptr)) {
        return;
    }

    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU });

    if (const auto dir = get_rep_dir(player_ptr)) {
        player_ptr->running = (command_arg ? command_arg : 1000);
        run_step(player_ptr, dir);
    }
}

/*!
 * @brief 「留まる」動作コマンドのメインルーチン /
 * Stay still.  Search.  Enter stores.
 * Pick up treasure if "pickup" is true.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pickup アイテムの自動拾いを行うならTRUE
 */
void do_cmd_stay(PlayerType *player_ptr, bool pickup)
{
    uint32_t mpe_mode = MPE_STAYING | MPE_ENERGY_USE;
    if (command_arg) {
        command_rep = command_arg - 1;
        RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::ACTION);
        command_arg = 0;
    }

    PlayerEnergy(player_ptr).set_player_turn_energy(100);
    if (pickup) {
        mpe_mode |= MPE_DO_PICKUP;
    }

    (void)move_player_effect(player_ptr, player_ptr->y, player_ptr->x, mpe_mode);
}

/*!
 * @brief 休憩ターン数のコマンド受付
 */
static bool input_rest_turns()
{
    constexpr auto p = _("休憩 (0-9999, '*' で HP/MP全快, '&' で必要なだけ): ", "Rest (0-9999, '*' for HP/SP, '&' as needed): ");
    while (true) {
        const auto rest_turns_opt = input_string(p, 4, "&");
        if (!rest_turns_opt.has_value()) {
            return false;
        }

        const auto &rest_turns = rest_turns_opt.value();
        if (rest_turns.starts_with('&')) {
            command_arg = COMMAND_ARG_REST_UNTIL_DONE;
            return true;
        }

        if (rest_turns.starts_with('*')) {
            command_arg = COMMAND_ARG_REST_FULL_HEALING;
            return true;
        }

        try {
            command_arg = static_cast<short>(std::clamp(std::stoi(rest_turns), 0, 9999));
            return true;
        } catch (std::invalid_argument const &) {
            msg_print(_("数値を入力して下さい。", "Please input numeric value."));
        }
    }
}

/*!
 * @brief 「休む」動作コマンドのメインルーチン /
 * Resting allows a player to safely restore his hp	-RAK-
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_rest(PlayerType *player_ptr)
{
    set_action(player_ptr, ACTION_NONE);
    if (PlayerClass(player_ptr).equals(PlayerClassType::BARD)) {
        auto is_singing = get_singing_song_effect(player_ptr) != 0;
        is_singing |= get_interrupting_song_effect(player_ptr) != 0;
        if (is_singing) {
            stop_singing(player_ptr);
        }
    }

    SpellHex spell_hex(player_ptr);
    if (spell_hex.is_spelling_any()) {
        (void)spell_hex.stop_all_spells();
    }

    if (!input_rest_turns()) {
        return;
    }

    set_superstealth(player_ptr, false);
    PlayerEnergy(player_ptr).set_player_turn_energy(100);
    if (command_arg > 100) {
        chg_virtue(player_ptr, Virtue::DILIGENCE, -1);
    }

    if (player_ptr->is_fully_healthy()) {
        chg_virtue(player_ptr, Virtue::DILIGENCE, -1);
    }

    player_ptr->resting = command_arg;
    player_ptr->action = ACTION_REST;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    rfu.set_flag(MainWindowRedrawingFlag::ACTION);
    handle_stuff(player_ptr);
    term_fresh();
}
