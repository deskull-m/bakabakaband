/*!
 *  @brief プレイヤーの移動処理 / Movement commands
 *  @date 2014/01/02
 *  @author
 *  Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 */

#include "player/player-move.h"
#include "core/disturbance.h"
#include "core/special-internal-keys.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/quest.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/floor-util.h"
#include "floor/geometry.h"
#include "game-option/disturbance-options.h"
#include "grid/grid.h"
#include "grid/trap.h"
#include "inventory/player-inventory.h"
#include "io/input-key-requester.h"
#include "mind/mind-ninja.h"
#include "monster/monster-update.h"
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/player-status-flags.h"
#include "player/player-status.h"
#include "realm/realm-song-numbers.h"
#include "spell-kind/spells-floor.h"
#include "spell-realm/spells-song.h"
#include "status/action-setter.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "target/target-checker.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include "view/display-messages.h"

/*!
 * @brief 地形やその上のアイテムの隠された要素を全て明かす /
 * Search for hidden things
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象となるマスのY座標
 * @param x 対象となるマスのX座標
 */
static void discover_hidden_things(PlayerType *player_ptr, const Pos2D &pos)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const auto &grid = floor.get_grid(pos);
    if (grid.mimic && floor.has_trap_at(pos)) {
        disclose_grid(player_ptr, pos);
        msg_print(_("トラップを発見した。", "You have found a trap."));
        disturb(player_ptr, false, true);
    }

    if (grid.is_hidden_door()) {
        msg_print(_("隠しドアを発見した。", "You have found a secret door."));
        disclose_grid(player_ptr, pos);
        disturb(player_ptr, false, false);
    }

    for (const auto this_o_idx : grid.o_idx_list) {
        auto &item = *floor.o_list[this_o_idx];
        if (item.bi_key.tval() != ItemKindType::CHEST) {
            continue;
        }

        if (item.pval <= 0 || chest_traps[item.pval].none()) {
            continue;
        }

        if (!item.is_known()) {
            msg_print(_("箱に仕掛けられたトラップを発見した！", "You have discovered a trap on the chest!"));
            item.mark_as_known();
            disturb(player_ptr, false, false);
        }
    }
}

/*!
 * @brief プレイヤーの探索処理判定
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void search(PlayerType *player_ptr)
{
    PERCENTAGE chance = player_ptr->skill_srh;
    const auto effects = player_ptr->effects();
    if (effects->blindness().is_blind() || no_lite(player_ptr)) {
        chance = chance / 10;
    }

    if (effects->confusion().is_confused() || effects->hallucination().is_hallucinated()) {
        chance = chance / 10;
    }

    for (const auto &d : Direction::directions()) {
        if (evaluate_percent(chance)) {
            discover_hidden_things(player_ptr, player_ptr->get_neighbor(d));
        }
    }
}

/*!
 * @brief 移動に伴うプレイヤーのステータス変化処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param ny 移動先Y座標
 * @param nx 移動先X座標
 * @param mpe_mode 移動オプションフラグ
 * @return プレイヤーが死亡やフロア離脱を行わず、実際に移動が可能ならばTRUEを返す。
 */
