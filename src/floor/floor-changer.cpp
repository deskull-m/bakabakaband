#include "floor/floor-changer.h"
#include "action/travel-execution.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/quest-monster-placer.h"
#include "dungeon/quest.h"
#include "effect/effect-characteristics.h"
#include "floor/dungeon-feeling.h"
#include "floor/floor-generator.h"
#include "floor/floor-mode-changer.h"
#include "floor/floor-object.h"
#include "floor/floor-save-util.h"
#include "floor/floor-save.h"
#include "floor/floor-util.h"
#include "floor/party-monsters.h"
#include "floor/wild.h"
#include "game-option/birth-options.h"
#include "game-option/play-record-options.h"
#include "grid/grid.h"
#include "io/write-diary.h"
#include "load/floor-loader.h"
#include "main/sound-of-music.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/monster-remover.h"
#include "monster-floor/monster-summon.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "player-base/player-class.h"
#include "spell-kind/spells-floor.h"
#include "system/artifact-type-definition.h"
#include "system/creature-entity.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "window/main-window-util.h"
#include "world/world.h"
#include <algorithm>
#include <range/v3/view.hpp>

/*!
 * @brief 階段移動先のフロアが生成できない時に簡単な行き止まりマップを作成する / Builds the dead end
 */
static void build_dead_end(CreatureEntity &creature, saved_floor_type *sf_ptr)
{

    msg_print(_("階段は行き止まりだった。", "The staircases come to a dead end..."));
    clear_cave(creature);
    creature.x = creature.y = 0;
    creature.get_floor()->height = SCREEN_HGT;
    creature.get_floor()->width = SCREEN_WID;
    for (POSITION y = 0; y < MAX_HGT; y++) {
        for (POSITION x = 0; x < MAX_WID; x++) {
            place_bold(creature, y, x, GB_SOLID_PERM);
        }
    }

    creature.y = creature.get_floor()->height / 2;
    creature.x = creature.get_floor()->width / 2;
    place_bold(creature, creature.y, creature.x, GB_FLOOR);
    wipe_generate_random_floor_flags(*creature.get_floor());
    const auto &fcms = FloorChangeModesStore::get_instace();
    if (fcms->has(FloorChangeMode::UP)) {
        sf_ptr->upper_floor_id = 0;
    } else if (fcms->has(FloorChangeMode::DOWN)) {
        sf_ptr->lower_floor_id = 0;
    }
}

static std::pair<short, Pos2D> decide_pet_index(CreatureEntity &creature, const int current_monster)
{

    auto &floor = *creature.get_floor();
    const auto p_pos = creature.get_position();
    Pos2D pos(0, 0);
    if (current_monster == 0) {
        const auto m_idx = floor.pop_empty_index_monster();
        creature.riding = m_idx;
        if (m_idx) {
            pos = p_pos;
        }

        return { m_idx, pos };
    }

    int d;
    for (d = 1; d < A_MAX; d++) {
        int j;
        for (j = 1000; j > 0; j--) {
            pos = scatter(floor, p_pos, d, PROJECT_NONE);
            if (monster_can_enter(&creature, pos.y, pos.x, party_monsters[current_monster].get_monrace(), 0)) {
                break;
            }
        }

        if (j != 0) {
            break;
        }
    }

    const short m_idx = (d == 6) ? 0 : floor.pop_empty_index_monster();
    return { m_idx, pos };
}

static MonraceDefinition &set_pet_params(CreatureEntity &creature, const int current_monster, MONSTER_IDX m_idx, const POSITION cy, const POSITION cx)
{

    creature.get_floor()->grid_array[cy][cx].m_idx = m_idx;
    auto &monster = creature.get_floor()->m_list[m_idx];
    monster = party_monsters[current_monster].clone();
    monster.y = cy;
    monster.x = cx;
    monster.set_floor(creature.get_floor());
    monster.get_monster_profile().ml = true;
    monster.set_timed_effect(CreatureTimedEffect::SLEEP_OR_PARALYSIS, 0);
    monster.get_monster_profile().hold_o_idx_list.clear();
    monster.reset_target();
    auto &r_ref = monster.get_real_monrace();
    if (!ironman_nightmare) {
        monster.get_monster_profile().mflag.set(MonsterTemporaryFlagType::PREVENT_MAGIC);
    }

    return r_ref;
}

