#include "object-activation/activation-genocide.h"
#include "spell-kind/spells-genocide.h"
#include "system/creature-entity.h"
#include "view/display-messages.h"

bool activate_genocide(CreatureEntity &creature)
{
    msg_print(_("深青色に輝いている...", "It glows deep blue..."));
    (void)symbol_genocide(creature, 200, true);
    return true;
}

bool activate_mass_genocide(CreatureEntity &creature)
{
    msg_print(_("ひどく鋭い音が流れ出た...", "It lets out a long, shrill note..."));
    (void)mass_genocide(creature, 200, true);
    return true;
}
