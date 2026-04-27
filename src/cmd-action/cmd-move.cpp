#include "cmd-action/cmd-move.h"
#include "action/action-limited.h"
#include "action/movement-execution.h"
#include "action/run-execution.h"
#include "avatar/avatar.h"
#include "cmd-io/cmd-save.h"
#include "core/asking-player.h"
#include "core/disturbance.h"
#include "core/stuff-handler.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/quest.h"
#include "floor/floor-mode-changer.h"
#include "floor/geometry.h"
#include "floor/wild.h"
#include "game-option/birth-options.h"
#include "game-option/input-options.h"
#include "game-option/map-screen-options.h"
#include "game-option/play-record-options.h"
#include "game-option/special-options.h"
#include "grid/grid.h"
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
#include "player/special-defense-types.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-song.h"
#include "status/action-setter.h"
#include "system/creature-entity.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
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
static bool confirm_leave_level(CreatureEntity &creature, bool down_stair)
{
    const auto &quests = QuestList::get_instance();
    const auto &quest = quests.get_quest(creature.get_floor()->quest_number);

    auto caution_in_tower = any_bits(quest.flags, QUEST_FLAG_TOWER);
    caution_in_tower &= quest.status != QuestStatusType::STAGE_COMPLETED || (down_stair && (quests.get_quest(QuestId::TOWER1).status != QuestStatusType::COMPLETED));

    auto caution_in_quest = quest.type == QuestKindType::RANDOM;
    caution_in_quest |= quest.flags & QUEST_FLAG_ONCE && quest.status != QuestStatusType::COMPLETED;
    caution_in_quest |= caution_in_tower;

    if (confirm_quest && creature.get_floor()->is_in_quest() && caution_in_quest) {
        msg_print(_("この階を一度去ると二度と戻って来られません。", "You can't come back here once you leave this floor."));
        return input_check(_("本当にこの階を去りますか？", "Really leave this floor? "));
    }

    return true;
}

/*!
 * @brief 階段を使って階層を昇る処理 / Go up one level
 */