bool move_player_effect(PlayerType *player_ptr, POSITION ny, POSITION nx, BIT_FLAGS mpe_mode)
{
    const Pos2D pos_new(ny, nx);
    const Pos2D pos_old(player_ptr->y, player_ptr->x);
    auto &floor = *player_ptr->current_floor_ptr;
    auto &grid_new = floor.get_grid(pos_new);
    auto &grid_old = floor.get_grid(pos_old);
    const auto &terrain_new = grid_new.get_terrain();
    const auto &terrain_old = grid_old.get_terrain();
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (!(mpe_mode & MPE_STAYING)) {
        const auto om_idx = grid_old.m_idx;
        const auto nm_idx = grid_new.m_idx;
        player_ptr->y = pos_new.y;
        player_ptr->x = pos_new.x;
        if (!(mpe_mode & MPE_DONT_SWAP_MON)) {
            grid_new.m_idx = om_idx;
            grid_old.m_idx = nm_idx;
            if (om_idx > 0) {
                auto &monster = floor.m_list[om_idx];
                monster.set_position(pos_new);
                update_monster(player_ptr, om_idx, true);
            }

            if (nm_idx > 0) {
                auto &monster = floor.m_list[nm_idx];
                monster.set_position(pos_old);
                update_monster(player_ptr, nm_idx, true);
            }
        }

        lite_spot(player_ptr, pos_old);
        lite_spot(player_ptr, pos_new);
        verify_panel(player_ptr);
        if (mpe_mode & MPE_FORGET_FLOW) {
            forget_flow(floor);
            rfu.set_flag(StatusRecalculatingFlag::UN_VIEW);
            rfu.set_flag(MainWindowRedrawingFlag::MAP);
        }

        static constexpr auto flags_srf = {
            StatusRecalculatingFlag::VIEW,
            StatusRecalculatingFlag::LITE,
            StatusRecalculatingFlag::FLOW,
            StatusRecalculatingFlag::MONSTER_LITE,
            StatusRecalculatingFlag::DISTANCE,
        };
        rfu.set_flags(flags_srf);
        static constexpr auto flags_swrf = {
            SubWindowRedrawingFlag::OVERHEAD,
            SubWindowRedrawingFlag::DUNGEON,
        };
        rfu.set_flags(flags_swrf);
        if ((!player_ptr->effects()->blindness().is_blind() && !no_lite(player_ptr)) || !floor.has_trap_at(pos_new)) {
            grid_new.info &= ~(CAVE_UNSAFE);
        }

        if (floor.is_underground() && floor.get_dungeon_definition().flags.has(DungeonFeatureType::FORGET)) {
            wiz_dark(player_ptr);
        }

        if (mpe_mode & MPE_HANDLE_STUFF) {
            handle_stuff(player_ptr);
        }

        if (PlayerClass(player_ptr).equals(PlayerClassType::NINJA)) {
            if (grid_new.info & (CAVE_GLOW)) {
                set_superstealth(player_ptr, false);
            } else if (player_ptr->cur_lite <= 0) {
                set_superstealth(player_ptr, true);
            }
        }

        using Tc = TerrainCharacteristics;
        if ((player_ptr->action == ACTION_HAYAGAKE) && (terrain_new.flags.has_not(Tc::PROJECTION) || (!player_ptr->levitation && terrain_new.flags.has(Tc::DEEP)))) {
            msg_print(_("ここでは素早く動けない。", "You cannot run in here."));
            set_action(player_ptr, ACTION_NONE);
        }

        if (PlayerRace(player_ptr).equals(PlayerRaceType::MERFOLK)) {
            if (terrain_new.flags.has(Tc::WATER) ^ terrain_old.flags.has(Tc::WATER)) {
                rfu.set_flag(StatusRecalculatingFlag::BONUS);
                update_creature(player_ptr);
            }
        }

        if (terrain_new.flags.has(TerrainCharacteristics::SLOW) ^ terrain_old.flags.has(TerrainCharacteristics::SLOW)) {
            rfu.set_flag(StatusRecalculatingFlag::BONUS);
            update_creature(player_ptr);
        }
    }

    const Pos2D pos(ny, nx);
    if (mpe_mode & MPE_ENERGY_USE) {
        if (music_singing(player_ptr, MUSIC_WALL)) {
            (void)project(player_ptr, 0, 0, player_ptr->y, player_ptr->x, (60 + player_ptr->lev), AttributeType::DISINTEGRATE, PROJECT_KILL | PROJECT_ITEM);
            if (!player_ptr->is_located_at(pos) || player_ptr->is_dead || player_ptr->leaving) {
                return false;
            }
        }

        if ((player_ptr->skill_fos >= 50) || (0 == randint0(50 - player_ptr->skill_fos))) {
            search(player_ptr);
        }

        if (player_ptr->action == ACTION_SEARCH) {
            search(player_ptr);
        }
    }

    if (!(mpe_mode & MPE_DONT_PICKUP)) {
        carry(player_ptr, any_bits(mpe_mode, MPE_DO_PICKUP));
    }

    // 自動拾い/自動破壊により床上のアイテムリストが変化した可能性があるので表示を更新
    if (!player_ptr->running) {
        static constexpr auto flags_swrf = {
            SubWindowRedrawingFlag::FLOOR_ITEMS,
            SubWindowRedrawingFlag::FOUND_ITEMS,
        };
        rfu.set_flags(flags_swrf);
        window_stuff(player_ptr);
    }

    PlayerEnergy energy(player_ptr);
    if (terrain_new.flags.has(TerrainCharacteristics::STORE)) {
        disturb(player_ptr, false, true);
        energy.reset_player_turn();
        command_new = SPECIAL_KEY_STORE;
    } else if (terrain_new.flags.has(TerrainCharacteristics::BLDG)) {
        disturb(player_ptr, false, true);
        energy.reset_player_turn();
        command_new = SPECIAL_KEY_BUILDING;
    } else if (terrain_new.flags.has(TerrainCharacteristics::QUEST_ENTER)) {
        disturb(player_ptr, false, true);
        energy.reset_player_turn();
        command_new = SPECIAL_KEY_QUEST;
    } else if (terrain_new.flags.has(TerrainCharacteristics::QUEST_EXIT)) {
        const auto &quests = QuestList::get_instance();
        if (quests.get_quest(floor.quest_number).type == QuestKindType::FIND_EXIT) {
            complete_quest(player_ptr, floor.quest_number);
        }
        leave_quest_check(player_ptr);
        floor.quest_number = i2enum<QuestId>(grid_new.special);
        floor.dun_level = 0;
        if (!floor.is_in_quest()) {
            player_ptr->word_recall = 0;
        }
        player_ptr->oldpx = 0;
        player_ptr->oldpy = 0;
        player_ptr->leaving = true;
    } else if (terrain_new.flags.has(TerrainCharacteristics::HIT_TRAP) && !(mpe_mode & MPE_STAYING)) {
        disturb(player_ptr, false, true);
        if (grid_new.mimic || terrain_new.flags.has(TerrainCharacteristics::SECRET)) {
            msg_print(_("トラップだ！", "You found a trap!"));
            disclose_grid(player_ptr, player_ptr->get_position());
        }

        hit_trap(player_ptr, any_bits(mpe_mode, MPE_BREAK_TRAP));
        if (!player_ptr->is_located_at(pos) || player_ptr->is_dead || player_ptr->leaving) {
            return false;
        }
    }

    if (!(mpe_mode & MPE_STAYING) && (disturb_trap_detect || alert_trap_detect) && player_ptr->dtrap && !(grid_new.info & CAVE_IN_DETECT)) {
        player_ptr->dtrap = false;
        if (!(grid_new.info & CAVE_UNSAFE)) {
            if (alert_trap_detect) {
                msg_print(_("* 注意:この先はトラップの感知範囲外です！ *", "*Leaving trap detect region!*"));
            }

            if (disturb_trap_detect) {
                disturb(player_ptr, false, true);
            }
        }
    }

    return player_ptr->is_located_at(pos) && !player_ptr->is_dead && !player_ptr->leaving;
}

