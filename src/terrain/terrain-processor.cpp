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
#include "system/creature-entity.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "util/bit-flags-calculator.h"
#include "util/point-2d.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief 噴火口地形の時間経過処理
 * @param creature クリーチャーへの参照
 * @param pos 地形の座標
 */
static void process_volcanic_crater(CreatureEntity &creature, const Pos2D &pos)
{
    // 1/100の確率で火炎ボールを発生
    if (randint0(100) != 0) {
        return;
    }

    // プレイヤーの視界内かチェック
    const auto &floor = *creature.get_floor();
    const bool in_sight = los(floor, creature.get_position(), pos);

    // プレイヤーの視界内であればメッセージ表示
    if (in_sight) {
        msg_print(_("噴火口から火炎が噴き出した！", "The volcanic crater erupts with flames!"));
    }

    // 火炎ボールの威力（ダンジョン階層に依存）
    const auto damage = 100 + floor.dun_level * 2;

    // 半径3の火炎ボール効果
    project(creature, -1, 3, pos.y, pos.x, damage, AttributeType::FIRE,
        (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP));
}

/*!
 * @brief 召喚陣地形の時間経過処理
 * @param creature クリーチャーへの参照
 * @param pos 地形の座標
 */
static void process_summoning_circle(CreatureEntity &creature, const Pos2D &pos)
{
    // 1/50の確率でモンスターを生成
    if (randint0(50) != 0) {
        return;
    }

    const auto &floor = *creature.get_floor();
    const auto &grid = floor.get_grid(pos);

    // 既にモンスターがいる場合は生成しない
    if (grid.has_monster()) {
        return;
    }

    // プレイヤーがその位置にいる場合は生成しない
    if (creature.is_located_at(pos)) {
        return;
    }

    // 召喚陣の位置にモンスターを生成（階層レベル準拠）
    auto m_idx = place_random_monster(creature, pos.y, pos.x, 0);

    if (m_idx) {
        // 生成に成功した場合、プレイヤーの視界内であればメッセージ表示
        if (los(floor, creature.get_position(), pos)) {
            msg_print(_("召喚陣からモンスターが現れた！", "A monster appears from the summoning circle!"));
        }
    }
}

/*!
 * @brief 地形のランダム変化処理
 * @param creature クリーチャーへの参照
 * @param pos 地形の座標
 * @param terrain 地形情報
 */
static void process_random_terrain_change(CreatureEntity &creature, const Pos2D &pos, const TerrainType &terrain)
{
    // 確率チェック
    if (terrain.random_change_prob <= 0) {
        return;
    }

    if (randint0(terrain.random_change_prob) != 0) {
        return;
    }

    // 変化先が無効な場合はスキップ
    if (terrain.random_change == 0) {
        return;
    }

    // 地形を変化させる
    set_terrain_id_to_grid(creature, pos, terrain.random_change);
}

/*!
 * @brief 地形の時間経過処理メイン関数
 * @param creature クリーチャーへの参照
 */
void process_terrain_effects(CreatureEntity &creature)
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

    auto &floor = *creature.get_floor();
    const auto &terrains = TerrainList::get_instance();
    const auto volcanic_crater_id = terrains.get_terrain_id(TerrainTag::VOLCANIC_CRATER);
    const auto summoning_circle_id = terrains.get_terrain_id(TerrainTag::SUMMONING_CIRCLE);

    // フロア内の全ての特殊地形を処理
    for (auto y = 1; y < MAX_HGT - 1; y++) {
        for (auto x = 1; x < MAX_WID - 1; x++) {
            const Pos2D pos(y, x);
            const auto &grid = floor.get_grid(pos);
            const auto feat_id = grid.feat;
            const auto &terrain = terrains.get_terrain(feat_id);

            // 特定の地形の特殊処理
            if (feat_id == volcanic_crater_id) {
                process_volcanic_crater(creature, pos);
            } else if (feat_id == summoning_circle_id) {
                process_summoning_circle(creature, pos);
            }

            // ランダム変化処理（全地形で可能）
            process_random_terrain_change(creature, pos, terrain);
        }
    }
}
