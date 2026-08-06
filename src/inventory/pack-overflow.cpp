#include "inventory/pack-overflow.h"
#include "core/disturbance.h"
#include "core/stuff-handler.h"
#include "flavor/flavor-describer.h"
#include "floor/floor-object.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "object/object-info.h"
#include "player/player-status.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "view/display-messages.h"

/*!
 * @brief アイテムの所持種類数が超えた場合にアイテムを床に落とす処理
 */
void pack_overflow(CreatureEntity &creature)
{
    if (!creature.inventory[INVEN_PACK]->is_valid()) {
        return;
    }

    update_creature(creature);
    if (!creature.inventory[INVEN_PACK]->is_valid()) {
        return;
    }

    auto &item = *creature.inventory[INVEN_PACK];
    disturb(creature, false, true);
    msg_print(_("ザックからアイテムがあふれた！", "Your pack overflows!"));

    const auto item_name = describe_flavor(creature, item, 0);
    msg_format(_("%s(%c)を落とした。", "You drop %s (%c)."), item_name.data(), index_to_label(INVEN_PACK));
    (void)drop_near(creature, item, creature.get_position(), false);

    vary_item(creature, INVEN_PACK, -255);
    handle_stuff(creature);
}
