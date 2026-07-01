/*!
 * @brief ゲームプレイのメインルーチン
 * @date 2020/05/10
 * @author Hourier
 * @details
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 * 2013 Deskull rearranged comment for Doxygen.
 */

#include "core/game-play.h"
#include "autopick/autopick-pref-processor.h"
#include "birth/character-builder.h"
#include "birth/inventory-initializer.h"
#include "cmd-io/cmd-gameoption.h"
#include "core/asking-player.h"
#include "core/game-closer.h"
#include "core/player-processor.h"
#include "core/score-util.h"
#include "core/scores.h"
#include "core/speed-table.h"
#include "core/stuff-handler.h"
#include "core/visuals-reseter.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon-processor.h"
#include "dungeon/quest.h"
#include "flavor/object-flavor.h"
#include "floor/floor-changer.h"
#include "floor/floor-leaver.h"
#include "floor/floor-mode-changer.h"
#include "floor/floor-save.h"
#include "floor/floor-util.h"
#include "game-option/cheat-options.h"
#include "game-option/input-options.h"
#include "game-option/play-record-options.h"
#include "game-option/runtime-arguments.h"
#include "info-reader/fixed-map-parser.h"
#include "io/files-util.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-processor.h"
#include "io/read-pref-file.h"
#include "io/record-play-movie.h"
#include "io/screen-util.h"
#include "io/signal-handlers.h"
#include "io/write-diary.h"
#include "item-info/flavor-initializer.h"
#include "load/load.h"
#include "main/sound-of-music.h"
#include "market/arena-entry.h"
#include "market/bounty.h"
#include "market/building-initializer.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/monster-remover.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-util.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/class-info.h"
#include "player-info/race-types.h"
#include "player/player-personality-types.h"
#include "player/player-skill.h"
#include "player/player-status.h"
#include "player/process-name.h"
#include "racial/racial-android.h"
#include "save/save.h"
#include "spell/spells-status.h"
#include "spell/technic-info-table.h"
#include "status/buff-setter.h"
#include "store/home.h"
#include "store/store-util.h"
#include "store/store.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/angband-system.h"
#include "system/angband-version.h"
#include "system/creature-entity.h"
#include "system/dungeon/quest-definition.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/floor/wilderness-grid.h"
#include "system/gamevalue.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/redrawing-flags-updater.h"
#include "system/system-variables.h"
#include "target/target-checker.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "util/angband-files.h"
#include "view/display-messages.h"
#include "view/display-player.h"
#include "window/main-window-util.h"
#include "wizard/wizard-special-process.h"
#include "world/world-collapsion.h"
#include "world/world.h"
#include <ctime>

static void restore_windows(CreatureEntity &creature)
{
    creature.set_hack_mutation(false);
    AngbandWorld::get_instance().character_icky_depth = 1;
    term_activate(angband_terms[0]);
    angband_terms[0]->resize_hook = resize_map;
    for (auto i = 1U; i < angband_terms.size(); ++i) {
        if (angband_terms[i]) {
            angband_terms[i]->resize_hook = redraw_window;
        }
    }

    term_set_cursor(false);
}

static void send_waiting_record(CreatureEntity &creature)
{
    auto &system = AngbandSystem::get_instance();
    if (!system.is_awaiting_report_status()) {
        return;
    }

    if (!input_check_strict(creature, _("待機していたスコア登録を今行ないますか？", "Do you register score now? "), UserCheck::NO_HISTORY)) {
        quit("");
    }

    static constexpr auto flags = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::HP,
        StatusRecalculatingFlag::MP,
        StatusRecalculatingFlag::SPELLS,
    };
    RedrawingFlagsUpdater::get_instance().set_flags(flags);
    update_creature(creature);
    creature.is_dead_ = true;
    auto &world = AngbandWorld::get_instance();
    world.play_time.pause();
    signals_ignore_tstp();
    world.character_icky_depth = 1;
    const auto path = path_build(ANGBAND_DIR_APEX, "scores.raw");
    highscore_fd = fd_open(path, O_RDWR);

    /* 町名消失バグ対策(#38205)のためここで世界マップ情報を読み出す */
    const auto &area = WildernessGrids::get_instance().get_area();
    parse_fixed_map(creature, WILDERNESS_DEFINITION, 0, 0, area.height(), area.width());
    bool success = send_world_score(creature, true);
    if (!success && !input_check_strict(creature, _("スコア登録を諦めますか？", "Do you give up score registration? "), UserCheck::NO_HISTORY)) {
        prt(_("引き続き待機します。", "standing by for future registration..."), 0, 0);
        (void)inkey();
    } else {
        system.set_awaiting_report_score(false);
        top_twenty(creature);
        if (!save_player(creature, SaveType::CLOSE_GAME)) {
            msg_print(_("セーブ失敗！", "death save failed!"));
        }
    }

    (void)fd_close(highscore_fd);
    highscore_fd = -1;
    signals_handle_tstp();
    quit("");
}