/*!
 * @brief 移動先のフロアに伴ったペットを配置する / Place preserved pet monsters on new floor
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void place_pet(CreatureEntity &creature)
{

    const auto max_num = AngbandWorld::get_instance().is_wild_mode() ? 1 : PartyMonsters::MAX_SIZE;
    auto &floor = *creature.get_floor();
    for (int current_monster = 0; current_monster < max_num; current_monster++) {
        if (!MonraceList::is_valid(party_monsters[current_monster].r_idx)) {
            continue;
        }

        const auto &[m_idx, pos] = decide_pet_index(creature, current_monster);
        if (m_idx != 0) {
            const auto &monrace = set_pet_params(creature, current_monster, m_idx, pos.y, pos.x);
            update_monster(creature, m_idx, true);
            lite_spot(creature, pos);
            if (monrace.misc_flags.has(MonsterMiscType::MULTIPLY)) {
                floor.num_repro++;
            }
        } else {
            const auto &monster = party_monsters[current_monster];
            auto &monrace = monster.get_real_monrace();
            msg_format(_("%sとはぐれてしまった。", "You have lost sight of %s."), monster_desc(creature, monster, 0).data());
            if (record_named_pet && monster.is_named()) {
                exe_write_diary(floor, DiaryKind::NAMED_PET, RECORD_NAMED_PET_LOST_SIGHT, monster_desc(creature, monster, MD_INDEF_VISIBLE));
            }

            if (monrace.has_entity()) {
                monrace.decrement_current_numbers();
            }
        }
    }

    party_monsters.wipe_all();
}

/*!
 * @brief ユニークモンスターやアーティファクトの所在フロアを更新する
 * @param floor フロアへの参照
 * @param cur_floor_id 現在のフロアID
 * @details
 * The floor_id and floor_id are not updated correctly
 * while new floor creation since dungeons may be re-created by
 * auto-scum option.
 */
static void update_unique_artifact(const FloorType &floor, int16_t cur_floor_id)
{
    for (int i = 1; i < floor.m_max; i++) {
        const auto &m_ref = floor.m_list[i];
        if (!m_ref.is_valid()) {
            continue;
        }

        auto &r_ref = m_ref.get_real_monrace();
        if (r_ref.kind_flags.has(MonsterKindType::UNIQUE) || (r_ref.population_flags.has(MonsterPopulationType::NAZGUL))) {
            r_ref.floor_id = cur_floor_id;
        }
    }

    for (const auto &item_ptr : floor.o_list) {
        if (!item_ptr->is_valid()) {
            continue;
        }

        if (item_ptr->is_fixed_artifact()) {
            item_ptr->get_fixed_artifact().floor_id = cur_floor_id;
        }
    }
}

static bool is_visited_floor(saved_floor_type *sf_ptr)
{
    return sf_ptr->last_visit != 0;
}

static void update_floor_id(CreatureEntity &creature, saved_floor_type *sf_ptr)
{

    const auto &fcms = FloorChangeModesStore::get_instace();
    const auto is_up = fcms->has(FloorChangeMode::UP);
    const auto is_down = fcms->has(FloorChangeMode::DOWN);
    if (!creature.in_saved_floor()) {
        if (is_up) {
            sf_ptr->lower_floor_id = 0;
        } else if (is_down) {
            sf_ptr->upper_floor_id = 0;
        }

        return;
    }

    saved_floor_type *cur_sf_ptr = get_sf_ptr(creature.floor_id);
    if (is_up) {
        if (cur_sf_ptr->upper_floor_id == new_floor_id) {
            sf_ptr->lower_floor_id = creature.floor_id;
        }

        return;
    }

    if (is_down && (cur_sf_ptr->lower_floor_id == new_floor_id)) {
        sf_ptr->upper_floor_id = creature.floor_id;
    }
}

