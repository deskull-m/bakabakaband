#include "object-activation/activation-util.h"
#include "object/object-info.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"

ae_type *initialize_ae_type(CreatureEntity &player, ae_type *ae_ptr, const INVENTORY_IDX i_idx)
{
    auto item = ref_item(player, i_idx);
    ae_ptr->o_ptr = item.get();
    ae_ptr->lev = ae_ptr->o_ptr->get_baseitem_level();
    ae_ptr->broken = 0;
    return ae_ptr;
}