static void init_random_seed(CreatureEntity &creature, bool new_game)
{
    auto &world = AngbandWorld::get_instance();
    auto init_random_seed = false;
    if (!world.character_loaded) {
        new_game = true;
        world.character_dungeon = false;
        init_random_seed = true;
        init_saved_floors(false);
    } else if (new_game) {
        init_saved_floors(true);
    }

    if (!new_game) {
        process_player_name(creature);
    }

    if (init_random_seed) {
        Rand_state_init();
    }
}

static void init_world_floor_info(CreatureEntity &creature, std::optional<QuestId> initial_quest_id = std::nullopt)
{
    AngbandWorld::get_instance().character_dungeon = false;
    wc_ptr->collapse_degree = 0;
    auto &floor = *creature.get_floor();
    floor.reset_dungeon_index();
    floor.dun_level = 0;
    floor.quest_number = QuestId::NONE;
    floor.inside_arena = false;
    AngbandSystem::get_instance().set_phase_out(false);
    write_level = true;
    auto &system = AngbandSystem::get_instance();
    system.set_seed_flavor(randint0(0x10000000));
    system.set_seed_town(randint0(0x10000000));
    player_birth(creature, initial_quest_id);
    counts_write(creature, 2, 0);
    creature.count = 0;
    load = false;
    determine_bounty_uniques(creature);
    determine_daily_bounty(creature);
    wipe_o_list(floor);
}

/*!
 * @brief フロア情報をゲームロード時に復帰
 * @todo 3.0.Xで削除予定
 * 1.0.9 以前はセーブ前に creature.get_riding() = -1 としていたので、再設定が必要だった。
 * もう不要だが、以前のセーブファイルとの互換のために残しておく。
 */
static void restore_world_floor_info(CreatureEntity &creature)
{
    write_level = false;
    constexpr auto mes = _("                            ----ゲーム再開----", "                            --- Restarted Game ---");
    const auto &floor = *creature.get_floor();
    exe_write_diary(floor, DiaryKind::GAMESTART, 1, mes);

    if (creature.get_riding() != -1) {
        return;
    }

    creature.ride_monster(0);
    for (short i = floor.m_max; i > 0; i--) {
        const auto &monster = floor.get_monster(i);
        if (creature.is_located_at({ monster.y, monster.x })) {
            creature.ride_monster(i);
            break;
        }
    }
}

static void reset_world_info(CreatureEntity &creature)
{
    auto &world = AngbandWorld::get_instance();
    world.creating_savefile = false;
    creature.set_teleport_town(false);
    creature.set_sutemi(false);
    world.timewalk_m_idx = 0;
    creature.set_now_damaged(false);
    now_message = 0;
    record_item_name.clear();
}

static void generate_wilderness(CreatureEntity &creature)
{
    const auto &area = WildernessGrids::get_instance().get_area();
    parse_fixed_map(creature, WILDERNESS_DEFINITION, 0, 0, area.height(), area.width());
    init_flags = INIT_ONLY_BUILDINGS;
    parse_fixed_map(creature, TOWN_DEFINITION_LIST, 0, 0, MAX_HGT, MAX_WID);
    select_floor_music(creature);
}

static void change_floor_if_error(CreatureEntity &creature)
{
    if (!AngbandWorld::get_instance().character_dungeon) {
        change_floor(creature);
        return;
    }

    auto &system = AngbandSystem::get_instance();
    if (!system.is_panic_save_executed()) {
        return;
    }

    if (!creature.y || !creature.x) {
        msg_print(_("プレイヤーの位置がおかしい。フロアを再生成します。", "What a strange creature location, regenerate the dungeon floor."));
        change_floor(creature);
    }

    if (!creature.y || !creature.x) {
        creature.y = creature.x = 10;
    }

    system.set_panic_save(false);
}

static void generate_world(CreatureEntity &creature, bool new_game)
{
    reset_world_info(creature);
    const auto &floor = *creature.get_floor();
    panel_row_min = floor.height;
    panel_col_min = floor.width;

    initialize_items_flavor();
    prt(_("お待ち下さい...", "Please wait..."), 0, 0);
    term_fresh();
    generate_wilderness(creature);
    change_floor_if_error(creature);
    auto &world = AngbandWorld::get_instance();
    world.character_generated = true;
    world.character_icky_depth = 0;
    if (!new_game) {
        return;
    }

    const auto mes = format(_("%%sに降り立った。", "arrived in %%s."), map_name(creature).data());
    exe_write_diary(floor, DiaryKind::DESCRIPTION, 0, mes);
}

static void init_io(CreatureEntity &creature)
{
    term_xtra(TERM_XTRA_REACT, 0);
    RedrawingFlagsUpdater::get_instance().fill_up_sub_flags();
    handle_stuff(creature);
    if (arg_force_original) {
        rogue_like_commands = false;
    }

    if (arg_force_roguelike) {
        rogue_like_commands = true;
    }
}

