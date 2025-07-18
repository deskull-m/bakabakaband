/*!
 * @brief 落馬処理
 * @date 2020/05/31
 * @author Hourier
 */

#include "pet/pet-fall-off.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "floor/geometry.h"
#include "grid/grid.h"
#include "monster-attack/monster-attack-player.h"
#include "monster/monster-describer.h"
#include "pet/pet-util.h"
#include "player-base/player-class.h"
#include "player/player-damage.h"
#include "player/player-move.h"
#include "player/player-skill.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain/terrain-definition.h"
#include "target/target-checker.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <tl/optional.hpp>

/*!
 * @brief モンスターから直接攻撃を受けた時に落馬するかどうかを判定し、判定アウトならば落馬させる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 */
void check_fall_off_horse(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    if ((player_ptr->riding == 0) || (monap_ptr->damage == 0)) {
        return;
    }

    const auto m_steed_name = monster_desc(player_ptr, player_ptr->current_floor_ptr->m_list[player_ptr->riding], 0);
    if (process_fall_off_horse(player_ptr, (monap_ptr->damage > 200) ? 200 : monap_ptr->damage, false)) {
        msg_format(_("%s^から落ちてしまった！", "You have fallen from %s."), m_steed_name.data());
    }
}

/*!
 * @brief 落馬する可能性を計算する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dam 落馬判定を発した際に受けたダメージ量
 * @param force TRUEならば強制的に落馬する
 * @param 乗馬中のモンスターのレベル
 * @return falseなら落馬しないことで確定、TRUEなら処理続行
 * @details レベルの低い乗馬からは落馬しにくい
 */
static bool calc_fall_off_possibility(PlayerType *player_ptr, const int dam, const bool force, const MonraceDefinition &monrace)
{
    if (force) {
        return true;
    }

    auto cur = player_ptr->skill_exp[PlayerSkillKindType::RIDING];

    int fall_off_level = monrace.level;
    if (player_ptr->riding_ryoute) {
        fall_off_level += 20;
    }

    PlayerSkill(player_ptr).gain_riding_skill_exp_on_fall_off_check(dam);

    if (randint0(dam / 2 + fall_off_level * 2) >= cur / 30 + 10) {
        return true;
    }

    if ((PlayerClass(player_ptr).is_tamer() && !player_ptr->riding_ryoute) || !one_in_(player_ptr->lev * (player_ptr->riding_ryoute ? 2 : 3) + 30)) {
        return false;
    }

    return true;
}

/*!
 * @brief プレイヤーの落馬判定処理
 * @param dam 落馬判定を発した際に受けたダメージ量
 * @param force TRUEならば強制的に落馬する
 * @return 実際に落馬したらTRUEを返す
 */
bool process_fall_off_horse(PlayerType *player_ptr, int dam, bool force)
{
    const auto &monster = player_ptr->current_floor_ptr->m_list[player_ptr->riding];
    const auto &monrace = monster.get_monrace();

    if (!player_ptr->riding || AngbandWorld::get_instance().is_wild_mode()) {
        return false;
    }

    tl::optional<Pos2D> pos_fall_off;
    if (dam >= 0 || force) {
        if (!calc_fall_off_possibility(player_ptr, dam, force, monrace)) {
            return false;
        }

        /* Check around the player */
        auto num_fall_off_grids = 0;
        for (const auto &d : Direction::directions_8()) {
            const auto pos = player_ptr->get_neighbor(d);

            const auto &grid = player_ptr->current_floor_ptr->get_grid(pos);

            if (grid.has_monster()) {
                continue;
            }

            /* Skip non-empty grids */
            if (!grid.has(TerrainCharacteristics::MOVE) && !grid.has(TerrainCharacteristics::CAN_FLY)) {
                if (!can_player_ride_pet(player_ptr, grid, false)) {
                    continue;
                }
            }

            if (grid.has(TerrainCharacteristics::PATTERN)) {
                continue;
            }

            num_fall_off_grids++;

            /* Randomize choice */
            if (randint0(num_fall_off_grids) > 0) {
                continue;
            }

            pos_fall_off = pos;
        }

        if (!pos_fall_off) {
            const auto m_name = monster_desc(player_ptr, monster, 0);
            msg_format(_("%sから振り落とされそうになって、壁にぶつかった。", "You have nearly fallen from %s but bumped into a wall."), m_name.data());
            take_hit(player_ptr, DAMAGE_NOESCAPE, monrace.level + 3, _("壁への衝突", "bumping into a wall"));
            return false;
        }

        lite_spot(player_ptr, player_ptr->get_position());
        lite_spot(player_ptr, *pos_fall_off);
        verify_panel(player_ptr);
    }

    player_ptr->ride_monster(0);
    player_ptr->pet_extra_flags &= ~(PF_TWO_HANDS);
    player_ptr->riding_ryoute = player_ptr->old_riding_ryoute = false;

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::VIEW,
        StatusRecalculatingFlag::LITE,
        StatusRecalculatingFlag::FLOW,
        StatusRecalculatingFlag::MONSTER_LITE,
        StatusRecalculatingFlag::MONSTER_STATUSES,
    };
    rfu.set_flags(flags_srf);
    handle_stuff(player_ptr);
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::OVERHEAD,
        SubWindowRedrawingFlag::DUNGEON,
    };
    rfu.set_flags(flags_swrf);
    static constexpr auto flags_mwrf = {
        MainWindowRedrawingFlag::EXTRA,
        MainWindowRedrawingFlag::UHEALTH,
    };
    rfu.set_flags(flags_mwrf);
    auto fall_dam = false;
    if (player_ptr->levitation && !force) {
        const auto m_name = monster_desc(player_ptr, monster, 0);
        msg_format(_("%sから落ちたが、空中でうまく体勢を立て直して着地した。", "You are thrown from %s but make a good landing."), m_name.data());
    } else {
        take_hit(player_ptr, DAMAGE_NOESCAPE, monrace.level + 3, _("落馬", "Falling from riding"));
        fall_dam = true;
    }

    if (pos_fall_off && !player_ptr->is_dead) {
        (void)move_player_effect(player_ptr, pos_fall_off->y, pos_fall_off->x, MPE_DONT_PICKUP | MPE_DONT_SWAP_MON);
    }

    return fall_dam;
}
