/*!
 * @file travel-execution.cpp
 * @brief トラベル移動処理実装
 */

#include "action/travel-execution.h"
#include "action/movement-execution.h"
#include "core/disturbance.h"
#include "game-option/disturbance-options.h"
#include "game-option/input-options.h"
#include "grid/grid.h"
#include "player-status/player-energy.h"
#include "player/player-move.h"
#include "player/player-status-flags.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"
#include <queue>
#include <vector>

namespace {
constexpr auto TRAVEL_UNABLE = 9999;

/*!
 * @brief トラベル処理中に地形に応じた移動コスト基準を返す
 * @param player_ptr	プレイヤーへの参照ポインタ
 * @param pos 該当地点の座標
 * @return コスト値
 */
int travel_flow_cost(PlayerType *player_ptr, const Pos2D &pos)
{
    int cost = 1;
    const auto &grid = player_ptr->current_floor_ptr->get_grid(pos);
    const auto &terrain = grid.get_terrain();
    if (terrain.flags.has(TerrainCharacteristics::AVOID_RUN)) {
        cost += 1;
    }

    if (terrain.flags.has_all_of({ TerrainCharacteristics::WATER, TerrainCharacteristics::DEEP }) && !player_ptr->levitation) {
        cost += 5;
    }

    if (terrain.flags.has(TerrainCharacteristics::LAVA)) {
        int lava = 2;
        if (!has_resist_fire(player_ptr)) {
            lava *= 2;
        }

        if (!player_ptr->levitation) {
            lava *= 2;
        }

        if (terrain.flags.has(TerrainCharacteristics::DEEP)) {
            lava *= 2;
        }

        cost += lava;
    }

    if (grid.is_mark()) {
        if (terrain.flags.has(TerrainCharacteristics::DOOR)) {
            cost += 1;
        }

        if (terrain.flags.has(TerrainCharacteristics::TRAP)) {
            cost += 10;
        }
    }

    return cost;
}

/*!
 * @brief 隣接するマスのコストを計算する
 * @param player_ptr	プレイヤーへの参照ポインタ
 * @param pos 隣接するマスの座標
 * @param current_cost 現在のマスのコスト
 * @param wall プレイヤーが壁の中にいるならばTRUE
 * @return 隣接するマスのコスト。移動不可ならばtl::nullopt
 */
tl::optional<int> travel_flow_aux(PlayerType *player_ptr, const Pos2D pos, int current_cost, bool wall)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &grid = floor.get_grid(pos);
    const auto &terrain = grid.get_terrain();
    if (!floor.contains(pos)) {
        return tl::nullopt;
    }

    if (floor.is_underground() && !(grid.info & CAVE_KNOWN)) {
        return tl::nullopt;
    }

    const auto from_wall = (current_cost / TRAVEL_UNABLE);
    auto is_wall = terrain.flags.has_any_of({ TerrainCharacteristics::WALL, TerrainCharacteristics::CAN_DIG });
    is_wall |= terrain.flags.has(TerrainCharacteristics::DOOR) && (grid.mimic > 0);
    auto can_move = terrain.flags.has_not(TerrainCharacteristics::MOVE);
    can_move &= terrain.flags.has(TerrainCharacteristics::CAN_FLY);
    can_move &= !player_ptr->levitation;

    auto add_cost = 1;
    if (is_wall || can_move) {
        if (!wall || !from_wall) {
            return tl::nullopt;
        }

        add_cost += TRAVEL_UNABLE;
    } else {
        add_cost = travel_flow_cost(player_ptr, pos);
    }

    const auto base_cost = (current_cost % TRAVEL_UNABLE);
    return base_cost + add_cost;
}

/*!
 * @brief トラベルの次の移動方向を決定する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param prev_dir 前回移動を行った方向
 * @param costs トラベルの目標到達地点までの行程
 * @return 次の方向
 */
Direction decide_travel_step_dir(PlayerType *player_ptr, const Direction &prev_dir, std::span<const std::array<int, MAX_WID>, MAX_HGT> costs)
{
    if (!prev_dir) {
        return Direction::none();
    }

    const auto &blindness = player_ptr->effects()->blindness();
    if (blindness.is_blind() || no_lite(player_ptr)) {
        msg_print(_("目が見えない！", "You cannot see!"));
        return Direction::none();
    }

    const auto p_pos = player_ptr->get_position();
    auto &floor = *player_ptr->current_floor_ptr;
    const auto &p_grid = floor.get_grid(p_pos);
    if ((disturb_trap_detect || alert_trap_detect) && player_ptr->dtrap && none_bits(p_grid.info, CAVE_IN_DETECT)) {
        player_ptr->dtrap = false;
        if (none_bits(p_grid.info, CAVE_UNSAFE)) {
            if (alert_trap_detect) {
                msg_print(_("* 注意:この先はトラップの感知範囲外です！ *", "*Leaving trap detect region!*"));
            }

            if (disturb_trap_detect) {
                return Direction::none();
            }
        }
    }

    const auto max = prev_dir.is_diagonal() ? 2 : 1;
    for (auto i = -max; i <= max; i++) {
        const auto dir = prev_dir.rotated_45degree(i);
        const auto pos = player_ptr->get_neighbor(dir);
        const auto &grid = floor.get_grid(pos);
        if (grid.has_monster()) {
            const auto &monster = floor.m_list[grid.m_idx];
            if (monster.ml) {
                return Direction::none();
            }
        }
    }

    auto cost = costs[p_pos.y][p_pos.x];
    auto new_direction = Direction::none();
    for (const auto &d : Direction::directions_8()) {
        const auto pos_neighbor = p_pos + d.vec();
        const auto dir_cost = costs[pos_neighbor.y][pos_neighbor.x];
        if (dir_cost < cost) {
            new_direction = d;
            cost = dir_cost;
        }
    }

    if (!new_direction) {
        return Direction::none();
    }

    const auto pos_new = p_pos + new_direction.vec();
    if (!easy_open && floor.has_closed_door_at(pos_new)) {
        return Direction::none();
    }

    const auto &grid_new = floor.get_grid(pos_new);
    if (!grid_new.mimic && !trap_can_be_ignored(player_ptr, grid_new.feat)) {
        return Direction::none();
    }

    return new_direction;
}
}

