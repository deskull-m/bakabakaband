/*!
 * @brief セーブファイル読み込み処理 / Purpose: support for loading savefiles -BEN-
 * @date 2014/07/07
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

#include "load/load.h"
#include "core/asking-player.h"
#include "dungeon/quest.h"
#include "game-option/birth-options.h"
#include "io/files-util.h"
#include "io/report.h"
#include "io/uid-checker.h"
#include "load/dummy-loader.h"
#include "load/dungeon-loader.h"
#include "load/extra-loader.h"
#include "load/info-loader.h"
#include "load/inventory-loader.h"
#include "load/item/item-loader-factory.h"
#include "load/load-util.h"
#include "load/load-zangband.h"
#include "load/lore-loader.h"
#include "load/old/item-loader-savefile50.h"
#include "load/old/load-v1-5-0.h"
#include "load/old/load-v1-7-0.h"
#include "load/option-loader.h"
#include "load/player-info-loader.h"
#include "load/quest-loader.h"
#include "load/store-loader.h"
#include "load/world-loader.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "player/player-personality.h"
#include "player/player-sex.h"
#include "player/race-info-table.h"
#include "system/angband-exceptions.h"
#include "system/angband-system.h"
#include "system/angband-version.h"
#include "system/player-type-definition.h"
#include "system/system-variables.h"
#include "util/angband-files.h"
#include "util/enum-converter.h"
#include "view/display-messages.h"
#include "world/world-collapsion.h"
#include "world/world.h"
#include <sstream>
#include <string>
#include <vector>

/*!
 * @brief 変愚蛮怒 v2.1.3で追加された街とクエストについて読み込む
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return エラーコード
 * @details 旧海底都市クエスト (クエストNo.18)は廃止済
 */
static errr load_town_quest(PlayerType *player_ptr)
{
    auto load_town_result = load_town();
    if (load_town_result != 0) {
        return load_town_result;
    }

    auto [max_quests_load, max_rquests_load] = load_quest_info();
    analyze_quests(player_ptr, max_quests_load, max_rquests_load);

    load_wilderness_info(player_ptr);
    return analyze_wilderness();
}

/*!
 * @brief 合計のプレイ時間をロードする
 */
static void rd_total_play_time()
{
    if (loading_savefile_version_is_older_than(4)) {
        return;
    }

    AngbandWorld::get_instance().sf_play_time = rd_u32b();
}

/*!
 * @brief 世界状況をロードする
 */
static void rd_world_info()
{
    if (loading_savefile_version_is_older_than(9)) {
        wc_ptr->collapse_degree = 0;
        return;
    }

    wc_ptr->collapse_degree = rd_u32b();
}

/*!
 * @brief 勝利した職業フラグをロードする
 */
static void rd_winner_class()
{
    if (loading_savefile_version_is_older_than(4)) {
        return;
    }

    rd_FlagGroup(AngbandWorld::get_instance().sf_winner, rd_byte);
    rd_FlagGroup(AngbandWorld::get_instance().sf_retired, rd_byte);
}

static void load_player_world(PlayerType *player_ptr)
{
    rd_total_play_time();
    rd_world_info();
    rd_winner_class();
    rd_base_info(player_ptr);
    rd_player_info(player_ptr);
    preserve_mode = rd_bool();
    AngbandSystem::get_instance().set_awaiting_report_score(rd_bool());
    rd_dummy2();
    rd_global_configurations(player_ptr);
    rd_extra(player_ptr);

    if (player_ptr->energy_need < -999) {
        player_ptr->timewalk = true;
    }

    load_note(_("特別情報をロードしました", "Loaded extra information"));
}

static errr load_hp(PlayerType *player_ptr)
{
    auto tmp16u = rd_u16b();
    if (tmp16u > PY_MAX_LEVEL) {
        load_note(format(_("ヒットポイント配列が大きすぎる(%u)！", "Too many (%u) hitpoint entries!"), tmp16u));
        return 25;
    }

    for (auto i = 0; i < tmp16u; i++) {
        player_ptr->player_hp[i] = rd_s16b();
    }

    return 0;
}

static void load_spells(PlayerType *player_ptr)
{
    player_ptr->spell_learned1 = rd_u32b();
    player_ptr->spell_learned2 = rd_u32b();
    player_ptr->spell_worked1 = rd_u32b();
    player_ptr->spell_worked2 = rd_u32b();
    player_ptr->spell_forgotten1 = rd_u32b();
    player_ptr->spell_forgotten2 = rd_u32b();
    player_ptr->learned_spells = rd_s16b();
    player_ptr->add_spells = rd_s16b();
}

