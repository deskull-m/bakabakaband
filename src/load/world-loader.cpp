#include "load/world-loader.h"
#include "load/angband-version-comparer.h"
#include "load/load-util.h"
#include "load/load-zangband.h"
#include "market/bounty.h"
#include "system/angband-system.h"
#include "system/building-type-definition.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/dungeon/dungeon-record.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/floor/floor-info.h"
#include "system/floor/wilderness-grid.h"
#include "system/inner-game-data.h"
#include "system/player-type-definition.h"
#include "world/world.h"

static void rd_hengband_dungeons()
{
    const int dungeons_size = DungeonList::get_instance().size();
    const auto &dungeons = DungeonList::get_instance();
    auto &records = DungeonRecords::get_instance();
    const int max = rd_byte();
    for (auto i = 0; i < max; i++) {
        int tmp16s = rd_s16b();
        if (i >= dungeons_size) {
            continue;
        }

        const auto dungeon_id = i2enum<DungeonId>(i);
        const auto &dungeon = dungeons.get_dungeon(dungeon_id);
        auto &record = records.get_record(dungeon_id);
        if (tmp16s > 0) {
            record.set_max_level(tmp16s);
        }

        if (record.get_max_level() > dungeon.maxdepth) {
            record.set_max_level(dungeon.maxdepth);
        }
    }
}

void rd_dungeons(PlayerType *player_ptr)
{
    rd_hengband_dungeons();
    if (player_ptr->max_plv < player_ptr->lev) {
        player_ptr->max_plv = player_ptr->lev;
    }
}

/*!
 * @brief 現実変容処理の有無及びその残りターン数を読み込む
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void rd_alter_reality(PlayerType *player_ptr)
{
    player_ptr->recall_dungeon = i2enum<DungeonId>(rd_s16b());
    player_ptr->alter_reality = rd_s16b();
}

void set_gambling_monsters()
{
    auto &melee_arena = MeleeArena::get_instance();
    for (auto i = 0; i < NUM_GLADIATORS; i++) {
        const auto monrace_id = i2enum<MonraceId>(rd_s16b());
        uint32_t odds;
        odds = rd_u32b();

        melee_arena.set_gladiator(i, { monrace_id, odds });
    }
}

/*!
 * @details 自動拾い関係はこれしかないのでworldに突っ込むことにする。必要があれば再分割する
 */
void rd_autopick(PlayerType *player_ptr)
{
    player_ptr->autopick_autoregister = rd_bool();
}

static void rd_world_info(PlayerType *player_ptr)
{
    auto &igd = InnerGameData::get_instance();
    igd.init_turn_limit();
    auto &world = AngbandWorld::get_instance();
    world.dungeon_turn_limit = TURNS_PER_TICK * TOWN_DAWN * (MAX_DAYS - 1) + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
    player_ptr->current_floor_ptr->generated_turn = rd_s32b();
    player_ptr->feeling_turn = rd_s32b();

    world.game_turn = rd_s32b();
    world.dungeon_turn = rd_s32b();
    world.arena_start_turn = rd_s32b();
    world.today_mon = i2enum<MonraceId>(rd_s16b());
    world.knows_daily_bounty = rd_s16b() != 0; // 現在bool型だが、かつてモンスター種族IDを保存していた仕様に合わせる
}

void rd_global_configurations(PlayerType *player_ptr)
{
    auto &system = AngbandSystem::get_instance();
    system.set_seed_flavor(rd_u32b());
    system.set_seed_town(rd_u32b());
    system.set_panic_save(rd_u16b() > 0);
    auto &world = AngbandWorld::get_instance();
    world.total_winner = rd_u16b();
    world.noscore = rd_u16b();

    player_ptr->is_dead = rd_bool();

    player_ptr->feeling = rd_byte();
    rd_world_info(player_ptr);
}

void load_wilderness_info(PlayerType *player_ptr)
{
    const auto x = rd_s32b();
    const auto y = rd_s32b();
    auto &wilderness = WildernessGrids::get_instance();
    wilderness.set_player_position({ y, x });

    auto &world = AngbandWorld::get_instance();
    world.set_wild_mode(rd_bool());
    player_ptr->ambush_flag = rd_bool();
}

errr analyze_wilderness(void)
{
    const auto wild_x_size = rd_s32b();
    const auto wild_y_size = rd_s32b();
    auto &wilderness = WildernessGrids::get_instance();
    const auto &area = wilderness.get_area();
    if ((wild_y_size > area.height()) || (wild_x_size > area.width())) {
        load_note(format(_("荒野が大きすぎる(%d/%d)！", "Wilderness is too big (%d/%d)!"), wild_x_size, wild_y_size));
        return 23;
    }

    for (auto x = 0; x < wild_x_size; x++) {
        for (auto y = 0; y < wild_y_size; y++) {
            wilderness.get_grid({ y, x }).seed = rd_u32b();
        }
    }

    return 0;
}
