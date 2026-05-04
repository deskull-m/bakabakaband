/*!
 * @todo いずれcmd-item.c/h に統合したい
 * @brief プレイヤーの読むコマンド実装
 * @date 2018/09/07
 * @author deskull
 */

#include "cmd-item/cmd-read.h"
#include "action/action-limited.h"
#include "floor/floor-object.h"
#include "object-hook/hook-expendable.h"
#include "object-use/read/read-execution.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player-info/samurai-data-type.h"
#include "player/attack-defense-types.h"
#include "player/special-defense-types.h"
#include "status/action-setter.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "world/world.h"

/*!
 * @brief 読むコマンドのメインルーチン /
 * Eat some food (from the pack or floor)
 */
void do_cmd_read_scroll(CreatureEntity &creature)
{
    if (AngbandWorld::get_instance().is_wild_mode() || cmd_limit_arena(creature)) {
        return;
    }

    CreatureClass(creature).break_samurai_stance({ SamuraiStanceType::MUSOU, SamuraiStanceType::KOUKIJIN });

    if (cmd_limit_blind(creature) || cmd_limit_confused(creature)) {
        return;
    }

    constexpr auto q = _("どの巻物を読みますか? ", "Read which scroll? ");
    constexpr auto s = _("読める巻物がない。", "You have no scrolls to read.");
    const auto &[item, i_idx] = choose_item(creature, q, s, USE_INVEN | USE_FLOOR, FuncItemTester(&ItemEntity::is_readable));
    if (!item) {
        return;
    }

    ObjectReadEntity(creature, i_idx).execute(item->is_aware());
}
