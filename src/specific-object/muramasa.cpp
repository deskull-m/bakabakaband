#include "specific-object/muramasa.h"
#include "artifact/fixed-art-types.h"
#include "core/asking-player.h"
#include "spell/spells-object.h"
#include "status/base-status.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "view/display-messages.h"

bool activate_muramasa(CreatureEntity &creature, ItemEntity &item)
{
    if (!item.is_specific_artifact(FixedArtifactId::MURAMASA)) {
        return false;
    }

    if (!input_check(_("本当に使いますか？", "Are you sure?! "))) {
        return true;
    }

    msg_print(_("村正が震えた．．．", "The Muramasa pulsates..."));
    do_inc_stat(creature, A_STR);
    if (one_in_(2)) {
        msg_print(_("村正は壊れた！", "The Muramasa is destroyed!"));
        curse_weapon_object(creature, true, item);
    }

    return true;
}
