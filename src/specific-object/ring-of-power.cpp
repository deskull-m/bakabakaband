#include "specific-object/ring-of-power.h"
#include "effect/attribute-types.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "player/player-status.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-sight.h"
#include "status/base-status.h"
#include "system/creature-entity.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

/*!
 * @brief 『一つの指輪』の効果処理 /
 * Hack -- activate the ring of power
 * @param creature クリーチャーへの参照
 * @param dir 発動の方向ID
 */
static void exe_ring_of_power(CreatureEntity &creature, const Direction &dir)
{
    switch (randint1(10)) {
    case 1:
    case 2:
        msg_print(_("あなたは悪性のオーラに包み込まれた。", "You are surrounded by a malignant aura."));
        sound(SoundKind::EVIL);
        (void)dec_stat(creature, A_STR, 50, true);
        (void)dec_stat(creature, A_INT, 50, true);
        (void)dec_stat(creature, A_WIS, 50, true);
        (void)dec_stat(creature, A_DEX, 50, true);
        (void)dec_stat(creature, A_CON, 50, true);
        (void)dec_stat(creature, A_CHR, 50, true);
        creature.exp -= (creature.exp / 4);
        creature.max_exp -= (creature.exp / 4);
        check_experience(creature);
        break;
    case 3:
        msg_print(_("あなたは強力なオーラに包み込まれた。", "You are surrounded by a powerful aura."));
        dispel_monsters(creature, 1000);
        break;
    case 4:
    case 5:
    case 6:
        fire_ball(creature, AttributeType::MANA, dir, 600, 3);
        break;
    case 7:
    case 8:
    case 9:
    case 10:
        fire_bolt(creature, AttributeType::MANA, dir, 500);
        break;
    default:
        break;
    }
}

bool activate_ring_of_power(CreatureEntity &creature, std::string_view name)
{
    msg_format(_("%sは漆黒に輝いた...", "The %s glows intensely black..."), name.data());
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    exe_ring_of_power(creature, dir);
    return true;
}
