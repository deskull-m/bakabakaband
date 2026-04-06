#include "racial/racial-vampire.h"
#include "dungeon/dungeon-flag-types.h"
#include "floor/geometry.h"
#include "hpmp/hp-mp-processor.h"
#include "player/digestion-processor.h"
#include "player/player-status.h"
#include "spell-kind/spells-specific-bolt.h"
#include "system/creature-entity.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

bool vampirism(CreatureEntity &creature)
{
    const auto &floor = *creature.current_floor_ptr;
    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::NO_MELEE)) {
        msg_print(_("なぜか攻撃することができない。", "Something prevents you from attacking."));
        return false;
    }

    const auto dir = get_direction(creature);
    if (!dir) {
        return false;
    }

    const auto pos = creature.get_neighbor(dir);
    const auto &grid = floor.get_grid(pos);
    stop_mouth(creature);
    if (!grid.has_monster()) {
        msg_print(_("何もない場所に噛みついた！", "You bite into thin air!"));
        return false;
    }

    msg_print(_("あなたはニヤリとして牙をむいた...", "You grin and bare your fangs..."));

    int dummy = creature.level * 2;
    if (!hypodynamic_bolt(creature, dir, dummy)) {
        msg_print(_("げぇ！ひどい味だ。", "Yechh. That tastes foul."));
        return true;
    }

    if (creature.food < PY_FOOD_FULL) {
        (void)hp_player(creature, dummy);
    } else {
        msg_print(_("あなたは空腹ではありません。", "You were not hungry."));
    }

    /* Gain nutritional sustenance: 150/hp drained */
    /* A Food ration gives 5000 food points (by contrast) */
    /* Don't ever get more than "Full" this way */
    /* But if we ARE Gorged,  it won't cure us */
    dummy = creature.food + std::min(5000, 100 * dummy);
    if (creature.food < PY_FOOD_MAX) { /* Not gorged already */
        (void)set_food(creature, dummy >= PY_FOOD_MAX ? PY_FOOD_MAX - 1 : dummy);
    }

    return true;
}
