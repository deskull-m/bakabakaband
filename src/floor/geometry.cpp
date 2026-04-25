#include "floor/geometry.h"
#include "game-option/text-display-options.h"
#include "system/angband-system.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "target/projection-path-calculator.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief 指定された座標をプレイヤーが視覚に収められるかを返す。 / Can the player "see" the given grid in detail?
 * @param y y座標
 * @param x x座標
 * @return 視覚に収められる状態ならTRUEを返す
 * @details
 * He must have vision, illumination, and line of sight.\n
 * \n
 * Note -- "CAVE_LITE" is only set if the "torch" has "los()".\n
 * So, given "CAVE_LITE", we know that the grid is "fully visible".\n
 *\n
 * Note that "CAVE_GLOW" makes little sense for a wall, since it would mean\n
 * that a wall is visible from any direction.  That would be odd.  Except\n
 * under wizard light, which might make sense.  Thus, for walls, we require\n
 * not only that they be "CAVE_GLOW", but also, that they be adjacent to a\n
 * grid which is not only "CAVE_GLOW", but which is a non-wall, and which is\n
 * in line of sight of the player.\n
 *\n
 * This extra check is expensive, but it provides a more "correct" semantics.\n
 *\n
 * Note that we should not run this check on walls which are "outer walls" of\n
 * the dungeon, or we will induce a memory fault, but actually verifying all\n
 * of the locations would be extremely expensive.\n
 *\n
 * Thus, to speed up the function, we assume that all "perma-walls" which are\n
 * "CAVE_GLOW" are "illuminated" from all sides.  This is correct for all cases\n
 * except "vaults" and the "buildings" in town.  But the town is a hack anyway,\n
 * and the player has more important things on his mind when he is attacking a\n
 * monster vault.  It is annoying, but an extremely important optimization.\n
 *\n
 * Note that "glowing walls" are only considered to be "illuminated" if the\n
 * grid which is next to the wall in the direction of the player is also a\n
 * "glowing" grid.  This prevents the player from being able to "see" the\n
 * walls of illuminated rooms from a corridor outside the room.\n
 */
bool player_can_see_bold(CreatureEntity &creature, POSITION y, POSITION x)
{
    /* Blind players see nothing */
    if (creature.is_blind()) {
        return false;
    }

    const Pos2D pos(y, x);
    const auto &floor = *creature.get_floor();
    const auto &grid = floor.get_grid(pos);

    /* Note that "torch-lite" yields "illumination" */
    if (grid.info & (CAVE_LITE | CAVE_MNLT)) {
        return true;
    }

    /* Require line of sight to the grid */
    if (!grid.has_los()) {
        return false;
    }

    /* Noctovision of Ninja */
    if (creature.has_see_nocto()) {
        return true;
    }

    /* Require "perma-lite" of the grid */
    if ((grid.info & (CAVE_GLOW | CAVE_MNDK)) != CAVE_GLOW) {
        return false;
    }

    /* Feature code (applying "mimic" field) */
    /* Floors are simple */
    if (grid.has_los_terrain(TerrainKind::MIMIC)) {
        return true;
    }

    /* Check for "local" illumination */
    return floor.is_illuminated_at(creature.get_position(), pos);
}

/*
 * Calculate "incremental motion". Used by project() and shoot().
 * Assumes that (*y,*x) lies on the path from (y1,x1) to (y2,x2).
 */
Pos2D mmove2(const Pos2D &pos_orig, const Pos2D &pos1, const Pos2D &pos2)
{
    /* Extract the distance travelled */
    auto dy = (pos_orig.y < pos1.y) ? pos1.y - pos_orig.y : pos_orig.y - pos1.y;
    auto dx = (pos_orig.x < pos1.x) ? pos1.x - pos_orig.x : pos_orig.x - pos1.x;

    /* Number of steps */
    auto dist = (dy > dx) ? dy : dx;

    /* We are calculating the next location */
    dist++;

    /* Calculate the total distance along each axis */
    dy = (pos2.y < pos1.y) ? (pos1.y - pos2.y) : (pos2.y - pos1.y);
    dx = (pos2.x < pos1.x) ? (pos1.x - pos2.x) : (pos2.x - pos1.x);

    /* Paranoia -- Hack -- no motion */
    if (!dy && !dx) {
        return pos_orig;
    }

    /* Move mostly vertically */
    if (dy > dx) {
        /* Extract a shift factor */
        auto shift = (dist * dx + (dy - 1) / 2) / dy;

        /* Sometimes move along the minor axis, Always move along major axis */
        const auto y = (pos2.y < pos1.y) ? (pos1.y - dist) : (pos1.y + dist);
        const auto x = (pos2.x < pos1.x) ? (pos1.x - shift) : (pos1.x + shift);
        return { y, x };
    }

    /* Move mostly horizontally */
    auto shift = (dist * dy + (dx - 1) / 2) / dx;
    const auto y = (pos2.y < pos1.y) ? (pos1.y - shift) : (pos1.y + shift);
    const auto x = (pos2.x < pos1.x) ? (pos1.x - dist) : (pos1.x + dist);
    return { y, x };
}

/*!
 * @brief Is the monster seen by the player?
 * @param creature クリーチャーへの参照
 * @param m_ptr 個々のモンスターへの参照ポインタ
 * @return 個々のモンスターがプレイヤーが見えたらTRUE
 * @todo is_seen() の関数マクロをバラそうとしたがインクルード関係のコンパイルエラーで失敗
 */
bool is_seen(CreatureEntity &creature, const CreatureEntity &target)
{
    auto is_inside_view = !ignore_unview;
    is_inside_view |= AngbandSystem::get_instance().is_phase_out();
    const auto p_pos = creature.get_position();
    const auto t_pos = target.get_position();
    is_inside_view |= player_can_see_bold(creature, t_pos.y, t_pos.x) && projectable(*creature.get_floor(), p_pos, t_pos);
    const auto ml = target.has_monster_profile() ? target.get_monster_profile().ml : false;
    return ml && is_inside_view;
}
