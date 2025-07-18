/*!
 * @file save.c
 * @brief セーブファイル書き込み処理 / Purpose: interact with savefiles
 * @date 2014/07/12
 * @author
 * <pre>
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * </pre>
 */

#include "save/save.h"
#include "core/object-compressor.h"
#include "dungeon/quest.h"
#include "inventory/inventory-slot-types.h"
#include "io/files-util.h"
#include "io/report.h"
#include "io/uid-checker.h"
#include "locale/character-encoding.h"
#include "monster/monster-compaction.h"
#include "player/player-status.h"
#include "save/floor-writer.h"
#include "save/info-writer.h"
#include "save/item-writer.h"
#include "save/lore-writer.h"
#include "save/player-writer.h"
#include "save/save-util.h"
#include "system/artifact-type-definition.h"
#include "system/floor/floor-info.h"
#include "system/floor/town-info.h"
#include "system/floor/town-list.h"
#include "system/floor/wilderness-grid.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "util/angband-files.h"
#include "view/display-messages.h"
#include "world/world-collapsion.h"
#include "world/world.h"
#include <algorithm>
#include <sstream>
#include <string>

#include <ctime>

/*!
 * @brief セーブデータの書き込み /
 * Actually write a save-file
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 成功すればtrue
 */
static bool wr_savefile_new(PlayerType *player_ptr)
{
    compact_monsters(player_ptr, 0);

    uint32_t now = (uint32_t)time((time_t *)0);
    auto &world = AngbandWorld::get_instance();
    world.sf_system = 0L;
    world.sf_when = now;
    world.sf_saves++;

    save_xor_byte = 0;
    auto variant_length = VARIANT_NAME.length();
    wr_byte(static_cast<byte>(variant_length));
    for (auto i = 0U; i < variant_length; i++) {
        save_xor_byte = 0;
        wr_byte(VARIANT_NAME[i]);
    }

    save_xor_byte = 0;
    wr_byte(H_VER_MAJOR);
    wr_byte(H_VER_MINOR);
    wr_byte(H_VER_PATCH);
    wr_byte(H_VER_EXTRA);

    auto tmp8u = static_cast<uint8_t>(Rand_external(256));
    wr_byte(tmp8u);
    v_stamp = 0L;
    x_stamp = 0L;

    wr_u32b(world.sf_system);
    wr_u32b(world.sf_when);
    wr_u16b(world.sf_lives);
    wr_u16b(world.sf_saves);

    wr_u32b(SAVEFILE_VERSION);
    wr_u16b(0);
    wr_byte(0);

#ifdef JP
#ifdef EUC
    wr_byte(enum2i(CharacterEncoding::EUC_JP));
#endif
#ifdef SJIS
    wr_byte(enum2i(CharacterEncoding::SHIFT_JIS));
#endif
#else
    wr_byte(enum2i(CharacterEncoding::US_ASCII));
#endif

    wr_randomizer();
    wr_options();
    wr_message_history();

    uint16_t tmp16u = static_cast<uint16_t>(MonraceList::get_instance().size());
    wr_u16b(tmp16u);
    for (auto r_idx = 0; r_idx < tmp16u; r_idx++) {
        wr_lore(i2enum<MonraceId>(r_idx));
    }

    tmp16u = static_cast<uint16_t>(BaseitemList::get_instance().size());
    wr_u16b(tmp16u);
    for (short bi_id = 0; bi_id < tmp16u; bi_id++) {
        wr_perception(bi_id);
    }

    tmp16u = static_cast<uint16_t>(towns_info.size());
    wr_u16b(tmp16u);

    const auto &quests = QuestList::get_instance();
    tmp16u = static_cast<uint16_t>(quests.size());
    wr_u16b(tmp16u);

    tmp8u = MAX_RANDOM_QUEST - MIN_RANDOM_QUEST;
    wr_byte(tmp8u);

    for (const auto &[quest_id, quest] : quests) {
        wr_s16b(enum2i(quest_id));
        wr_s16b(enum2i(quest.status));
        wr_s16b((int16_t)quest.level);
        wr_byte((byte)quest.complev);
        wr_u32b(quest.comptime);

        auto is_quest_running = quest.status == QuestStatusType::TAKEN;
        is_quest_running |= quest.status == QuestStatusType::COMPLETED;
        is_quest_running |= !QuestType::is_fixed(quest_id);
        if (!is_quest_running) {
            continue;
        }

        wr_s16b((int16_t)quest.cur_num);
        wr_s16b((int16_t)quest.max_num);
        wr_s16b(enum2i(quest.type));
        wr_s16b(enum2i(quest.r_idx));
        wr_s16b(enum2i(quest.reward_fa_id));
        wr_byte((byte)quest.flags);
        wr_byte((byte)quest.dungeon);
    }

    const auto &wilderness = WildernessGrids::get_instance();
    const auto &pos = wilderness.get_player_position();
    wr_s32b(pos.x);
    wr_s32b(pos.y);
    wr_bool(world.is_wild_mode());
    wr_bool(player_ptr->ambush_flag);
    const auto &area = wilderness.get_area();
    wr_s32b(area.width());
    wr_s32b(area.height());
    for (auto x = 0; x < area.width(); x++) {
        for (auto y = 0; y < area.height(); y++) {
            wr_u32b(wilderness.get_grid({ y, x }).get_seed());
        }
    }

    const auto &artifacts = ArtifactList::get_instance();
    auto max_a_num = enum2i(artifacts.rbegin()->first);
    tmp16u = max_a_num + 1;
    wr_u16b(tmp16u);
    for (auto i = 0U; i < tmp16u; i++) {
        const auto a_idx = i2enum<FixedArtifactId>(i);
        const auto &artifact = artifacts.get_artifact(a_idx);
        wr_bool(artifact.is_generated);
        wr_s16b(artifact.floor_id);
    }

    wr_u32b(world.sf_play_time);
    wr_s32b(wc_ptr->collapse_degree);
    wr_FlagGroup(world.sf_winner, wr_byte);
    wr_FlagGroup(world.sf_retired, wr_byte);

    wr_player(player_ptr);
    tmp16u = PY_MAX_LEVEL;
    wr_u16b(tmp16u);
    for (int i = 0; i < tmp16u; i++) {
        wr_s16b((int16_t)player_ptr->player_hp[i]);
    }

    wr_u32b(player_ptr->spell_learned1);
    wr_u32b(player_ptr->spell_learned2);
    wr_u32b(player_ptr->spell_worked1);
    wr_u32b(player_ptr->spell_worked2);
    wr_u32b(player_ptr->spell_forgotten1);
    wr_u32b(player_ptr->spell_forgotten2);
    wr_s16b(player_ptr->learned_spells);
    wr_s16b(player_ptr->add_spells);
    for (auto i = 0; i < 64; i++) {
        const auto spell_id = (i < std::ssize(player_ptr->spell_order_learned)) ? player_ptr->spell_order_learned[i] : 255;
        wr_byte(static_cast<byte>(spell_id));
    }

    for (int i = 0; i < INVEN_TOTAL; i++) {
        const auto &item = *player_ptr->inventory[i];
        if (!item.is_valid()) {
            continue;
        }

        wr_u16b((uint16_t)i);
        wr_item(item);
    }

    wr_u16b(0xFFFF);
    tmp16u = static_cast<uint16_t>(towns_info.size());
    wr_u16b(tmp16u);

    tmp16u = MAX_STORES;
    wr_u16b(tmp16u);
    for (size_t i = 1; i < towns_info.size(); i++) {
        for (auto sst : STORE_SALE_TYPE_LIST) {
            wr_store(&towns_info[i].get_store(sst));
        }
    }

    wr_s16b(player_ptr->pet_follow_distance);
    wr_s16b(player_ptr->pet_extra_flags);
    if (AngbandSystem::get_instance().is_awaiting_report_status() || !player_ptr->is_dead) {
        wr_string(screen_dump);
    } else {
        wr_string("");
    }

    if (!player_ptr->is_dead) {
        if (!wr_dungeon(player_ptr)) {
            return false;
        }

        wr_ghost();
        wr_s32b(0);
    }

    wr_u32b(v_stamp);
    wr_u32b(x_stamp);
    return !ferror(saving_savefile) && (fflush(saving_savefile) != EOF);
}

