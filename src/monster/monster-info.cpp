/*!
 * @brief モンスター情報の記述 / describe monsters (using monster memory)
 * @date 2013/12/11
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * 2014 Deskull rearranged comment for Doxygen.
 */

#include "monster/monster-info.h"
#include "alliance/alliance.h"
#include "floor/wild.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-resistance-mask.h"
#include "monster/monster-describer.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-status.h"
#include "monster/smart-learn-types.h"
#include "player/player-status-flags.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"

/*!
 * @brief モンスターが地形を踏破できるかどうかを返す
 * Check if monster can cross terrain
 * @param creature_ptr クリーチャーへの参照ポインタ
 * @param feat 地形ID
 * @param r_ptr モンスター種族構造体の参照ポインタ
 * @param mode オプション
 * @return 踏破可能ならばTRUEを返す
 */
bool monster_can_cross_terrain(CreatureEntity &creature, FEAT_IDX feat, const MonraceDefinition &monrace, BIT_FLAGS16 mode)
{
    const auto &terrain = TerrainList::get_instance().get_terrain(feat);
    if (terrain.flags.has(TerrainCharacteristics::PATTERN)) {
        if (!(mode & CEM_RIDING)) {
            if (monrace.feature_flags.has_not(MonsterFeatureType::CAN_FLY)) {
                return false;
            }
        } else {
            if (!(mode & CEM_P_CAN_ENTER_PATTERN)) {
                return false;
            }
        }
    }

    if (terrain.flags.has(TerrainCharacteristics::CAN_FLY) && monrace.feature_flags.has(MonsterFeatureType::CAN_FLY)) {
        return true;
    }
    if (terrain.flags.has(TerrainCharacteristics::CAN_SWIM) && monrace.feature_flags.has(MonsterFeatureType::CAN_SWIM)) {
        return true;
    }
    if (terrain.flags.has(TerrainCharacteristics::CAN_PASS)) {
        if (monrace.feature_flags.has(MonsterFeatureType::PASS_WALL) && (!(mode & CEM_RIDING) || has_pass_wall(creature))) {
            return true;
        }
    }

    if (terrain.flags.has_not(TerrainCharacteristics::MOVE)) {
        return false;
    }

    if (terrain.flags.has(TerrainCharacteristics::MOUNTAIN) && (monrace.wilderness_flags.has(MonsterWildernessType::WILD_MOUNTAIN))) {
        return true;
    }

    if (terrain.flags.has(TerrainCharacteristics::WATER)) {
        if (monrace.feature_flags.has_not(MonsterFeatureType::AQUATIC)) {
            if (terrain.flags.has(TerrainCharacteristics::DEEP)) {
                return false;
            } else if (monrace.aura_flags.has(MonsterAuraType::FIRE)) {
                return false;
            }
        }
    } else if (monrace.feature_flags.has(MonsterFeatureType::AQUATIC)) {
        return false;
    }

    /* Railway only monsters can only move on railways */
    if (monrace.feature_flags.has(MonsterFeatureType::RAILWAY_ONLY)) {
        if (terrain.flags.has_not(TerrainCharacteristics::RAILWAY)) {
            return false;
        }
    }

    if (terrain.flags.has(TerrainCharacteristics::LAVA)) {
        if (monrace.resistance_flags.has_none_of(RFR_EFF_IM_FIRE_MASK)) {
            return false;
        }
    }

    if (terrain.flags.has(TerrainCharacteristics::COLD_PUDDLE)) {
        if (monrace.resistance_flags.has_none_of(RFR_EFF_IM_COLD_MASK)) {
            return false;
        }
    }

    if (terrain.flags.has(TerrainCharacteristics::ELEC_PUDDLE)) {
        if (monrace.resistance_flags.has_none_of(RFR_EFF_IM_ELEC_MASK)) {
            return false;
        }
    }

    if (terrain.flags.has(TerrainCharacteristics::ACID_PUDDLE)) {
        if (monrace.resistance_flags.has_none_of(RFR_EFF_IM_ACID_MASK)) {
            return false;
        }
    }

    if (terrain.flags.has(TerrainCharacteristics::POISON_PUDDLE)) {
        if (monrace.resistance_flags.has_none_of(RFR_EFF_IM_POISON_MASK)) {
            return false;
        }
    }

    if (terrain.flags.has(TerrainCharacteristics::DUNG_POOL)) {
        if (monrace.resistance_flags.has_none_of(RFR_EFF_IM_POISON_MASK)) {
            return false;
        }
    }

    if (terrain.flags.has(TerrainCharacteristics::CHAOS_TAINTED)) {
        if (monrace.resistance_flags.has_none_of(RFR_EFF_RESIST_CHAOS_MASK)) {
            return false;
        }
    }

    if (terrain.flags.has(TerrainCharacteristics::VOID)) {
        if (monrace.resistance_flags.has_none_of(RFR_EFF_RESIST_VOID_MASK)) {
            return false;
        }
    }

    return true;
}