static void reset_unique_by_floor_change(CreatureEntity &creature)
{

    auto &floor = *creature.get_floor();
    for (short i = 1; i < floor.m_max; i++) {
        auto &monster = floor.m_list[i];
        if (!monster.is_valid()) {
            continue;
        }

        if (!monster.is_pet()) {
            monster.hp = monster.maxhp = monster.max_maxhp;
            (void)set_monster_fast(floor, i, 0);
            (void)set_monster_slow(floor, i, 0);
            (void)set_monster_stunned(floor, i, 0);
            (void)set_monster_confused(floor, i, 0);
            (void)set_monster_monfear(floor, i, 0);
            (void)set_monster_invulner(floor, i, 0, false);
        }

        const auto &monrace = monster.get_real_monrace();
        if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE) && monrace.population_flags.has_not(MonsterPopulationType::NAZGUL)) {
            continue;
        }

        if (monrace.floor_id != new_floor_id) {
            delete_monster_idx(creature, i);
        }
    }
}

static void new_floor_allocation(CreatureEntity &creature, saved_floor_type *sf_ptr)
{

    GAME_TURN tmp_last_visit = sf_ptr->last_visit;
    const auto &floor = *creature.get_floor();
    auto alloc_chance = floor.get_dungeon_definition().max_m_alloc_chance;
    const auto &world = AngbandWorld::get_instance();
    while (tmp_last_visit > world.game_turn) {
        tmp_last_visit -= TURNS_PER_TICK * TOWN_DAWN;
    }

    GAME_TURN absence_ticks = (world.game_turn - tmp_last_visit) / TURNS_PER_TICK;
    reset_unique_by_floor_change(creature);
    std::vector<OBJECT_IDX> delete_i_idx_list;
    for (const auto &[i_idx, item_ptr] : floor.o_list | ranges::views::enumerate) {
        if (!item_ptr->is_valid() || !item_ptr->is_fixed_artifact()) {
            continue;
        }

        auto &artifact = item_ptr->get_fixed_artifact();
        if (artifact.floor_id == new_floor_id) {
            artifact.is_generated = true;
        } else {
            delete_i_idx_list.push_back(static_cast<OBJECT_IDX>(i_idx));
        }
    }
    delete_items(creature, std::move(delete_i_idx_list));

    (void)place_quest_monsters(creature);
    GAME_TURN alloc_times = absence_ticks / alloc_chance;
    if (randint0(alloc_chance) < (absence_ticks % alloc_chance)) {
        alloc_times++;
    }

    for (MONSTER_IDX i = 0; i < alloc_times; i++) {
        (void)alloc_monster(creature, 0, 0, summon_specific);
    }
}

