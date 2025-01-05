/*!
 * @brief 馬鹿馬鹿蛮怒 v1.5.0以前の旧いセーブデータを読み込む処理
 * @date 2020/07/04
 * @author Hourier
 * @details 互換性を最大限に確保するため、基本的に関数分割は行わないものとする.
 */

#include "load/old/load-v1-5-0.h"
#include "artifact/fixed-art-types.h"
#include "floor/floor-object.h"
#include "game-option/birth-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "grid/trap.h"
#include "load/item/item-loader-factory.h"
#include "load/item/item-loader-version-types.h"
#include "load/load-util.h"
#include "load/monster/monster-loader-factory.h"
#include "load/old-feature-types.h"
#include "load/old/item-loader-savefile50.h"
#include "load/old/monster-loader-savefile50.h"
#include "mind/mind-weaponsmith.h"
#include "monster-floor/monster-move.h"
#include "monster-race/race-flags-resistance.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/old-ego-extra-values.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object-enchant/trg-types.h"
#include "sv-definition/sv-armor-types.h"
#include "sv-definition/sv-lite-types.h"
#include "system/angband-exceptions.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem-info.h"
#include "system/dungeon-info.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include "world/world-object.h"
#include "world/world.h"

/* Old hidden trap flag */
static const BIT_FLAGS CAVE_TRAP = 0x8000;

const int OLD_QUEST_WATER_CAVE = 18; // 湖の洞窟.
const int QUEST_OLD_CASTLE = 27; // 古い城.
const int QUEST_ROYAL_CRYPT = 28; // 王家の墓.

static void move_RF3_to_RFR(MonraceDefinition *r_ptr, BIT_FLAGS f3, const BIT_FLAGS rf3, const MonsterResistanceType rfr)
{
    if (f3 & rf3) {
        r_ptr->resistance_flags.set(rfr);
    }
}

static void move_RF4_BR_to_RFR(MonraceDefinition *r_ptr, BIT_FLAGS f4, const BIT_FLAGS rf4_br, const MonsterResistanceType rfr)
{
    if (f4 & rf4_br) {
        r_ptr->resistance_flags.set(rfr);
    }
}

/*!
 * @brief モンスターの思い出を読み込む
 * @param r_ptr モンスター種族情報への参照ポインタ
 * @param r_idx モンスター種族ID
 * @details 本来はr_idxからr_ptrを決定可能だが、互換性を優先するため元コードのままとする
 */
void set_old_lore(MonraceDefinition *r_ptr, BIT_FLAGS f3, BIT_FLAGS f4, const MonraceId r_idx)
{
    r_ptr->r_resistance_flags.clear();
    move_RF3_to_RFR(r_ptr, f3, RF3_IM_ACID, MonsterResistanceType::IMMUNE_ACID);
    move_RF3_to_RFR(r_ptr, f3, RF3_IM_ELEC, MonsterResistanceType::IMMUNE_ELEC);
    move_RF3_to_RFR(r_ptr, f3, RF3_IM_FIRE, MonsterResistanceType::IMMUNE_FIRE);
    move_RF3_to_RFR(r_ptr, f3, RF3_IM_COLD, MonsterResistanceType::IMMUNE_COLD);
    move_RF3_to_RFR(r_ptr, f3, RF3_IM_POIS, MonsterResistanceType::IMMUNE_POISON);
    move_RF3_to_RFR(r_ptr, f3, RF3_RES_TELE, MonsterResistanceType::RESIST_TELEPORT);
    move_RF3_to_RFR(r_ptr, f3, RF3_RES_NETH, MonsterResistanceType::RESIST_NETHER);
    move_RF3_to_RFR(r_ptr, f3, RF3_RES_WATE, MonsterResistanceType::RESIST_WATER);
    move_RF3_to_RFR(r_ptr, f3, RF3_RES_PLAS, MonsterResistanceType::RESIST_PLASMA);
    move_RF3_to_RFR(r_ptr, f3, RF3_RES_NEXU, MonsterResistanceType::RESIST_NEXUS);
    move_RF3_to_RFR(r_ptr, f3, RF3_RES_DISE, MonsterResistanceType::RESIST_DISENCHANT);
    move_RF3_to_RFR(r_ptr, f3, RF3_RES_ALL, MonsterResistanceType::RESIST_ALL);

    move_RF4_BR_to_RFR(r_ptr, f4, RF4_BR_LITE, MonsterResistanceType::RESIST_LITE);
    move_RF4_BR_to_RFR(r_ptr, f4, RF4_BR_DARK, MonsterResistanceType::RESIST_DARK);
    move_RF4_BR_to_RFR(r_ptr, f4, RF4_BR_SOUN, MonsterResistanceType::RESIST_SOUND);
    move_RF4_BR_to_RFR(r_ptr, f4, RF4_BR_CHAO, MonsterResistanceType::RESIST_CHAOS);
    move_RF4_BR_to_RFR(r_ptr, f4, RF4_BR_TIME, MonsterResistanceType::RESIST_TIME);
    move_RF4_BR_to_RFR(r_ptr, f4, RF4_BR_INER, MonsterResistanceType::RESIST_INERTIA);
    move_RF4_BR_to_RFR(r_ptr, f4, RF4_BR_GRAV, MonsterResistanceType::RESIST_GRAVITY);
    move_RF4_BR_to_RFR(r_ptr, f4, RF4_BR_SHAR, MonsterResistanceType::RESIST_SHARDS);
    move_RF4_BR_to_RFR(r_ptr, f4, RF4_BR_WALL, MonsterResistanceType::RESIST_FORCE);

    if (f4 & RF4_BR_CONF) {
        r_ptr->r_resistance_flags.set(MonsterResistanceType::NO_CONF);
    }

    if (r_idx == MonraceId::STORMBRINGER) {
        r_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_CHAOS);
    }

    if (r_ptr->r_kind_flags.has(MonsterKindType::ORC)) {
        r_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_DARK);
    }
}