/*!
 * @brief セーブデータ書き込みのサブルーチン
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param path セーブデータのフルパス
 * @param type セーブ後の処理種別
 * @return セーブの成功可否
 */
static bool save_player_aux(PlayerType *player_ptr, const std::filesystem::path &path)
{
    safe_setuid_grab();
    auto fd = fd_make(path);
    safe_setuid_drop();

    bool is_save_successful = false;
    saving_savefile = nullptr;
    if (fd >= 0) {
        (void)fd_close(fd);
        safe_setuid_grab();
        saving_savefile = angband_fopen(path, FileOpenMode::WRITE, true);
        safe_setuid_drop();
        if (saving_savefile) {
            if (wr_savefile_new(player_ptr)) {
                is_save_successful = true;
            }

            if (angband_fclose(saving_savefile)) {
                is_save_successful = false;
            }
        }

        safe_setuid_grab();
        if (!is_save_successful) {
            (void)fd_kill(path);
        }

        safe_setuid_drop();
    }

    if (!is_save_successful) {
        return false;
    }

    auto &world = AngbandWorld::get_instance();
    counts_write(player_ptr, 0, world.play_time.elapsed_sec());
    world.character_saved = true;
    return true;
}

/*!
 * @brief セーブデータ書き込みのメインルーチン
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 成功すればtrue
 * @details 以下の順番で書き込みを実行する.
 * 1. hoge.new にセーブデータを書き込む
 * 2. hoge をhoge.old にリネームする
 * 3. hoge.new をhoge にリネームする
 * 4. hoge.old を削除する
 */
bool save_player(PlayerType *player_ptr, SaveType type)
{
    std::stringstream ss_new;
    ss_new << savefile.string() << ".new";
    auto savefile_new = ss_new.str();
    safe_setuid_grab();
    fd_kill(savefile_new);

    safe_setuid_drop();
    auto &world = AngbandWorld::get_instance();
    world.play_time.update();
    auto result = false;
    if (save_player_aux(player_ptr, savefile_new.data())) {
        std::stringstream ss_old;
        ss_old << savefile.string() << ".old";
        auto savefile_old = ss_old.str();
        safe_setuid_grab();
        fd_kill(savefile_old);
        fd_move(savefile, savefile_old);
        fd_move(savefile_new, savefile);
        fd_kill(savefile_old);
        safe_setuid_drop();
        world.character_loaded = true;
        result = true;
    }

    if (type != SaveType::CLOSE_GAME) {
        world.is_loading_now = false;
        update_creature(player_ptr);
        player_ptr->current_floor_ptr->reset_mproc();
        world.is_loading_now = true;
    }

    return result;
}
