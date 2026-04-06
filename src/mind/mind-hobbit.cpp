#include "mind/mind-hobbit.h"
#include "floor/floor-object.h"
#include "sv-definition/sv-food-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

bool create_ration(CreatureEntity &creature)
{
    ItemEntity item({ ItemKindType::FOOD, SV_FOOD_RATION });
    (void)drop_near(creature, item, creature.get_position());
    msg_print(_("食事を料理して作った。", "You cook some food."));
    return true;
}
