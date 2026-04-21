#include "spell-kind/spells-charm.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "spell-kind/spells-launcher.h"
#include "system/creature-entity.h"

/*!
 * @brief チャーム・モンスター(1体)
 * @param creature クリーチャーへの参照
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev パワー
 * @return 作用が実際にあった場合TRUEを返す
 */
bool charm_monster(CreatureEntity &creature, const Direction &dir, PLAYER_LEVEL plev)
{
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL;
    return project_hook(creature, AttributeType::CHARM, dir, plev, flg);
}

/*!
 * @brief アンデッド支配(1体)
 * @param creature クリーチャーへの参照
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev パワー
 * @return 作用が実際にあった場合TRUEを返す
 */
bool control_one_undead(CreatureEntity &creature, const Direction &dir, PLAYER_LEVEL plev)
{
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL;
    return project_hook(creature, AttributeType::CONTROL_UNDEAD, dir, plev, flg);
}

/*!
 * @brief 悪魔支配(1体)
 * @param creature クリーチャーへの参照
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev パワー
 * @return 作用が実際にあった場合TRUEを返す
 */
bool control_one_demon(CreatureEntity &creature, const Direction &dir, PLAYER_LEVEL plev)
{
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL;
    return project_hook(creature, AttributeType::CONTROL_DEMON, dir, plev, flg);
}

/*!
 * @brief 動物支配(1体)
 * @param creature クリーチャーへの参照
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev パワー
 * @return 作用が実際にあった場合TRUEを返す
 */
bool charm_animal(CreatureEntity &creature, const Direction &dir, PLAYER_LEVEL plev)
{
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL;
    return project_hook(creature, AttributeType::CONTROL_ANIMAL, dir, plev, flg);
}
