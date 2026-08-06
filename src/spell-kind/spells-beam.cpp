#include "spell-kind/spells-beam.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "spell-kind/spells-launcher.h"
#include "system/creature-entity.h"

/*!
 * @brief 岩石溶解処理
 * @param creature クリーチャーへの参照
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool wall_to_mud(CreatureEntity &creature, const Direction &dir, int dam)
{
    BIT_FLAGS flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
    return project_hook(creature, AttributeType::KILL_WALL, dir, dam, flg);
}

/*!
 * @brief 魔法の施錠処理
 * @param creature クリーチャーへの参照
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool wizard_lock(CreatureEntity &creature, const Direction &dir)
{
    BIT_FLAGS flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
    return project_hook(creature, AttributeType::JAM_DOOR, dir, 20 + randint1(30), flg);
}

/*!
 * @brief ドア破壊処理
 * @param creature クリーチャーへの参照
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool destroy_door(CreatureEntity &creature, const Direction &dir)
{
    BIT_FLAGS flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM;
    return project_hook(creature, AttributeType::KILL_DOOR, dir, 0, flg);
}

/*!
 * @brief トラップ解除処理
 * @param creature クリーチャーへの参照
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool disarm_trap(CreatureEntity &creature, const Direction &dir)
{
    BIT_FLAGS flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM;
    return project_hook(creature, AttributeType::KILL_TRAP, dir, 0, flg);
}
