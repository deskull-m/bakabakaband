#include "player/player-known-flags.h"
#include "inventory/inventory-slot-types.h"
#include "player/permanent-resistances.h"
#include "player/race-resistances.h"
#include "player/temporary-resistances.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"

/*!
 * @brief プレイヤーの特性を供給源ごとに集める
 * @param creature クリーチャーへの参照
 * @return 装備 (鑑定済み)・常時・一時・免疫・弱点・乗馬の特性
 */
PlayerKnownFlags collect_player_known_flags(CreatureEntity &creature)
{
    PlayerKnownFlags result;
    for (const auto slot : INVEN_WIELDING_SLOTS) {
        result.equipment.set(creature.inventory[slot]->get_flags_known());
    }
    player_flags(creature, result.permanent);
    tim_player_flags(creature, result.temporary);
    player_immunity(creature, result.immunity);
    tim_player_immunity(creature, result.temporary_immunity);
    player_vulnerability_flags(creature, result.vulnerability);
    riding_flags(creature, result.riding, result.riding_negative);
    return result;
}