void do_cmd_go_up(CreatureEntity &creature)
{
    auto &quests = QuestList::get_instance();
    auto &floor = *creature.get_floor();
    const auto &grid = floor.get_grid({ creature.y, creature.x });
    const auto &terrain = grid.get_terrain();
    CreatureClass(creature).break_samurai_stance({ SamuraiStanceType::MUSOU });

    if (terrain.flags.has(TerrainCharacteristics::PORTAL)) {
        do_cmd_go_portal(creature);
        return;
    }

    if (terrain.flags.has_not(TerrainCharacteristics::UP_STAIRS)) {
        msg_print(_("ここには上り階段が見当たらない。", "I see no up staircase here."));
        return;
    }

    if (terrain.flags.has(TerrainCharacteristics::QUEST)) {
        if (!confirm_leave_level(creature, false)) {
            return;
        }

        if (creature.is_echizen()) {
            msg_print(_("なんだこの階段は！", "What's this STAIRWAY!"));
        } else {
            msg_print(_("上の階に登った。", "You enter the up staircase."));
        }

        sound(SoundKind::STAIRWAY);

        leave_quest_check(creature);
        floor.quest_number = i2enum<QuestId>(grid.special);
        const auto quest_id = floor.quest_number;
        auto &quest = quests.get_quest(quest_id);
        if (quest.status == QuestStatusType::UNTAKEN) {
            if (quest.type != QuestKindType::RANDOM) {
                init_flags = INIT_ASSIGN;
                parse_fixed_map(creature, QUEST_DEFINITION_LIST, 0, 0, 0, 0);
            }

            quest.status = QuestStatusType::TAKEN;
        }

        if (!inside_quest(quest_id)) {
            floor.dun_level = 0;
            creature.set_timed_effect(CreatureTimedEffect::WORD_RECALL, 0);
        }

        creature.leaving = true;
        creature.oldpx = 0;
        creature.oldpy = 0;
        PlayerEnergy(creature).set_player_turn_energy(100);
        return;
    }

    auto go_up = false;
    if (!floor.is_underground()) {
        go_up = true;
    } else {
        go_up = confirm_leave_level(creature, false);
    }

    if (!go_up) {
        return;
    }

    PlayerEnergy(creature).set_player_turn_energy(100);

    if (autosave_l) {
        do_cmd_save_game(creature, true);
    }

    const auto quest_number = floor.quest_number;
    auto &quest = quests.get_quest(quest_number);

    if (inside_quest(quest_number) && quest.type == QuestKindType::RANDOM) {
        leave_quest_check(creature);
        floor.quest_number = QuestId::NONE;
    }

    auto up_num = 0;
    if (inside_quest(quest_number) && quest.type != QuestKindType::RANDOM) {
        leave_quest_check(creature);
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

    // 一度利用した階段を消滅させる処理（移動前のフロア）
    const auto &dungeon = floor.get_dungeon_definition();
    if (dungeon.flags.has(DungeonFeatureType::VANISH_STAIRS) && floor.is_underground()) {
        const auto p_pos = creature.get_position();
        const auto floor_terrain_id = dungeon.select_floor_terrain_id();
        set_terrain_id_to_grid(creature, p_pos, floor_terrain_id);
        creature.vanish_stairs_flag = true; // 移動後のフロアでも階段を消す
    }

    if (up_num == floor.dun_level) {
        if (creature.is_echizen()) {
            msg_print(_("なんだこの階段は！", "What's this STAIRWAY!"));
        } else {
            msg_print(_("地上に戻った。", "You go back to the surface."));
        }
        creature.set_timed_effect(CreatureTimedEffect::WORD_RECALL, 0);
    } else {
        if (creature.is_echizen()) {
            msg_print(_("なんだこの階段は！", "What's this STAIRWAY!"));
        } else {
            msg_print(_("階段を上って新たなる迷宮へと足を踏み入れた。", "You enter a maze of up staircases."));
        }
    }

    sound(SoundKind::STAIRWAY);

    creature.leaving = true;
}

/*!
 * @brief 階段を使って階層を降りる処理 / Go down one level
 * @param creature クリーチャーへの参照
 */
void do_cmd_go_down(CreatureEntity &creature)
{
    CreatureClass(creature).break_samurai_stance({ SamuraiStanceType::MUSOU });

    auto &floor = *creature.get_floor();
    auto &grid = floor.grid_array[creature.y][creature.x];
    auto &terrain = grid.get_terrain();

    if (terrain.flags.has(TerrainCharacteristics::PORTAL)) {
        do_cmd_go_portal(creature);
        return;
    }

    if (terrain.flags.has_not(TerrainCharacteristics::DOWN_STAIRS)) {
        msg_print(_("ここには下り階段が見当たらない。", "I see no down staircase here."));
        return;
    }

    const auto is_fall_trap = terrain.flags.has(TerrainCharacteristics::TRAP);
    if (terrain.flags.has(TerrainCharacteristics::QUEST_ENTER)) {
        do_cmd_quest(creature);
        return;
    }

    if (terrain.flags.has(TerrainCharacteristics::QUEST)) {
        if (!confirm_leave_level(creature, true)) {
            return;
        }

        if (creature.is_echizen()) {
            msg_print(_("なんだこの階段は！", "What's this STAIRWAY!"));
        } else {
            msg_print(_("下の階に降りた。", "You enter the down staircase."));
        }

        sound(SoundKind::STAIRWAY);

        leave_quest_check(creature);
        leave_tower_check(creature);
        floor.quest_number = i2enum<QuestId>(grid.special);

        auto &quests = QuestList::get_instance();
        auto &quest = quests.get_quest(floor.quest_number);
        if (quest.status == QuestStatusType::UNTAKEN) {
            if (quest.type != QuestKindType::RANDOM) {
                init_flags = INIT_ASSIGN;
                parse_fixed_map(creature, QUEST_DEFINITION_LIST, 0, 0, 0, 0);
            }

            quest.status = QuestStatusType::TAKEN;
        }

        if (!floor.is_in_quest()) {
            floor.dun_level = 0;
            creature.set_timed_effect(CreatureTimedEffect::WORD_RECALL, 0);
        }

        creature.leaving = true;
        creature.oldpx = 0;
        creature.oldpy = 0;
        PlayerEnergy(creature).set_player_turn_energy(100);
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

        auto dungeon = DungeonList::get_instance().get_dungeon(dungeon_id);
        if (dungeon.min_plev > creature.level) {
            msg_print(_("あなたは弾き返された。このダンジョンに入るだけの力が備わっていないようだ。", "You are repelled. You lack the strength to enter this dungeon."));
            return;
        }

        const auto mes_entrance = DungeonService::check_first_entrance(dungeon_id);
        if (mes_entrance) {
            msg_print(*mes_entrance);
            if (!input_check(_("本当にこのダンジョンに入りますか？", "Do you really get in this dungeon? "))) {
                return;
            }
        }

        creature.oldpx = creature.x;
        creature.oldpy = creature.y;
        floor.set_dungeon_index(dungeon_id);
        fcms->set(FloorChangeMode::FIRST_FLOOR);
    }

    PlayerEnergy(creature).set_player_turn_energy(100);
    if (autosave_l) {
        do_cmd_save_game(creature, true);
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

        // ダンジョンに入った回数を記録
        creature.plus_incident_tree("ENTER_DUNGEON", 1);
        if (!dungeon.tag.empty()) {
            const auto dungeon_key = format("ENTER_DUNGEON/%s", dungeon.tag.c_str());
            creature.plus_incident_tree(dungeon_key.data(), 1);
        }
    }

    if (record_stair && !floor.is_in_quest()) {
        const auto note = is_fall_trap ? _("落とし戸に落ちた", "fell through a trap door") : _("階段を下りた", "climbed down the stairs to");
        exe_write_diary(floor, DiaryKind::STAIR, down_num, note);
    }

    // 一度利用した階段を消滅させる処理（移動前のフロア）
    if (!is_fall_trap && dungeon.flags.has(DungeonFeatureType::VANISH_STAIRS) && floor.is_underground()) {
        const auto p_pos = creature.get_position();
        const auto floor_terrain_id = dungeon.select_floor_terrain_id();
        set_terrain_id_to_grid(creature, p_pos, floor_terrain_id);
        creature.vanish_stairs_flag = true; // 移動後のフロアでも階段を消す
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
            if (creature.is_echizen()) {
                msg_print(_("なんだこの階段は！", "What's this STAIRWAY!"));
            } else {
                msg_print(_("階段を下りて新たなる迷宮へと足を踏み入れた。", "You enter a maze of down staircases."));
            }
        }

        sound(SoundKind::STAIRWAY);
    }

    creature.leaving = true;
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
 * @param creature クリーチャーへの参照
 * @param pickup アイテムの自動拾いを行うならTRUE
 */
void do_cmd_walk(CreatureEntity &creature, bool pickup)
{
    if (command_arg) {
        command_rep = command_arg - 1;
        RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::ACTION);
        command_arg = 0;
    }

    auto more = false;
    const auto is_wild_mode = AngbandWorld::get_instance().is_wild_mode();
    if (const auto dir = get_rep_dir(creature)) {
        PlayerEnergy energy(creature);
        energy.set_player_turn_energy(100);
        if (dir.has_direction()) {
            CreatureClass(creature).break_samurai_stance({ SamuraiStanceType::MUSOU });
        }

        if (is_wild_mode) {
            energy.mul_player_turn_energy((MAX_HGT + MAX_WID) / 2);
        }

        if (creature.action == ACTION_HAYAGAKE) {
            auto energy_use = (ENERGY)(creature.energy_use * (45 - (creature.level / 2)) / 100);
            energy.set_player_turn_energy(energy_use);
        }

        exe_movement(creature, dir, pickup, false);
        more = true;
    }

    const auto &floor = *creature.get_floor();
    const auto p_pos = creature.get_position();
    if (is_wild_mode && !floor.has_terrain_characteristics(p_pos, TerrainCharacteristics::TOWN)) {
        const auto wild_level = WildernessGrids::get_instance().get_player_grid().get_level();
        auto tmp = 120 + creature.level * 10 - wild_level + 5;
        if (tmp < 1) {
            tmp = 1;
        }

        if (((wild_level + 5) > (creature.level / 2)) && randint0(tmp) < (21 - creature.get_skill_stealth())) {
            // TODO: 広域マップの領域ごとのアライアンス情報を取得する機能が未実装のため、
            // 今回はデフォルトメッセージを使用。将来的にはアライアンス固有のメッセージを表示予定
            msg_print(_("襲撃だ！", "You are ambushed !"));
            creature.oldpy = randint1(MAX_HGT - 2);
            creature.oldpx = randint1(MAX_WID - 2);
            change_wild_mode(creature, true);
            PlayerEnergy(creature).set_player_turn_energy(100);
        }
    }

    if (!more) {
        disturb(creature, false, false);
    }
}

