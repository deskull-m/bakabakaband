/*!
 * @file open-close-execution.cpp
 * @brief 扉や箱を開ける処理
 * @date 2020/07/11
 * @author Hourier
 */

#include "action/open-close-execution.h"
#include "action/movement-execution.h"
#include "combat/attack-power-table.h"
#include "floor/floor-object.h"
#include "game-option/disturbance-options.h"
#include "game-option/input-options.h"
#include "grid/grid.h"
#include "grid/trap.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "player-base/player-class.h"
#include "player-status/player-energy.h"
#include "player/player-status-table.h"
#include "specific-object/chest.h"
#include "status/bad-status-setter.h"
#include "status/experience.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/creature-entity.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "term/screen-processor.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"

/*!
 * @brief 「開ける」動作コマンドのサブルーチン /
 * Perform the basic "open" command on doors
 * @param creature クリーチャーへの参照
 * @param y 対象を行うマスのY座標
 * @param x 対象を行うマスのX座標
 * @return 連続でコマンドを実行する時のみTRUE、1回きりの時はFALSE
 */
bool exe_open(CreatureEntity &creature, POSITION y, POSITION x)
{
    auto &player = static_cast<PlayerType &>(creature);
    const Pos2D pos(y, x);
    const auto &grid = creature.current_floor_ptr->get_grid(pos);
    auto &terrain = grid.get_terrain();
    PlayerEnergy(player).set_player_turn_energy(100);
    if (terrain.flags.has_not(TerrainCharacteristics::OPEN)) {
        constexpr auto fmt = _("%sはがっちりと閉じられているようだ。", "The %s appears to be stuck.");
        msg_format(fmt, grid.get_terrain(TerrainKind::MIMIC).name.data());
        return false;
    }

    if (!terrain.power) {
        cave_alter_feat(creature, y, x, TerrainCharacteristics::OPEN);
        sound(SoundKind::OPENDOOR);
        player.plus_incident_tree("OPEN_DOOR", 1);
        return false;
    }

    int i = player.skill_dis;
    const auto effects = player.effects();
    if (effects->blindness().is_blind() || no_lite(creature)) {
        i = i / 10;
    }

    if (effects->confusion().is_confused() || effects->hallucination().is_hallucinated()) {
        i = i / 10;
    }

    int j = terrain.power;
    j = i - (j * 4);
    if (j < 2) {
        j = 2;
    }

    if (!evaluate_percent(j)) {
        if (flush_failure) {
            flush();
        }

        msg_print(_("鍵をはずせなかった。", "You failed to pick the lock."));
        return true;
    }

    msg_print(_("鍵をはずした。", "You have picked the lock."));
    cave_alter_feat(creature, y, x, TerrainCharacteristics::OPEN);
    sound(SoundKind::OPENDOOR);
    player.plus_incident_tree("OPEN_DOOR", 1);
    gain_exp(creature, 1);
    return false;
}

/*!
 * @brief 「閉じる」動作コマンドのサブルーチン /
 * Perform the basic "close" command
 * @param y 対象を行うマスのY座標
 * @param x 対象を行うマスのX座標
 * @return 実際に処理が行われた場合TRUEを返す。
 * @details
 * Assume destination is an open/broken door
 * Assume there is no monster blocking the destination
 * Returns TRUE if repeated commands may continue
 * @todo 常にFALSEを返している
 */
bool exe_close(CreatureEntity &creature, const Pos2D &pos)
{
    auto &player = static_cast<PlayerType &>(creature);
    const auto &floor = *creature.current_floor_ptr;
    const auto &grid = floor.get_grid(pos);
    const auto terrain_id = grid.feat;
    auto more = false;
    PlayerEnergy(player).set_player_turn_energy(100);
    if (grid.get_terrain().flags.has_not(TerrainCharacteristics::CLOSE)) {
        return more;
    }

    const auto closed_feat = floor.get_dungeon_definition().convert_terrain_id(terrain_id, TerrainCharacteristics::CLOSE);
    auto is_preventing = !grid.o_idx_list.empty() || grid.is_object();
    is_preventing &= closed_feat != terrain_id;
    is_preventing &= TerrainList::get_instance().get_terrain(closed_feat).flags.has_not(TerrainCharacteristics::DROP);
    if (is_preventing) {
        msg_print(_("何かがつっかえて閉まらない。", "Something prevents it from closing."));
        return more;
    }

    cave_alter_feat(creature, pos.y, pos.x, TerrainCharacteristics::CLOSE);
    if (terrain_id == grid.feat) {
        msg_print(_("ドアは壊れてしまっている。", "The door appears to be broken."));
    } else {
        sound(SoundKind::SHUTDOOR);
        player.plus_incident_tree("CLOSE_DOOR", 1);
    }

    return more;
}

