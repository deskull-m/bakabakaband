#include "view/display-birth.h"
#include "birth/auto-roller.h"
#include "birth/birth-stat.h"
#include "game-option/birth-options.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "player/player-personality.h"
#include "player/player-status.h"
#include "system/creature-entity.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"

/*!
 * @brief オートロール中のステータスを表示する / Display stat values, subset of "put_stats()"
 * @details See 'display_player(creature, )' for screen layout constraints.
 */
void birth_put_stats(CreatureEntity &creature)
{
    if (!autoroller) {
        return;
    }

    const int col = 22;
    for (int i = 0; i < A_MAX; i++) {
        int j = creature.race->r_adj[i] + (*creature.pclass_ref).c_adj[i] + (*creature.personality).a_adj[i];
        int m = adjust_stat(creature.stat_max[i], j);
        c_put_str(TERM_L_GREEN, cnv_stat(m), 3 + i, col + 24);
    }
}