/*!
 * @brief 指定された座標の地形をモンスターが踏破できるかどうかを返す
 * Strictly check if monster can enter the grid
 * @param creature_ptr クリーチャーへの参照ポインタ
 * @param y 地形のY座標
 * @param x 地形のX座標
 * @param r_ptr モンスター種族構造体の参照ポインタ
 * @param mode オプション
 * @return 踏破可能ならばTRUEを返す
 */
bool monster_can_enter(CreatureEntity &creature, POSITION y, POSITION x, const MonraceDefinition &monrace, BIT_FLAGS16 mode)
{
    const Pos2D pos(y, x);
    auto &grid = creature.get_floor()->get_grid(pos);
    if (creature.is_located_at(pos)) {
        return false;
    }
    if (grid.has_monster()) {
        return false;
    }

    return monster_can_cross_terrain(creature, grid.feat, monrace, mode);
}

static uint8_t get_recial_sub_align(const MonraceDefinition &monrace)
{
    uint8_t sub_align = SUB_ALIGN_NEUTRAL;
    if (monrace.kind_flags.has(MonsterKindType::EVIL)) {
        sub_align |= SUB_ALIGN_EVIL;
    }
    if (monrace.kind_flags.has(MonsterKindType::GOOD)) {
        sub_align |= SUB_ALIGN_GOOD;
    }
    return sub_align;
}

/*!
 * @brief モンスターがプレイヤーに対して敵意を抱くかどうかを返す
 * @param creature クリーチャーへの参照
 * @param pa_good プレイヤーの善傾向値
 * @param pa_evil プレイヤーの悪傾向値
 * @param monrace モンスター種族情報の参照
 * @return プレイヤーに敵意を持つならばtrueを返す
 */
bool monster_has_hostile_to_player(CreatureEntity &creature, int pa_good, int pa_evil, const MonraceDefinition &monrace)
{
    byte sub_align1 = SUB_ALIGN_NEUTRAL;
    if (creature.alignment >= pa_good) {
        sub_align1 |= SUB_ALIGN_GOOD;
    }
    if (creature.alignment <= pa_evil) {
        sub_align1 |= SUB_ALIGN_EVIL;
    }

    const auto sub_align2 = get_recial_sub_align(monrace);
    return CreatureEntity::check_sub_alignments(sub_align1, sub_align2);
}

/*!
 * @brief モンスターが他のモンスターに対して敵意を抱くかどうかを返す
 * @param monster_other 敵意を抱くか調べる他のモンスターの参照
 * @param monrace モンスター種族情報の参照
 * @return monster_other で指定したモンスターに敵意を持つならばtrueを返す
 * @details アライアンス未所属（NONE）として判定する
 */
bool monster_has_hostile_to_other_monster(const CreatureEntity &creature_other, const MonraceDefinition &monrace)
{
    return monster_has_hostile_to_other_monster(creature_other, monrace, AllianceType::NONE);
}

/*!
 * @brief モンスターが他のモンスターに対して敵意を抱くかどうかを返す（アライアンス指定版）
 * @param creature_other 敵意を抱くか調べる他のクリーチャーの参照
 * @param monrace モンスター種族情報の参照
 * @param alliance_id アライアンスID
 * @return creature_other で指定したクリーチャーに敵意を持つならばtrueを返す
 */
bool monster_has_hostile_to_other_monster(const CreatureEntity &creature_other, const MonraceDefinition &monrace, AllianceType alliance_id)
{
    const auto &alliance = alliance_list.at(alliance_id);
    return alliance->is_hostile_to(creature_other, monrace);
}

/*!
 * @brief サブアライメント値と種族定義に基づく敵意判定
 * @param sub_align 確認対象のサブアライメント値
 * @param monrace モンスター種族情報の参照
 * @return sub_align のクリーチャーが monrace に対して敵意を持つならtrue
 */
bool monster_has_hostile_sub_align(uint8_t sub_align, const MonraceDefinition &monrace)
{
    uint8_t sub_align2 = SUB_ALIGN_NEUTRAL;
    if (monrace.kind_flags.has(MonsterKindType::EVIL)) {
        sub_align2 |= SUB_ALIGN_EVIL;
    }
    if (monrace.kind_flags.has(MonsterKindType::GOOD)) {
        sub_align2 |= SUB_ALIGN_GOOD;
    }
    return CreatureEntity::check_sub_alignments(sub_align, sub_align2);
}

bool is_original_ap_and_seen(CreatureEntity &subject, const CreatureEntity &creature)
{
    return creature.has_monster_profile() && creature.get_monster_profile().ml && !subject.is_hallucinated() && creature.is_original_ap();
}

/*!
 * @brief モンスターIDを取り、モンスター名をm_nameに代入する /
 * @param creature クリーチャーへの参照
 * @param m_idx モンスターID
 * @return std::string モンスター名
 */
std::string monster_name(CreatureEntity &creature, MONSTER_IDX m_idx)
{
    const auto &monster = creature.get_floor()->get_monster(m_idx);
    return monster_desc(creature, monster, 0x00);
}
