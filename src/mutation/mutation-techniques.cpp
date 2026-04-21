/*!
 * @brief 突然変異でのみ得ることができる特殊能力処理
 * @date 2020/07/04
 * @author Hourier
 */

#include "mutation/mutation-techniques.h"
#include "cmd-action/cmd-attack.h"
#include "floor/geometry.h"
#include "grid/grid.h"
#include "monster/monster-info.h"
#include "player/digestion-processor.h"
#include "player/player-move.h"
#include "player/player-status.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "target/target-getter.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * 岩石食い
 * @param creature クリーチャーへの参照
 * @return コマンドの入力方向に地形があればTRUE
 */
bool eat_rock(CreatureEntity &creature)
{
    const auto dir = get_direction(creature);
    if (!dir) {
        return false;
    }

    const auto pos = creature.get_neighbor(dir);
    const auto &grid = creature.get_floor()->get_grid(pos);
    const auto &terrain = grid.get_terrain();
    const auto &terrain_mimic = grid.get_terrain(TerrainKind::MIMIC);

    stop_mouth(creature);
    if (terrain_mimic.flags.has_not(TerrainCharacteristics::STONE)) {
        msg_print(_("この地形は食べられない。", "You cannot eat this feature."));
    } else if (terrain.flags.has(TerrainCharacteristics::PERMANENT)) {
        msg_format(_("いてっ！この%sはあなたの歯より硬い！", "Ouch!  This %s is harder than your teeth!"), terrain_mimic.name.data());
    } else if (grid.has_monster()) {
        const auto &monster = creature.get_floor()->get_monster(grid.m_idx);
        msg_print(_("何かが邪魔しています！", "There's something in the way!"));
        if (!monster.get_monster_profile().ml || !monster.is_pet()) {
            do_cmd_attack(creature, pos.y, pos.x, HISSATSU_NONE);
        }
    } else if (terrain.flags.has(TerrainCharacteristics::TREE)) {
        msg_print(_("木の味は好きじゃない！", "You don't like the woody taste!"));
    } else if (terrain.flags.has(TerrainCharacteristics::GLASS)) {
        msg_print(_("ガラスの味は好きじゃない！", "You don't like the glassy taste!"));
    } else if (terrain.flags.has(TerrainCharacteristics::DOOR) || terrain.flags.has(TerrainCharacteristics::CAN_DIG)) {
        (void)set_food(creature, creature.food + 3000);
    } else if (terrain.flags.has(TerrainCharacteristics::MAY_HAVE_GOLD) || terrain.flags.has(TerrainCharacteristics::HAS_GOLD)) {
        (void)set_food(creature, creature.food + 5000);
    } else {
        msg_format(_("この%sはとてもおいしい！", "This %s is very filling!"), terrain_mimic.name.data());
        (void)set_food(creature, creature.food + 10000);
    }

    cave_alter_feat(creature, pos.y, pos.x, TerrainCharacteristics::STONE);
    (void)move_player_effect(creature, pos.y, pos.x, MPE_DONT_PICKUP);
    return true;
}
