#include "mind/mind-warrior.h"
#include "cmd-action/cmd-attack.h"
#include "floor/geometry.h"
#include "spell-kind/spells-teleport.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

/*!
 * 戦士と盗賊における、ヒット＆アウェイのレイシャルパワー/突然変異
 * @param creature クリーチャーへの参照
 * @return コマンドの入力先にモンスターがいたらTRUE
 */
bool hit_and_away(CreatureEntity &creature)
{
    auto &player = static_cast<PlayerType &>(creature);
    const auto dir = get_direction(player);
    if (!dir) {
        return false;
    }

    const auto pos = creature.get_neighbor(dir);
    if (creature.current_floor_ptr->get_grid(pos).has_monster()) {
        do_cmd_attack(creature, pos.y, pos.x, HISSATSU_NONE);
        if (randint0(player.skill_dis) < 7) {
            msg_print(_("うまく逃げられなかった。", "You failed to run away."));
        } else {
            teleport_player(creature, 30, TELEPORT_SPONTANEOUS);
        }
        return true;
    }

    msg_print(_("その方向にはモンスターはいません。", "You don't see any monster in this direction"));
    msg_erase();
    return false;
}

/*!
 * 剣の舞い
 * @param creature クリーチャーへの参照
 * @return 常にTRUE
 */
bool sword_dancing(CreatureEntity &creature)
{
    for (auto i = 0; i < 6; i++) {
        const auto d = rand_choice(Direction::directions_8());
        const auto pos = creature.get_neighbor(d);
        const auto &grid = creature.current_floor_ptr->get_grid(pos);
        if (grid.has_monster()) {
            do_cmd_attack(creature, pos.y, pos.x, HISSATSU_NONE);
        } else {
            msg_print(_("攻撃が空をきった。", "You attack the empty air."));
        }
    }

    return true;
}
