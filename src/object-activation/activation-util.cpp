#include "object-activation/activation-util.h"
#include "object/object-info.h"
#include "system/baseitem-info.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"

ae_type *initialize_ae_type(PlayerType *player_ptr, ae_type *ae_ptr, const INVENTORY_IDX item)
{
    ae_ptr->o_ptr = ref_item(player_ptr, item);
    ae_ptr->lev = ae_ptr->o_ptr->get_baseitem().level;
    ae_ptr->broken = 0;
    return ae_ptr;
}
