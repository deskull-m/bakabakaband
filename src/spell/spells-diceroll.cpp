#include "spell/spells-diceroll.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/race-flags-resistance.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player/player-status-table.h"
#include "room/rooms-builder.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"

/*!
 * @brief モンスター魅了/服従用セービングスロー共通実装 (提案D5)
 * @param pow 魅了・服従パワー
 * @param target 対象クリーチャー
 * @param check_no_conf NO_CONF (混乱耐性) で無条件抵抗させるか (魅了=true / 服従=false)
 * @return 抵抗したらTRUE
 * @details charm/control で NO_CONF チェックの有無以外は完全に同一だったため統合。
 */
static bool common_saving_throw_impl(CreatureEntity &creature, int pow, const CreatureEntity &target, bool check_no_conf)
{
    auto &monrace = target.get_monrace();

    if (creature.get_floor()->inside_arena) {
        return true;
    }

    /* Memorize a flag */
    if (monrace.resistance_flags.has(MonsterResistanceType::RESIST_ALL)) {
        if (is_original_ap_and_seen(creature, target)) {
            monrace.r_resistance_flags.set(MonsterResistanceType::RESIST_ALL);
        }
        return true;
    }

    if (check_no_conf && monrace.resistance_flags.has(MonsterResistanceType::NO_CONF)) {
        if (is_original_ap_and_seen(creature, target)) {
            monrace.resistance_flags.set(MonsterResistanceType::NO_CONF);
        }
        return true;
    }

    if (monrace.misc_flags.has(MonsterMiscType::QUESTOR) || target.is_nopet()) {
        return true;
    }

    pow += (adj_chr_chm[creature.get_stat_index(A_CHR)] - 1);
    if (monrace.kind_flags.has(MonsterKindType::UNIQUE) || (monrace.population_flags.has(MonsterPopulationType::NAZGUL))) {
        pow = pow * 2 / 3;
    }
    // [提案C2第3弾・セーヴ] applies_stat_combat_bonus の対象は WIS セーヴ補正を実効レベルへ加算 (opt-in・既定OFF)
    return ((monrace.level + target.get_save_stat_bonus()) > randint1((pow - 10) < 1 ? 1 : (pow - 10)) + 5);
}

/*!
 * @brief モンスター魅了用セービングスロー共通部(汎用系)
 * @param pow 魅了パワー
 * @param target 対象モンスター
 * @return 魅了に抵抗したらTRUE
 */
bool common_saving_throw_charm(CreatureEntity &creature, int pow, const CreatureEntity &target)
{
    return common_saving_throw_impl(creature, pow, target, true);
}

/*!
 * @brief モンスター服従用セービングスロー共通部(部族依存系)
 * @param pow 服従パワー
 * @param target 対象モンスター
 * @return 服従に抵抗したらTRUE
 */
bool common_saving_throw_control(CreatureEntity &creature, int pow, const CreatureEntity &target)
{
    return common_saving_throw_impl(creature, pow, target, false);
}

/*!
 * @brief 一部ボルト魔法のビーム化確率を算出する / Prepare standard probability to become beam for fire_bolt_or_beam()
 * @return ビーム化確率(%)
 * @details
 * ハードコーティングによる実装が行われている。
 * メイジは(レベル)%、ハイメイジ、スペルマスターは(レベル)%、それ以外の職業は(レベル/2)%
 */
PERCENTAGE beam_chance(CreatureEntity &creature)
{
    CreatureClass pc(creature);
    if (pc.equals(PlayerClassType::MAGE)) {
        return (PERCENTAGE)(creature.get_level());
    }
    if (pc.equals(PlayerClassType::HIGH_MAGE) || pc.equals(PlayerClassType::SORCERER)) {
        return (PERCENTAGE)(creature.get_level() + 10);
    }

    return (PERCENTAGE)(creature.get_level() / 2);
}