/*!
 * @brief 移動処理による簡易な「開く」処理 /
 * easy_open_door --
 * @return 開く処理が実際に試みられた場合TRUEを返す
 * @details
 * <pre>
 *	If there is a jammed/closed/locked door at the given location,
 *	then attempt to unlock/open it. Return TRUE if an attempt was
 *	made (successful or not), otherwise return false.
 *
 *	The code here should be nearly identical to that in
 *	do_cmd_open_test() and exe_open().
 * </pre>
 */
bool easy_open_door(CreatureEntity &creature, const Pos2D &pos)
{
    auto &player = static_cast<PlayerType &>(creature);
    const auto &floor = *creature.current_floor_ptr;
    if (!floor.has_closed_door_at(pos)) {
        return false;
    }

    const auto &grid = floor.get_grid(pos);
    const auto &terrain = grid.get_terrain();
    if (terrain.flags.has_not(TerrainCharacteristics::OPEN)) {
        constexpr auto fmt = _("%sはがっちりと閉じられているようだ。", "The %s appears to be stuck.");
        msg_format(fmt, grid.get_terrain(TerrainKind::MIMIC).name.data());
    } else if (terrain.power) {
        auto power_disarm = player.skill_dis;
        const auto effects = player.effects();
        if (effects->blindness().is_blind() || no_lite(creature)) {
            power_disarm = power_disarm / 10;
        }

        if (effects->confusion().is_confused() || effects->hallucination().is_hallucinated()) {
            power_disarm = power_disarm / 10;
        }

        auto power_terrain = terrain.power;
        power_terrain = power_disarm - (power_terrain * 4);
        if (power_terrain < 2) {
            power_terrain = 2;
        }

        if (evaluate_percent(power_terrain)) {
            msg_print(_("鍵をはずした。", "You have picked the lock."));
            cave_alter_feat(creature, pos.y, pos.x, TerrainCharacteristics::OPEN);
            sound(SoundKind::OPENDOOR);
            gain_exp(creature, 1);
        } else {
            if (flush_failure) {
                flush();
            }

            msg_print(_("鍵をはずせなかった。", "You failed to pick the lock."));
        }
    } else {
        cave_alter_feat(creature, pos.y, pos.x, TerrainCharacteristics::OPEN);
        sound(SoundKind::OPENDOOR);
    }

    return true;
}

/*!
 * @brief 箱のトラップを解除する実行処理 /
 * Perform the basic "disarm" command
 * @param y 解除を行うマスのY座標
 * @param x 解除を行うマスのX座標
 * @param o_idx 箱のオブジェクトID
 * @return ターンを消費する処理が行われた場合TRUEを返す
 * @details
 * <pre>
 * Assume destination is a visible trap
 * Assume there is no monster blocking the destination
 * Returns TRUE if repeated commands may continue
 * </pre>
 */
bool exe_disarm_chest(CreatureEntity &creature, POSITION y, POSITION x, OBJECT_IDX o_idx)
{
    auto &player = static_cast<PlayerType &>(creature);
    const Pos2D pos(y, x);
    auto *o_ptr = creature.current_floor_ptr->o_list[o_idx].get();
    PlayerEnergy(player).set_player_turn_energy(100);
    int i = player.skill_dis;
    const auto effects = player.effects();
    if (effects->blindness().is_blind() || no_lite(creature)) {
        i = i / 10;
    }

    if (effects->confusion().is_confused() || effects->hallucination().is_hallucinated()) {
        i = i / 10;
    }

    auto j = i - o_ptr->pval;
    if (j < 2) {
        j = 2;
    }

    auto more = false;
    if (!o_ptr->is_known()) {
        msg_print(_("トラップが見あたらない。", "I don't see any traps."));
    } else if (o_ptr->pval <= 0) {
        msg_print(_("箱にはトラップが仕掛けられていない。", "The chest is not trapped."));
    } else if (chest_traps[o_ptr->pval].none()) {
        msg_print(_("箱にはトラップが仕掛けられていない。", "The chest is not trapped."));
    } else if (evaluate_percent(j)) {
        msg_print(_("箱に仕掛けられていたトラップを解除した。", "You have disarmed the chest."));
        gain_exp(creature, o_ptr->pval);
        o_ptr->pval = (0 - o_ptr->pval);
    } else if ((i > 5) && (randint1(i) > 5)) {
        more = true;
        if (flush_failure) {
            flush();
        }

        msg_print(_("箱のトラップ解除に失敗した。", "You failed to disarm the chest."));
    } else {
        msg_print(_("トラップを作動させてしまった！", "You set off a trap!"));
        sound(SoundKind::FAIL);
        Chest(creature).fire_trap(pos, o_idx);
    }

    return more;
}

