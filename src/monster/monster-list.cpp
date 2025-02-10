/*!
 * @brief モンスター処理 / misc code for monsters
 * @date 2014/07/08
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * 2014 Deskull rearranged comment for Doxygen.
 */

#include "monster/monster-list.h"
#include "core/speed-table.h"
#include "dungeon/dungeon-flag-types.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "floor/geometry.h"
#include "floor/wild.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-options.h"
#include "grid/grid.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-kind-mask.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-update.h"
#include "monster/monster-util.h"
#include "pet/pet-fall-off.h"
#include "player/player-status.h"
#include "system/alloc-entries.h"
#include "system/dungeon-info.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/system-variables.h"
#include "util/bit-flags-calculator.h"
#include "util/probability-table.h"
#include "view/display-messages.h"
#include "world/world-collapsion.h"
#include "world/world.h"
#include <cmath>
#include <iterator>

/*!
 * @brief モンスター配列の空きを探す / Acquires and returns the index of a "free" monster.
 * @return 利用可能なモンスター配列の添字
 * @details
 * This routine should almost never fail, but it *can* happen.
 */
MONSTER_IDX m_pop(FloorType *floor_ptr)
{
    /* Normal allocation */
    if (floor_ptr->m_max < MAX_FLOOR_MONSTERS) {
        const auto i = floor_ptr->m_max;
        floor_ptr->m_max++;
        floor_ptr->m_cnt++;
        return i;
    }

    /* Recycle dead monsters */
    for (short i = 1; i < floor_ptr->m_max; i++) {
        const auto &monster = floor_ptr->m_list[i];
        if (monster.is_valid()) {
            continue;
        }

        floor_ptr->m_cnt++;
        return i;
    }

    if (AngbandWorld::get_instance().character_dungeon) {
        msg_print(_("モンスターが多すぎる！", "Too many monsters!"));
    }

    return 0;
}

/*!
 * @brief 生成モンスター種族を1種生成テーブルから選択する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param min_level 最小生成階
 * @param max_level 最大生成階
 * @return 選択されたモンスター生成種族
 * @details nasty生成 (ゲーム内経過日数に応じて、現在フロアより深いフロアのモンスターを出現させる仕様)は
 */
MonraceId get_mon_num(PlayerType *player_ptr, DEPTH min_level, DEPTH max_level, BIT_FLAGS mode)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    if (max_level > MAX_DEPTH - 1) {
        max_level = MAX_DEPTH - 1;
    }
    const auto &dungeon = dungeons_info[floor.dungeon_idx];

    /* Boost the max_level */
    if (any_bits(mode, PM_ARENA) || dungeon.flags.has_not(DungeonFeatureType::BEGINNER)) {
        /* Nightmare mode allows more out-of depth monsters */
        if (ironman_nightmare) {
            /* What a bizarre calculation */
            max_level = 1 + (max_level * MAX_DEPTH / randint1(MAX_DEPTH));
        }
    }

    max_level += wc_ptr->plus_monster_level();

    ProbabilityTable<int> prob_table;

    /* Process probabilities */
    const auto &monraces = MonraceList::get_instance();
    const auto &table = MonraceAllocationTable::get_instance();
    for (auto i = 0U; i < table.size(); i++) {
        const auto &entry = table.get_entry(i);
        if (entry.level < min_level) {
            continue;
        }
        if (max_level < entry.level) {
            break;
        } // sorted by depth array,
        const auto monrace_id = entry.index;
        auto &monrace = monraces.get_monrace(monrace_id);
        if (none_bits(mode, PM_ARENA | PM_CHAMELEON)) {
            if (monrace.can_generate() && none_bits(mode, PM_CLONE)) {
                continue;
            }

            if (monrace.population_flags.has(MonsterPopulationType::ONLY_ONE) && monrace.has_entity()) {
                continue;
            }

            if (monrace.population_flags.has(MonsterPopulationType::BUNBUN_STRIKER) && (monrace.cur_num >= MAX_BUNBUN_NUM)) {
                continue;
            }

            if (!monraces.is_selectable(monrace_id)) {
                continue;
            }
        }

        prob_table.entry_item(i, entry.prob2);
    }

    if (cheat_hear) {
        msg_format(_("モンスター第3次候補数:%d(%d-%dF)%d ", "monster third selection:%d(%d-%dF)%d "), prob_table.item_count(), min_level, max_level,
            prob_table.total_prob());
    }

    if (prob_table.empty()) {
        return MonraceList::empty_id();
    }

    // 40%で1回、50%で2回、10%で3回抽選し、その中で一番レベルが高いモンスターを選択する
    int n = 1;

    const int p = randint0(100);
    if (p < 60) {
        n++;
    }
    if (p < 10) {
        n++;
    }

    std::vector<int> result;
    ProbabilityTable<int>::lottery(std::back_inserter(result), prob_table, n);
    const auto it = std::max_element(result.begin(), result.end(),
        [&table](int a, int b) { return table.get_entry(a).level < table.get_entry(b).level; });
    return table.get_entry(*it).index;
}