/*!
 * @brief プレイヤー足元に階段を設置する
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void set_stairs(CreatureEntity &creature)
{

    auto &floor = *creature.get_floor();
    auto &grid = floor.grid_array[creature.y][creature.x];
    const auto &fcms = FloorChangeModesStore::get_instace();
    const auto &dungeon = floor.get_dungeon_definition();
    const auto &terrains = TerrainList::get_instance();
    if (fcms->has(FloorChangeMode::UP) && !inside_quest(floor.get_quest_id())) {
        const auto terrain_down_stair = terrains.get_terrain_id(TerrainTag::DOWN_STAIR);
        const auto converted_terrain_id = dungeon.convert_terrain_id(terrain_down_stair, TerrainCharacteristics::SHAFT);
        const auto terrain_id = fcms->has(FloorChangeMode::SHAFT) ? converted_terrain_id : terrain_down_stair;
        grid.set_terrain_id(terrain_id);
    } else if (fcms->has(FloorChangeMode::DOWN) && !ironman_downward) {
        const auto terrain_up_stair = terrains.get_terrain_id(TerrainTag::UP_STAIR);
        const auto converted_terrain_id = dungeon.convert_terrain_id(terrain_up_stair, TerrainCharacteristics::SHAFT);
        const auto terrain_id = fcms->has(FloorChangeMode::SHAFT) ? converted_terrain_id : terrain_up_stair;
        grid.set_terrain_id(terrain_id);
    }

    grid.mimic = 0;
    grid.special = creature.floor_id;
}

static void update_new_floor_feature(CreatureEntity &creature, saved_floor_type *sf_ptr, const bool loaded)
{

    if (loaded) {
        new_floor_allocation(creature, sf_ptr);
        return;
    }

    if (!is_visited_floor(sf_ptr)) {
        generate_floor(creature);
    } else {
        build_dead_end(creature, sf_ptr);
    }

    sf_ptr->last_visit = AngbandWorld::get_instance().game_turn;
    auto &floor = *creature.get_floor();
    sf_ptr->dun_level = floor.dun_level;
    if (FloorChangeModesStore::get_instace()->has(FloorChangeMode::NO_RETURN)) {
        return;
    }

    set_stairs(creature);
}

static void cut_off_the_upstair(CreatureEntity &creature)
{

    const auto &fcms = FloorChangeModesStore::get_instace();
    if (fcms->has(FloorChangeMode::RANDOM_PLACE)) {
        if (const auto p_pos = new_player_spot(creature); p_pos) {
            creature.set_position(*p_pos);
        }

        return;
    }

    if (fcms->has_not(FloorChangeMode::NO_RETURN) || fcms->has_none_of({ FloorChangeMode::DOWN, FloorChangeMode::UP })) {
        return;
    }

    const auto is_blind = creature.is_blind();
    const auto mes = is_blind
                         ? _("ゴトゴトと何か音がした。", "You hear some noises.")
                         : _("突然階段が塞がれてしまった！", "Suddenly the stairs is blocked!");
    msg_print(mes);
}

static void update_floor(CreatureEntity &creature)
{
    const auto &fcms = FloorChangeModesStore::get_instace();
    if (fcms->has_none_of({ FloorChangeMode::SAVE_FLOORS, FloorChangeMode::FIRST_FLOOR })) {
        generate_floor(creature);
        new_floor_id = 0;
        return;
    }

    if (new_floor_id == 0) {
        new_floor_id = get_unused_floor_id(creature);
    }

    saved_floor_type *sf_ptr;
    sf_ptr = get_sf_ptr(new_floor_id);
    const bool loaded = is_visited_floor(sf_ptr) && load_floor(creature, sf_ptr, 0);
    update_floor_id(creature, sf_ptr);
    update_new_floor_feature(creature, sf_ptr, loaded);
    cut_off_the_upstair(creature);
    sf_ptr->visit_mark = latest_visit_mark++;
}

/*!
 * @brief フロアの切り替え処理 / Enter new floor.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * If the floor is an old saved floor, it will be\n
 * restored from the temporary file.  If the floor is new one, new floor\n
 * will be generated.\n
 */
void change_floor(CreatureEntity &creature)
{

    auto &world = AngbandWorld::get_instance();
    world.character_dungeon = false;
    creature.dtrap = false;
    panel_row_min = 0;
    panel_row_max = 0;
    panel_col_min = 0;
    panel_col_max = 0;
    creature.ambush_flag = false;
    update_floor(creature);
    place_pet(creature);
    Travel::get_instance().reset_goal();
    auto &floor = *creature.get_floor();
    update_unique_artifact(floor, new_floor_id);
    creature.floor_id = new_floor_id;
    world.character_dungeon = true;
    if (creature.ppersonality == PERSONALITY_MUNCHKIN) {
        wiz_lite(creature, CreatureClass(creature).equals(PlayerClassType::NINJA));
    }

    floor.generated_turn = world.game_turn;
    auto &df = DungeonFeeling::get_instance();
    df.set_turns(floor.generated_turn);
    df.set_feeling(0);
    auto &fcms = FloorChangeModesStore::get_instace();
    fcms->clear();
    select_floor_music(creature);
    fcms->clear();

    // 移動後のフロアで階段を消去する処理
    if (creature.vanish_stairs_flag) {
        creature.vanish_stairs_flag = false;
        const auto &dungeon = floor.get_dungeon_definition();
        if (dungeon.flags.has(DungeonFeatureType::VANISH_STAIRS) && floor.is_underground()) {
            const auto p_pos = creature.get_position();
            auto &grid = floor.grid_array[p_pos.y][p_pos.x];
            const auto &terrain = grid.get_terrain();

            // 階段かどうかをチェック
            if (terrain.flags.has_any_of({ TerrainCharacteristics::UP_STAIRS, TerrainCharacteristics::DOWN_STAIRS })) {
                const auto floor_terrain_id = dungeon.select_floor_terrain_id();
                set_terrain_id_to_grid(creature, p_pos, floor_terrain_id);
                msg_print(_("階段が消え去った。", "The staircase vanishes."));
            }
        }
    }
}
