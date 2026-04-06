/*!
 * @file racial-execution.cpp
 * @brief レイシャルパワー実行処理実装
 */

#include "action/racial-execution.h"
#include "system/creature-entity.h"
#include "action/action-limited.h"
#include "artifact/fixed-art-types.h"
#include "core/asking-player.h"
#include "game-option/disturbance-options.h"
#include "inventory/inventory-slot-types.h"
#include "player-base/player-class.h"
#include "player-info/race-info.h"
#include "player-status/player-energy.h"
#include "racial/racial-switcher.h"
#include "racial/racial-util.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"

/*!
 * @brief レイシャル・パワー発動処理
 * @param creature クリーチャーへの参照
 * @param command 発動するレイシャルのID
 * @return 処理を実際に実行した場合はTRUE、キャンセルした場合FALSEを返す。
 */
bool exe_racial_power(CreatureEntity &creature, const int32_t command)
{
    if (command <= -3) {
        return switch_class_racial_execution(creature, command);
    }

    if (creature.mimic_form != MimicKindType::NONE) {
        return switch_mimic_racial_execution(creature);
    }

    return switch_race_racial_execution(creature, command);
}

/*!
 * @brief レイシャル・パワーの発動成功率を計算する / Returns the chance to activate a racial power/mutation
 * @param creature クリーチャーへの参照
 * @param rpi_ptr 発動したいレイシャル・パワー情報の構造体参照ポインタ
 * @return 成功率(%)を返す
 */
PERCENTAGE racial_chance(CreatureEntity &creature, rpi_type *rpi_ptr)
{
    if ((creature.level < rpi_ptr->min_level) || creature.is_confused()) {
        return 0;
    }

    PERCENTAGE difficulty = rpi_ptr->fail;
    if (difficulty == 0) {
        return 100;
    }

    const auto &player_stun = creature.effects()->stun();
    if (player_stun.is_stunned()) {
        difficulty += player_stun.current();
    } else if (creature.level > rpi_ptr->min_level) {
        PERCENTAGE lev_adj = (PERCENTAGE)((creature.level - rpi_ptr->min_level) / 3);
        if (lev_adj > 10) {
            lev_adj = 10;
        }

        difficulty -= lev_adj;
    }

    auto special_easy = CreatureClass(creature).equals(PlayerClassType::IMITATOR);
    special_easy &= creature.is_wielding(FixedArtifactId::GOGO_PENDANT);
    special_easy &= rpi_ptr->racial_name == _("倍返し", "Double Revenge");
    if (special_easy) {
        difficulty -= 12;
    }

    if (difficulty < 5) {
        difficulty = 5;
    }

    difficulty = difficulty / 2;
    const auto stat = creature.stat_cur[rpi_ptr->stat];
    auto sum = 0;
    for (auto i = 1; i <= stat; i++) {
        int val = i - difficulty;
        if (val > 0) {
            sum += (val <= difficulty) ? val : difficulty;
        }
    }

    return ((sum * 100) / difficulty) / stat;
}

static void adjust_racial_power_difficulty(CreatureEntity &creature, rpi_type *rpi_ptr, int *difficulty)
{
    if (*difficulty == 0) {
        return;
    }

    const auto &player_stun = creature.effects()->stun();
    if (player_stun.is_stunned()) {
        *difficulty += player_stun.current();
    } else if (creature.level > rpi_ptr->min_level) {
        int lev_adj = ((creature.level - rpi_ptr->min_level) / 3);
        if (lev_adj > 10) {
            lev_adj = 10;
        }
        *difficulty -= lev_adj;
    }

    if (*difficulty < 5) {
        *difficulty = 5;
    }
}

/*!
 * @brief レイシャル・パワーの発動の判定処理
 * @param creature クリーチャーへの参照
 * @param rpi_ptr 発動したいレイシャル・パワー情報の構造体参照ポインタ
 * @return racial_level_check_result
 */
racial_level_check_result check_racial_level(CreatureEntity &creature, rpi_type *rpi_ptr)
{
    const PLAYER_LEVEL min_level = rpi_ptr->min_level;
    int difficulty = rpi_ptr->fail;
    rpi_ptr->racial_cost = rpi_ptr->cost;
    const int use_hp = creature.csp < rpi_ptr->racial_cost ? rpi_ptr->racial_cost - creature.csp : 0;

    PlayerEnergy energy(creature);
    if (creature.level < min_level) {
        msg_format(_("この能力を使用するにはレベル %d に達していなければなりません。", "You need to attain level %d to use this power."), min_level);
        energy.reset_player_turn();
        return RACIAL_CANCEL;
    }

    if (cmd_limit_confused(creature)) {
        energy.reset_player_turn();
        return RACIAL_CANCEL;
    }

    if (creature.hp < use_hp) {
        if (!input_check(_("本当に今の衰弱した状態でこの能力を使いますか？", "Really use the power in your weakened state? "))) {
            energy.reset_player_turn();
            return RACIAL_CANCEL;
        }
    }

    adjust_racial_power_difficulty(creature, rpi_ptr, &difficulty);
    energy.set_player_turn_energy(100);
    if (randint1(creature.stat_cur[rpi_ptr->stat]) >= ((difficulty / 2) + randint1(difficulty / 2))) {
        return RACIAL_SUCCESS;
    }

    if (flush_failure) {
        flush();
    }

    msg_print(_("充分に集中できなかった。", "You've failed to concentrate hard enough."));
    return RACIAL_FAILURE;
}