/*!
 * @brief 「走る」動作コマンドのメインルーチン /
 * Start running.
 * @param creature クリーチャーへの参照
 */
void do_cmd_run(CreatureEntity &creature)
{
    if (cmd_limit_confused(creature)) {
        return;
    }

    CreatureClass(creature).break_samurai_stance({ SamuraiStanceType::MUSOU });

    if (const auto dir = get_rep_dir(creature)) {
        creature.running = (command_arg ? command_arg : 1000);
        run_step(creature, dir);
    }
}

/*!
 * @brief 「留まる」動作コマンドのメインルーチン /
 * Stay still.  Search.  Enter stores.
 * Pick up treasure if "pickup" is true.
 * @param creature クリーチャーへの参照
 * @param pickup アイテムの自動拾いを行うならTRUE
 */
void do_cmd_stay(CreatureEntity &creature, bool pickup)
{
    uint32_t mpe_mode = MPE_STAYING | MPE_ENERGY_USE;
    if (command_arg) {
        command_rep = command_arg - 1;
        RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::ACTION);
        command_arg = 0;
    }

    PlayerEnergy(creature).set_player_turn_energy(100);
    if (pickup) {
        mpe_mode |= MPE_DO_PICKUP;
    }

    (void)move_player_effect(creature, creature.y, creature.x, mpe_mode);
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
 * Resting allows a creature to safely restore his hp	-RAK-
 * @param creature クリーチャーへの参照
 */