static void init_riding_pet(CreatureEntity &creature, bool new_game)
{
    CreatureClass pc(creature);
    if (!new_game || !pc.is_tamer()) {
        return;
    }

    const auto pet_id = pc.equals(PlayerClassType::CAVALRY) ? MonraceId::HORSE : MonraceId::YASE_HORSE;
    const auto &monrace = MonraceList::get_instance().get_monrace(pet_id);
    const auto m_idx = place_specific_monster(creature, creature.y, creature.x - 1, pet_id, (PM_FORCE_PET | PM_NO_KAGE));
    auto &monster = creature.get_floor()->get_monster(*m_idx);
    monster.speed = monrace.speed;
    monster.maxhp = monrace.hit_dice.floored_expected_value();
    monster.max_maxhp = monster.maxhp;
    monster.hp = monrace.hit_dice.floored_expected_value();
    monster.set_dealt_damage(0);
    monster.energy_need = ENERGY_NEED() + ENERGY_NEED();
}

static void decide_arena_death(CreatureEntity &creature)
{
    if (!creature.is_playing() || !creature.is_dead()) {
        return;
    }

    auto &world = AngbandWorld::get_instance();
    auto &floor = *creature.get_floor();
    if (!floor.inside_arena) {

        while (true) {
            char i;

            if (!input_check(_("復活せずに何もかも諦めますか? ", "Do you give up everything without resurrection?? "))) {
                cheat_death(creature, cheat_live);
                return;
            }

            /* Special Verification for suicide */
            prt(_("確認のため '@' を押して下さい。", "Please verify SUICIDE by typing the '@' sign: "), 0, 0);

            flush();
            i = inkey();
            prt("", 0, 0);
            if (i == '@') {
                break;
            }
        }
    }

    floor.inside_arena = false;
    auto &entries = ArenaEntryList::get_instance();
    if (entries.is_player_true_victor()) {
        entries.increment_entry();
    } else {
        entries.set_defeated_entry();
    }
    creature.set_playing(false);
    creature.is_dead_ = true;
    creature.set_leaving(true);

    world.set_arena(true);
    reset_tim_flags(creature);
    FloorChangeModesStore::get_instace()->set({ FloorChangeMode::SAVE_FLOORS, FloorChangeMode::RANDOM_CONNECT });
    leave_floor(creature);
}

static void process_game_turn(CreatureEntity &creature)
{
    auto load_game = true;
    auto &floor = *creature.get_floor();
    auto &world = AngbandWorld::get_instance();
    world.play_time.unpause();
    while (true) {
        process_dungeon(creature, load_game);
        world.character_xtra = true;
        handle_stuff(creature);
        world.character_xtra = false;
        Target::clear_last_target();
        health_track(creature, 0);
        floor.forget_lite();
        floor.forget_view();
        floor.forget_mon_lite();
        if (!creature.is_playing() && !creature.is_dead()) {
            break;
        }

        wipe_o_list(floor);
        if (!creature.is_dead()) {
            wipe_monsters_list(creature);
        }

        msg_erase();
        load_game = false;
        decide_arena_death(creature);
        if (creature.is_dead() || wc_ptr->is_blown_away()) {
            break;
        }
        change_floor(creature);
    }
}

/*!
 * @brief 1ゲームプレイの主要ルーチン / Actually play a game
 * @param creature クリーチャーへの参照
 * @param new_game 新規にゲームを始めたかどうか
 * @param browsing_movie ムービーモードか
 * @note
 * If the "new_game" parameter is true, then, after loading the
 * savefile, we will commit suicide, if necessary, to allow the
 * creature to start a new game.
 */
void play_game(CreatureEntity &creature, bool new_game, bool browsing_movie, std::optional<QuestId> initial_quest_id)
{
    if (browsing_movie) {
        reset_visuals(creature);
        browse_movie();
        return;
    }

    restore_windows(creature);
    if (!load_savedata(creature, &new_game)) {
        quit(_("セーブファイルが壊れています", "broken savefile"));
    }

    extract_option_vars();
    send_waiting_record(creature);
    AngbandWorld::get_instance().creating_savefile = new_game;
    init_random_seed(creature, new_game);
    if (new_game) {
        init_world_floor_info(creature, initial_quest_id);
    } else {
        restore_world_floor_info(creature);
    }

    // クエスト開始時は、マップサイズを事前に取得する
    auto &floor = *creature.get_floor();
    if (new_game && floor.is_in_quest()) {
        init_flags = INIT_GET_SIZE;
        parse_fixed_map(creature, QUEST_DEFINITION_LIST, 0, 0, 0, 0);
        // サイズが取得できなかった場合はデフォルト値を設定
        if (floor.height == 0 || floor.width == 0) {
            floor.height = MAX_HGT;
            floor.width = MAX_WID;
        }
    }

    generate_world(creature, new_game);
    creature.set_playing(true);
    reset_visuals(creature);
    load_all_pref_files(creature);
    if (new_game) {
        player_outfit(creature);
    }

    init_io(creature);
    if (creature.hp < 0 && !cheat_immortal) {
        creature.is_dead_ = true;
    }

    if (CreatureRace(&creature).equals(PlayerRaceType::ANDROID)) {
        calc_android_exp(creature);
    }

    init_riding_pet(creature, new_game);
    (void)combine_and_reorder_home(creature, StoreSaleType::HOME);
    (void)combine_and_reorder_home(creature, StoreSaleType::MUSEUM);
    select_floor_music(creature);
    process_game_turn(creature);
    close_game(creature);
    quit("");
}
