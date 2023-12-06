﻿#include "object-activation/activation-util.h"
#include "object/object-info.h"
#include "system/baseitem-info-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"

ae_type *initialize_ae_type(PlayerType *player_ptr, ae_type *ae_ptr, const INVENTORY_IDX item)
{
    ae_ptr->o_ptr = ref_item(player_ptr, item);
    ae_ptr->lev = k_info[ae_ptr->o_ptr->k_idx].level;
    ae_ptr->broken = 0;
    return ae_ptr;
}
