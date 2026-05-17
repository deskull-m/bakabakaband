#include "specific-object/death-crimson.h"
#include "artifact/fixed-art-types.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/geometry.h"
#include "player-base/player-class.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

/*!
 * @brief クリムゾンを発射する / Fire Crimson, evoluting gun.
 * @param creature クリーチャーへの参照
 * @return キャンセルした場合 false.
 * @details
 * Need to analyze size of the window.
 * Need more color coding.
 */
static bool fire_crimson(CreatureEntity &creature)
{
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    const auto [ty, tx] = dir.get_target_position(creature.get_position(), 99);

    int num = 1;
    if (CreatureClass(creature).equals(PlayerClassType::ARCHER)) {
        if (creature.get_level() >= 10) {
            num++;
        }

        if (creature.get_level() >= 30) {
            num++;
        }

        if (creature.get_level() >= 45) {
            num++;
        }
    }

    BIT_FLAGS flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
    for (int i = 0; i < num; i++) {
        (void)project(creature, 0, creature.get_level() / 20 + 1, ty, tx, creature.get_level() * creature.get_level() * 6 / 50, AttributeType::ROCKET, flg);
    }

    return true;
}

bool activate_crimson(CreatureEntity &creature, ItemEntity &item)
{
    if (!item.is_specific_artifact(FixedArtifactId::CRIMSON)) {
        return false;
    }

    msg_print(_("せっかくだから『クリムゾン』をぶっぱなすぜ！", "I'll fire CRIMSON! SEKKAKUDAKARA!"));
    return fire_crimson(creature);
}