/*!
 * @brief カメレオンの王の変身対象となるモンスターかどうか判定する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param r_idx モンスター種族ID
 * @param m_idx 変身するモンスターのモンスターID
 * @param grid カメレオンの足元の地形
 * @param summoner_m_idx モンスターの召喚による場合、召喚者のモンスターID
 * @return 対象にできるならtrueを返す
 */
static bool monster_hook_chameleon_lord(PlayerType *player_ptr, MonraceId r_idx, MONSTER_IDX m_idx, const Grid &grid, std::optional<MONSTER_IDX> summoner_m_idx)
{
    const auto &monraces = MonraceList::get_instance();
    const auto &monrace = monraces.get_monrace(r_idx);
    if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE)) {
        return false;
    }

    if (monrace.behavior_flags.has(MonsterBehaviorType::FRIENDLY) || monrace.misc_flags.has(MonsterMiscType::CHAMELEON)) {
        return false;
    }

    if (std::abs(monrace.level - monraces.get_monrace(MonraceId::CHAMELEON_K).level) > 5) {
        return false;
    }

    if (monrace.is_explodable()) {
        return false;
    }

    if (!monster_can_cross_terrain(player_ptr, grid.feat, &monrace, 0)) {
        return false;
    }

    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &monster = floor.m_list[m_idx];
    const auto &old_monrace = monster.get_monrace();
    if (old_monrace.misc_flags.has_not(MonsterMiscType::CHAMELEON)) {
        return !monster_has_hostile_align(player_ptr, &monster, 0, 0, &monrace);
    }

    return !summoner_m_idx || !monster_has_hostile_align(player_ptr, &floor.m_list[*summoner_m_idx], 0, 0, &monrace);
}

/*!
 * @brief カメレオンの変身対象となるモンスターかどうか判定する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param r_idx モンスター種族ID
 * @param m_idx 変身するモンスターのモンスターID
 * @param grid カメレオンの足元の地形
 * @param summoner_m_idx モンスターの召喚による場合、召喚者のモンスターID
 * @return 対象にできるならtrueを返す
 * @todo グローバル変数対策の上 monster_hook.cへ移す。
 */