void do_cmd_rest(CreatureEntity &creature)
{
    set_action(creature, ACTION_NONE);
    if (CreatureClass(creature).equals(PlayerClassType::BARD)) {
        auto is_singing = get_singing_song_effect(creature) != 0;
        is_singing |= get_interrupting_song_effect(creature) != 0;
        if (is_singing) {
            stop_singing(creature);
        }
    }

    SpellHex spell_hex(creature);
    if (spell_hex.is_spelling_any()) {
        (void)spell_hex.stop_all_spells();
    }

    if (!input_rest_turns()) {
        return;
    }

    set_superstealth(creature, false);
    PlayerEnergy(creature).set_player_turn_energy(100);
    if (command_arg > 100) {
        chg_virtue(creature, Virtue::DILIGENCE, -1);
    }

    if (creature.is_fully_healthy()) {
        chg_virtue(creature, Virtue::DILIGENCE, -1);
    }

    creature.plus_incident_tree("REST", 1);
    creature.resting = command_arg;
    creature.action = ACTION_REST;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    rfu.set_flag(MainWindowRedrawingFlag::ACTION);
    handle_stuff(creature);
    term_fresh();
}

/*!
 * @brief ポータルを使って他ダンジョンの同階層に移動する処理
 * @param creature クリーチャーへの参照
 */
void do_cmd_go_portal(CreatureEntity &creature)
{
    auto &floor = *creature.get_floor();
    const auto &grid = floor.get_grid({ creature.y, creature.x });
    const auto &terrain = grid.get_terrain();

    if (terrain.flags.has_not(TerrainCharacteristics::PORTAL)) {
        msg_print(_("ここにはポータルが見当たらない。", "I see no portal here."));
        return;
    }

    CreatureClass(creature).break_samurai_stance({ SamuraiStanceType::MUSOU });

    if (!confirm_leave_level(creature, false)) {
        return;
    }

    msg_print(_("ポータルに足を踏み入れた。", "You step into the portal."));
    sound(SoundKind::STAIRWAY);

    // 現在のダンジョンと階層を記録
    const auto current_dungeon = floor.dungeon_id;
    const auto current_level = floor.dun_level;

    // ランダムで他のダンジョンを選択
    const auto &dungeons = DungeonList::get_instance();
    std::vector<DungeonId> available_dungeons;

    for (auto d_idx = DungeonId::ANGBAND; d_idx < DungeonId::MAX; d_idx = static_cast<DungeonId>(static_cast<int>(d_idx) + 1)) {
        if (d_idx == current_dungeon || d_idx == DungeonId::WILDERNESS) {
            continue;
        }

        const auto &dungeon = dungeons.get_dungeon(d_idx);
        // 現在の階層がダンジョンの範囲内かチェック
        if (current_level <= dungeon.maxdepth) {
            available_dungeons.push_back(d_idx);
        }
    }

    if (available_dungeons.empty()) {
        msg_print(_("ポータルは反応しなかった。", "The portal does not respond."));
        return;
    }

    // ランダムに選択
    const auto target_dungeon = available_dungeons[randint0(available_dungeons.size())];

    // エネルギー消費
    PlayerEnergy(creature).set_player_turn_energy(100);

    // オートセーブ
    if (autosave_l) {
        do_cmd_save_game(creature, true);
    }

    // 階層移動処理
    creature.leaving = true;
    floor.set_dungeon_index(target_dungeon);

    const auto &target_dungeon_info = dungeons.get_dungeon(target_dungeon);
    msg_format(_("%sに転移した！", "You are transferred to %s!"), target_dungeon_info.name.data());
}
