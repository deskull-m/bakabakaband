/*!
 * @file blue-magic-status.cpp
 * @brief 青魔法の状態異常系スペル定義
 */

#include "blue-magic/blue-magic-status.h"
#include "system/creature-entity.h"
#include "blue-magic/blue-magic-util.h"
#include "spell/spells-status.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

bool cast_blue_scare(CreatureEntity &creature, bmc_type *bmc_ptr)
{
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    auto &player = static_cast<PlayerType &>(creature);
    msg_print(_("恐ろしげな幻覚を作り出した。", "You cast a fearful illusion."));
    fear_monster(player, dir, bmc_ptr->plev + 10);
    return true;
}

bool cast_blue_blind(CreatureEntity &creature, bmc_type *bmc_ptr)
{
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    auto &player = static_cast<PlayerType &>(creature);
    confuse_monster(player, dir, bmc_ptr->plev * 2);
    return true;
}

bool cast_blue_confusion(CreatureEntity &creature, bmc_type *bmc_ptr)
{
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    auto &player = static_cast<PlayerType &>(creature);
    msg_print(_("誘惑的な幻覚をつくり出した。", "You cast a mesmerizing illusion."));
    confuse_monster(player, dir, bmc_ptr->plev * 2);
    return true;
}

bool cast_blue_slow(CreatureEntity &creature, bmc_type *bmc_ptr)
{
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    auto &player = static_cast<PlayerType &>(creature);
    slow_monster(player, dir, bmc_ptr->plev);
    return true;
}

bool cast_blue_sleep(CreatureEntity &creature, bmc_type *bmc_ptr)
{
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    auto &player = static_cast<PlayerType &>(creature);
    sleep_monster(player, dir, bmc_ptr->plev);
    return true;
}