Travel &Travel::get_instance()
{
    static Travel instance{};
    return instance;
}

bool Travel::can_travel_to(const FloorType &floor, const Pos2D &pos)
{
    const auto &grid = floor.get_grid(pos);
    const auto &terrain = grid.get_terrain();
    const auto is_marked = grid.is_mark();
    const auto is_wall = terrain.flags.has_any_of({ TerrainCharacteristics::WALL, TerrainCharacteristics::CAN_DIG });
    const auto is_door = terrain.flags.has(TerrainCharacteristics::DOOR) && (grid.mimic > 0);

    return !is_marked || (!is_wall && !is_door);
}

const tl::optional<Pos2D> &Travel::get_goal() const
{
    return this->pos_goal;
}

void Travel::set_goal(PlayerType *player_ptr, const Pos2D &pos)
{
    this->pos_goal = pos;

    this->dir = Direction::none();
    const auto p_pos = player_ptr->get_position();
    auto dx = std::abs(p_pos.x - pos.x);
    auto dy = std::abs(p_pos.y - pos.y);
    auto sx = ((pos.x == p_pos.x) || (dx < dy)) ? 0 : ((pos.x > p_pos.x) ? 1 : -1);
    auto sy = ((pos.y == p_pos.y) || (dy < dx)) ? 0 : ((pos.y > p_pos.y) ? 1 : -1);
    for (const auto &d : Direction::directions()) {
        if (Pos2DVec(sy, sx) == d.vec()) {
            this->dir = d;
        }
    }

    this->forget_flow();
    this->update_flow(player_ptr);
    this->state = TravelState::STANDBY_TO_EXECUTE;
}

void Travel::reset_goal()
{
    this->pos_goal.reset();
    this->state = TravelState::STOP;
    this->forget_flow();
}

bool Travel::is_ongoing() const
{
    return this->state == TravelState::STANDBY_TO_EXECUTE || this->state == TravelState::EXECUTING;
}

void Travel::stop()
{
    this->state = TravelState::STOP;
}

int Travel::get_cost(const Pos2D &pos) const
{
    return this->costs[pos.y][pos.x];
}

/*!
 * @brief トラベル機能の実装 /
 * Travel command
 * @param player_ptr	プレイヤーへの参照ポインタ
 */
void Travel::step(PlayerType *player_ptr)
{
    this->dir = decide_travel_step_dir(player_ptr, this->dir, this->costs);
    if (!this->dir) {
        if (this->state != TravelState::EXECUTING) {
            msg_print(_("道筋が見つかりません！", "No route is found!"));
            this->reset_goal();
        }

        disturb(player_ptr, false, true);
        return;
    }

    PlayerEnergy(player_ptr).set_player_turn_energy(100);
    exe_movement(player_ptr, this->dir, always_pickup, false);
    if (player_ptr->get_position() == this->get_goal()) {
        this->reset_goal();
    } else {
        this->state = TravelState::EXECUTING;
    }
}

/*!
 * @brief トラベルの目標到達地点までの行程を得る
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void Travel::update_flow(PlayerType *player_ptr)
{
    if (!this->pos_goal) {
        return;
    }

    const auto &terrain = player_ptr->current_floor_ptr->get_grid(player_ptr->get_position()).get_terrain();
    const auto wall = terrain.flags.has(TerrainCharacteristics::MOVE);

    using CostAndPos = std::pair<int, Pos2D>;
    auto compare = [](const auto &a, const auto &b) { return a.first > b.first; }; // costが小さいものを優先
    std::priority_queue<CostAndPos, std::vector<CostAndPos>, decltype(compare)> pq(compare);

    this->costs[this->pos_goal->y][this->pos_goal->x] = 1;
    pq.emplace(1, *this->pos_goal);

    while (!pq.empty()) {
        // MSVCの警告対策のため構造化束縛を使わない
        const auto cost = pq.top().first;
        const auto pos = pq.top().second;
        pq.pop();

        for (const auto &d : Direction::directions_8()) {
            const auto pos_neighbor = pos + d.vec();
            auto &cost_neighbor = this->costs[pos_neighbor.y][pos_neighbor.x];
            const auto new_cost = travel_flow_aux(player_ptr, pos_neighbor, cost, wall);
            if (new_cost && *new_cost < cost_neighbor) {
                cost_neighbor = *new_cost;
                pq.emplace(cost_neighbor, pos_neighbor);
            }
        }
    }
}

/*!
 * @brief トラベル処理の記憶配列を初期化する Hack: forget the "flow" information
 * @param player_ptr	プレイヤーへの参照ポインタ
 */
void Travel::forget_flow()
{
    for (auto &row : this->costs) {
        row.fill(MAX_SHORT);
    }
}