/*!
 * @brief 該当地形のトラップがプレイヤーにとって無効かどうかを判定して返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param feat 地形ID
 * @return トラップが自動的に無効ならばTRUEを返す
 */
bool trap_can_be_ignored(PlayerType *player_ptr, FEAT_IDX feat)
{
    const auto &terrain = TerrainList::get_instance().get_terrain(feat);
    if (terrain.flags.has_not(TerrainCharacteristics::TRAP)) {
        return true;
    }

    switch (i2enum<TrapType>(terrain.subtype)) {
    case TrapType::TRAPDOOR:
    case TrapType::PIT:
    case TrapType::SPIKED_PIT:
    case TrapType::POISON_PIT:
        if (player_ptr->levitation) {
            return true;
        }
        break;
    case TrapType::TELEPORT:
        if (player_ptr->anti_tele) {
            return true;
        }
        break;
    case TrapType::FIRE:
        if (has_immune_fire(player_ptr)) {
            return true;
        }
        break;
    case TrapType::ACID:
        if (has_immune_acid(player_ptr)) {
            return true;
        }
        break;
    case TrapType::BLIND:
        if (has_resist_blind(player_ptr)) {
            return true;
        }
        break;
    case TrapType::CONFUSE:
        if (has_resist_conf(player_ptr)) {
            return true;
        }
        break;
    case TrapType::POISON:
        if (has_resist_pois(player_ptr)) {
            return true;
        }
        break;
    case TrapType::SLEEP:
        if (player_ptr->free_act) {
            return true;
        }
        break;
    default:
        break;
    }

    return false;
}