static bool monster_hook_chameleon(PlayerType *player_ptr, MonraceId r_idx, MONSTER_IDX m_idx, const Grid &grid, std::optional<MONSTER_IDX> summoner_m_idx)
{
    const auto &monrace = MonraceList::get_instance().get_monrace(r_idx);
    if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
        return false;
    }

    if (monrace.misc_flags.has(MonsterMiscType::MULTIPLY)) {
        return false;
    }

    if (monrace.behavior_flags.has(MonsterBehaviorType::FRIENDLY) || (monrace.misc_flags.has(MonsterMiscType::CHAMELEON))) {
        return false;
    }

    if (monrace.is_explodable()) {
        return false;
    }

    if (!monster_can_cross_terrain(player_ptr, grid.feat, &monrace, 0)) {
        return false;
    }

    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &monster = floor.m_list[m_idx];
    const auto &old_monrace = monster.get_monrace();
    if (old_monrace.misc_flags.has_not(MonsterMiscType::CHAMELEON)) {
        if (old_monrace.kind_flags.has(MonsterKindType::GOOD) && monrace.kind_flags.has_not(MonsterKindType::GOOD)) {
            return false;
        }

        if (old_monrace.kind_flags.has(MonsterKindType::EVIL) && monrace.kind_flags.has_not(MonsterKindType::EVIL)) {
            return false;
        }

        if (old_monrace.kind_flags.has_none_of(alignment_mask)) {
            return false;
        }
    } else if (summoner_m_idx && monster_has_hostile_align(player_ptr, &floor.m_list[*summoner_m_idx], 0, 0, &monrace)) {
        return false;
    }

    auto hook_pf = get_monster_hook(player_ptr);
    return hook_pf(player_ptr, r_idx);
}

static std::optional<MonraceId> polymorph_of_chameleon(PlayerType *player_ptr, MONSTER_IDX m_idx, const Grid &grid, const std::optional<MONSTER_IDX> summoner_m_idx)
{
    auto &floor = *player_ptr->current_floor_ptr;
    auto &monster = floor.m_list[m_idx];
    const auto old_unique = monster.get_monrace().kind_flags.has(MonsterKindType::UNIQUE);
    auto hook_fp = old_unique ? monster_hook_chameleon_lord : monster_hook_chameleon;
    auto hook = [m_idx, grid, summoner_m_idx, hook_fp](PlayerType *player_ptr, MonraceId r_idx) {
        return hook_fp(player_ptr, r_idx, m_idx, grid, summoner_m_idx);
    };
    get_mon_num_prep_chameleon(player_ptr, std::move(hook));

    int level;
    if (old_unique) {
        level = MonraceList::get_instance().get_monrace(MonraceId::CHAMELEON_K).level;
    } else if (!floor.is_in_underground()) {
        level = wilderness[player_ptr->wilderness_y][player_ptr->wilderness_x].level;
    } else {
        level = floor.dun_level;
    }

    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::CHAMELEON)) {
        level += 2 + randint1(3);
    }

    const auto new_monrace_id = get_mon_num(player_ptr, 0, level, PM_CHAMELEON);
    if (!MonraceList::is_valid(new_monrace_id)) {
        return std::nullopt;
    }

    return new_monrace_id;
}

/*!
 * @brief カメレオンの変身処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 変身処理を受けるモンスター情報のID
 * @param grid カメレオンの足元の地形
 * @param summoner_m_idx モンスターの召喚による場合、召喚者のモンスターID
 */
void choose_chameleon_polymorph(PlayerType *player_ptr, MONSTER_IDX m_idx, const Grid &grid, std::optional<MONSTER_IDX> summoner_m_idx)
{
    auto &floor = *player_ptr->current_floor_ptr;
    auto &monster = floor.m_list[m_idx];

    auto new_monrace_id = polymorph_of_chameleon(player_ptr, m_idx, grid, summoner_m_idx);
    if (!new_monrace_id) {
        return;
    }

    monster.r_idx = *new_monrace_id;
    monster.ap_r_idx = *new_monrace_id;
}

/*!
 * @brief 指定したモンスターに隣接しているモンスターの数を返す。
 * / Count number of adjacent monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 隣接数を調べたいモンスターのID
 * @return 隣接しているモンスターの数
 */
int get_monster_crowd_number(FloorType *floor_ptr, MONSTER_IDX m_idx)
{
    auto *m_ptr = &floor_ptr->m_list[m_idx];
    POSITION my = m_ptr->fy;
    POSITION mx = m_ptr->fx;
    int count = 0;
    for (int i = 0; i < 7; i++) {
        int ay = my + ddy_ddd[i];
        int ax = mx + ddx_ddd[i];

        if (!in_bounds(floor_ptr, ay, ax)) {
            continue;
        }
        if (floor_ptr->grid_array[ay][ax].has_monster()) {
            count++;
        }
    }

    return count;
}