/*!
 * @brief 箱のトラップを解除するコマンドのサブルーチン /
 * Perform the basic "disarm" command
 * @param y 解除を行うマスのY座標
 * @param x 解除を行うマスのX座標
 * @param dir プレイヤーからみた方向
 * @return ターンを消費する処理が行われた場合TRUEを返す
 * @details
 * <pre>
 * Assume destination is a visible trap
 * Assume there is no monster blocking the destination
 * Returns TRUE if repeated commands may continue
 * </pre>
 */

bool exe_disarm(CreatureEntity &creature, POSITION y, POSITION x, const Direction &dir)
{
    auto &player = static_cast<PlayerType &>(creature);
    const auto &baseitems = BaseitemList::get_instance();
    const Pos2D pos(y, x);
    const auto &grid = creature.current_floor_ptr->get_grid(pos);
    const auto &terrain = grid.get_terrain();
    const auto &name = terrain.name;
    int power = terrain.power;
    int i = player.skill_dis;
    PlayerEnergy(player).set_player_turn_energy(100);
    auto effects = player.effects();
    if (effects->blindness().is_blind() || no_lite(creature)) {
        i = i / 10;
    }

    if (effects->confusion().is_confused() || effects->hallucination().is_hallucinated()) {
        i = i / 10;
    }

    int j = i - power;
    if (j < 2) {
        j = 2;
    }

    auto more = false;
    if (evaluate_percent(j)) {
        ItemEntity item;
        item.generate(baseitems.lookup_baseitem_id({ ItemKindType::TRAP, 0 }));
        item.pval = grid.feat;
        msg_format(_("%sを解除した。", "You have disarmed the %s."), name.data());
        gain_exp(creature, power);
        cave_alter_feat(creature, y, x, TerrainCharacteristics::DISARM);
        exe_movement(player, dir, easy_disarm, false);

        (void)drop_near(player, item, pos);

    } else if ((i > 5) && (randint1(i) > 5)) {
        if (flush_failure) {
            flush();
        }

        msg_format(_("%sの解除に失敗した。", "You failed to disarm the %s."), name.data());
        more = true;
    } else {
        msg_format(_("%sを作動させてしまった！", "You set off the %s!"), name.data());
        exe_movement(player, dir, easy_disarm, false);
    }

    return more;
}

/*!
 * @brief 「打ち破る」動作コマンドのサブルーチン /
 * Perform the basic "bash" command
 * @param y 対象を行うマスのY座標
 * @param x 対象を行うマスのX座標
 * @param dir プレイヤーから見たターゲットの方向
 * @return 実際に処理が行われた場合TRUEを返す。
 * @details
 * <pre>
 * Assume destination is a closed/locked/jammed door
 * Assume there is no monster blocking the destination
 * Returns TRUE if repeated commands may continue
 * </pre>
 */
bool exe_bash(CreatureEntity &creature, POSITION y, POSITION x, const Direction &dir)
{
    auto &player = static_cast<PlayerType &>(creature);
    const auto &floor = *creature.current_floor_ptr;
    const Pos2D pos(y, x);
    const auto &grid = floor.get_grid(pos);
    const auto &terrain = grid.get_terrain();
    int bash = adj_str_blow[player.stat_index[A_STR]];
    int power = terrain.power;
    const auto &name = grid.get_terrain(TerrainKind::MIMIC).name;
    PlayerEnergy(player).set_player_turn_energy(100);
    msg_format(_("%sに体当たりをした！", "You smash into the %s!"), name.data());
    power = (bash - (power * 10));
    if (CreatureClass(player).equals(PlayerClassType::BERSERKER)) {
        power *= 2;
    }

    if (power < 1) {
        power = 1;
    }

    auto more = false;
    if (evaluate_percent(power)) {
        msg_format(_("%sを壊した！", "The %s crashes open!"), name.data());
        sound(terrain.flags.has(TerrainCharacteristics::GLASS) ? SoundKind::GLASS : SoundKind::OPENDOOR);
        const auto &dungeon = floor.get_dungeon_definition();
        if (one_in_(2) || (dungeon.convert_terrain_id(grid.feat, TerrainCharacteristics::OPEN) == grid.feat) || terrain.flags.has(TerrainCharacteristics::GLASS)) {
            cave_alter_feat(creature, y, x, TerrainCharacteristics::BASH);
        } else {
            cave_alter_feat(creature, y, x, TerrainCharacteristics::OPEN);
        }

        exe_movement(player, dir, false, false);
    } else if (evaluate_percent(adj_dex_safe[player.stat_index[A_DEX]] + player.level)) {
        msg_format(_("この%sは頑丈だ。", "The %s holds firm."), name.data());
        more = true;
    } else {
        msg_print(_("体のバランスをくずしてしまった。", "You are off-balance."));
        (void)BadStatusSetter(player).mod_paralysis(2 + randint0(2));
    }

    return more;
}
