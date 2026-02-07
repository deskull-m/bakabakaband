#include "load/load-zangband.h"
#include "load/load-util.h"
#include "player/attack-defense-types.h"
#include "system/player-type-definition.h"

void set_zangband_action(PlayerType *player_ptr)
{
    if (rd_byte() != 0) {
        player_ptr->action = ACTION_LEARN;
    }
}