static errr verify_checksum()
{
    auto n_v_check = v_check;
    auto a = rd_u32b();
    if (a == n_v_check) {
        return 0;
    }

    load_note(_("チェックサムがおかしい", "Invalid checksum"));
    return 11;
}

static errr verify_encoded_checksum()
{
    auto n_x_check = x_check;
    auto a = rd_u32b();
    if (a == n_x_check) {
        return 0;
    }

    load_note(_("エンコードされたチェックサムがおかしい", "Invalid encoded checksum"));
    return 11;
}

/*!
 * @brief セーブファイル読み込み処理の実体 / Actually read the savefile
 * @return エラーコード
 */
static errr exe_reading_savefile(PlayerType *player_ptr)
{
    rd_version_info();
    if (!loading_savefile_version_is_older_than(SAVEFILE_VERSION + 1)) {
        load_note(_("セーブデータのバージョンが新しすぎる", "Savefile version is too new"));
        return -1;
    }

    rd_dummy3();
    rd_system_info();
    load_lore();
    auto item_loader = ItemLoaderFactory::create_loader();
    item_loader->load_item();
    auto load_town_quest_result = load_town_quest(player_ptr);
    if (load_town_quest_result != 0) {
        return load_town_quest_result;
    }

    load_note(_("クエスト情報をロードしました", "Loaded Quests"));
    item_loader->load_artifact();
    load_player_world(player_ptr);
    auto load_hp_result = load_hp(player_ptr);
    if (load_hp_result != 0) {
        return load_hp_result;
    }

    sp_ptr = &sex_info[player_ptr->psex];
    rp_ptr = &race_info[enum2i(player_ptr->prace)];
    cp_ptr = &class_info.at(player_ptr->pclass);
    ap_ptr = &personality_info[player_ptr->ppersonality];

    auto short_pclass = enum2i(player_ptr->pclass);
    mp_ptr = &class_magics_info[short_pclass];

    load_spells(player_ptr);
    if (PlayerClass(player_ptr).equals(PlayerClassType::MINDCRAFTER)) {
        player_ptr->add_spells = 0;
    }

    auto load_inventory_result = load_inventory(player_ptr);
    if (load_inventory_result != 0) {
        return load_inventory_result;
    }

    load_store(player_ptr);
    player_ptr->pet_follow_distance = rd_s16b();
    player_ptr->pet_extra_flags = rd_u16b();

    auto restore_dungeon_result = restore_dungeon(player_ptr);
    if (restore_dungeon_result != 0) {
        return restore_dungeon_result;
    }

    auto checksum_result = verify_checksum();
    if (checksum_result != 0) {
        return checksum_result;
    }

    return verify_encoded_checksum();
}

/*!
 * @brief セーブファイル読み込み処理 (UIDチェック等含む) / Reading the savefile (including UID check)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return エラーコード
 */
static errr rd_savefile(PlayerType *player_ptr)
{
    safe_setuid_grab();
    loading_savefile = angband_fopen(savefile, FileOpenMode::READ, true);
    safe_setuid_drop();
    if (!loading_savefile) {
        return -1;
    }

    try {
        auto err = exe_reading_savefile(player_ptr);
        if (ferror(loading_savefile)) {
            err = -1;
        }

        angband_fclose(loading_savefile);
        return err;
    } catch (SaveDataNotSupportedException const &e) {
        msg_print(e.what());
        angband_fclose(loading_savefile);
        return 1;
    }
}

/*!
 * @brief 死亡した、または互換性のないセーブデータを読み込んだ時にやりなおさせる
 * @param plyaer_ptr プレイヤーへの参照ポインタ
 * @param new_game 新しくゲームを始めさせるフラグ
 * @return 常にtrue (前後の処理上都合が良いため)
 */
static bool reset_save_data(PlayerType *player_ptr, bool *new_game)
{
    *new_game = true;
    player_ptr->is_dead = false;
    AngbandWorld::get_instance().sf_lives++;
    return true;
}

