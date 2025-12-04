/*!
 * @brief 地形の時間経過処理実装
 * @author Hourier
 * @date 2025/10/05
 */

#include "terrain/terrain-processor.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "effect/spells-effect-util.h"
#include "floor/line-of-sight.h"
#include "grid/grid.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "spell/spells-util.h"
#include "system/angband-system.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "util/bit-flags-calculator.h"
#include "util/point-2d.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief 噴火口地形の時間経過処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pos 地形の座標
 */
static void process_volcanic_crater(PlayerType *player_ptr, const Pos2D &pos)
{
    // 1/100の確率で火炎ボールを発生
    if (randint0(100) != 0) {
        return;
    }

    // プレイヤーの視界内かチェック
    const auto &floor = *player_ptr->current_floor_ptr;
    const bool in_sight = los(floor, player_ptr->get_position(), pos);

    // プレイヤーの視界内であればメッセージ表示
    if (in_sight) {
        msg_print(_("噴火口から火炎が噴き出した！", "The volcanic crater erupts with flames!"));
    }

    // 火炎ボールの威力（ダンジョン階層に依存）
    const auto damage = 100 + floor.dun_level * 2;

    // 半径3の火炎ボール効果
    project(player_ptr, -1, 3, pos.y, pos.x, damage, AttributeType::FIRE,
        (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP));
}

/*!
 * @brief 召喚陣地形の時間経過処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pos 地形の座標
 */
static void process_summoning_circle(PlayerType *player_ptr, const Pos2D &pos)
{
    // 1/50の確率でモンスターを生成
    if (randint0(50) != 0) {
        return;
    }

    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &grid = floor.get_grid(pos);

    // 既にモンスターがいる場合は生成しない
    if (grid.has_monster()) {
        return;
    }

    // プレイヤーがその位置にいる場合は生成しない
    if (player_ptr->is_located_at(pos)) {
        return;
    }

    // 召喚陣の位置にモンスターを生成（階層レベル準拠）
    auto m_idx = place_random_monster(player_ptr, pos.y, pos.x, 0);

    if (m_idx) {
        // 生成に成功した場合、プレイヤーの視界内であればメッセージ表示
        if (los(floor, player_ptr->get_position(), pos)) {
            msg_print(_("召喚陣からモンスターが現れた！", "A monster appears from the summoning circle!"));
        }
    }
}

/*!
 * @brief 地形の時間経過処理メイン関数
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void process_terrain_effects(PlayerType *player_ptr)
{
    const auto &world = AngbandWorld::get_instance();

    // 10ターンに1回の処理
    if (world.game_turn % 10 != 0) {
        return;
    }

    // フェーズアウト中や荒野モードでは処理しない
    if (AngbandSystem::get_instance().is_phase_out() || world.is_wild_mode()) {
        return;
    }

    auto &floor = *player_ptr->current_floor_ptr;
    const auto &terrains = TerrainList::get_instance();
    const auto volcanic_crater_id = terrains.get_terrain_id(TerrainTag::VOLCANIC_CRATER);
    const auto summoning_circle_id = terrains.get_terrain_id(TerrainTag::SUMMONING_CIRCLE);

    // フロア内の全ての特殊地形を処理
    for (auto y = 1; y < MAX_HGT - 1; y++) {
        for (auto x = 1; x < MAX_WID - 1; x++) {
            const Pos2D pos(y, x);
            const auto &grid = floor.get_grid(pos);

            if (grid.feat == volcanic_crater_id) {
                process_volcanic_crater(player_ptr, pos);
            } else if (grid.feat == summoning_circle_id) {
                process_summoning_circle(player_ptr, pos);
            }
        }
    }
}
