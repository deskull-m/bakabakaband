#include "object-activation/activation-util.h"
#include "object-enchant/activation-info-table.h"
#include "object-enchant/object-ego.h"
#include "object/object-info.h"
#include "system/artifact-type-definition.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"

ae_type::ae_type(CreatureEntity &player, short i_idx)
{
    this->item = ref_item(player, i_idx);
    if (this->item) {
        this->lev = this->item->get_baseitem_level();
    }
}

void ae_type::decide_activation_level()
{
    if (this->item->is_fixed_artifact()) {
        this->lev = this->item->get_fixed_artifact().level;
        return;
    }

    if (this->item->is_random_artifact()) {
        const auto it_activation = this->item->find_activation_info();
        if (it_activation != activation_info.end()) {
            this->lev = it_activation->level;
        }

        return;
    }

    const auto tval = this->item->bi_key.tval();
    if (((tval == ItemKindType::RING) || (tval == ItemKindType::AMULET)) && this->item->is_ego()) {
        this->lev = this->item->get_ego().level;
        this->broken = egos_info[this->item->ego_idx].broken_rate;
    }
}
