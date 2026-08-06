#pragma once

enum class WishResultType {
    FAIL = -1,
    NOTHING = 0,
    NORMAL = 1,
    EGO = 2,
    ARTIFACT = 3,
    MAX
};

class CreatureEntity;
void wizard_item_modifier(CreatureEntity &creature);
void wiz_modify_item(CreatureEntity &creature);
WishResultType do_cmd_wishing(CreatureEntity &creature, int prob, bool art_ok, bool ego_ok, bool confirm);
void wiz_identify_full_inventory(CreatureEntity &creature);