static bool on_read_save_data_not_supported(PlayerType *player_ptr, bool *new_game)
{
    auto mes_not_play = _("このセーブデータの続きをプレイすることはできません。", "You can't play the rest of the game from this save data.");
    auto mes_check_restart = _("最初からプレイを始めますか？(モンスターの思い出は引き継がれます)", "Play from the beginning? (Monster recalls will be inherited.) ");
    msg_print(mes_not_play);
    msg_erase();
    if (!input_check(mes_check_restart)) {
        msg_print(_("ゲームを終了します。", "Exit the game."));
        msg_erase();
        return false;
    }

    AngbandSystem::get_instance().set_awaiting_report_score(false);
    return reset_save_data(player_ptr, new_game);
}

/**
 * @brief セーブデータから引き継いでプレイできるかどうか調べる
 *
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 引き継ぎ可能ならtrue、そうでなければfalseを返す
 */
static bool can_takeover_savefile(PlayerType *player_ptr)
{
    if (loading_savefile_version_is_older_than(8) && PlayerClass(player_ptr).equals(PlayerClassType::SMITH)) {
        return false;
    }

    return true;
}

/*!
 * @brief セーブデータ読み込みのメインルーチン /
 * Attempt to Load a "savefile"
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param new_game セーブデータの新規作成が必要か否か
 * @return セーブデータが読み込めればtrue
 */
bool load_savedata(PlayerType *player_ptr, bool *new_game)
{
    auto what = "generic";
    AngbandWorld::get_instance().game_turn = 0;
    player_ptr->is_dead = false;
    if (savefile.empty()) {
        return true;
    }

    const auto &savefile_str = savefile.string();
#ifndef WINDOWS
    if (access(savefile_str.data(), 0) < 0) {
        msg_print(_("セーブファイルがありません。", "Savefile does not exist."));
        msg_erase();
        *new_game = true;
        return true;
    }
#endif

    auto err = false;
    auto fd = -1;

    // バリアント名長1バイト+バージョン番号4バイト+セーブファイルエンコードキー1バイト == 6バイト.
    constexpr auto variant_length = static_cast<char>(VARIANT_NAME.length());
    constexpr auto version_length = variant_length + 6;
    char tmp_ver[version_length]{};
    if (!err) {
        fd = fd_open(savefile, O_RDONLY);
        if (fd < 0) {
            err = true;
        }

        if (err) {
            what = _("セーブファイルを開けません", "Cannot open savefile");
        }
    }

    if (!err) {
        if (fd_read(fd, tmp_ver, version_length)) {
            err = true;
        }

        if (err) {
            what = _("セーブファイルを読めません", "Cannot read savefile");
        }
    }

    if (err) {
        msg_format("%s: %s", what, savefile_str.data());
        msg_erase();
        return false;
    }

    fd_close(fd);

    if (!err) {
        term_clear();
        auto ret_rd_savefile = rd_savefile(player_ptr);
        if (ret_rd_savefile != 0 && ret_rd_savefile != 11) {
            err = true;
        }

        if (ret_rd_savefile < 0) {
            what = _("セーブファイルを解析出来ません。", "Cannot parse savefile");
        } else if (ret_rd_savefile > 0 && ret_rd_savefile != 11) {
            return on_read_save_data_not_supported(player_ptr, new_game);
        }
    }

    auto &world = AngbandWorld::get_instance();
    if (!err) {
        if (!world.game_turn) {
            err = true;
        }

        if (err) {
            what = _("セーブファイルが壊れています", "Broken savefile");
        }
    }

    if (err) {
        auto &system = AngbandSystem::get_instance();
        constexpr auto fmt = _("エラー(%s)がバージョン %s 用セーブファイル読み込み中に発生。", "Error (%s) reading %s savefile.");
        msg_format(fmt, what, system.build_version_expression(VersionExpression::WITH_EXTRA).data());
        msg_erase();
        return false;
    }

    if (!can_takeover_savefile(player_ptr)) {
        return on_read_save_data_not_supported(player_ptr, new_game);
    }

    if (player_ptr->is_dead || wc_ptr->is_blown_away()) {
        return reset_save_data(player_ptr, new_game);
    }

    world.character_loaded = true;
    auto tmp = counts_read(player_ptr, 2);
    if (tmp > player_ptr->count) {
        player_ptr->count = tmp;
    }

    const auto play_time = world.play_time.elapsed_sec();
    if (counts_read(player_ptr, 0) > play_time || counts_read(player_ptr, 1) == play_time) {
        counts_write(player_ptr, 2, ++player_ptr->count);
    }

    counts_write(player_ptr, 1, play_time);
    return true;
}
