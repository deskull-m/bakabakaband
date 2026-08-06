/*!
 * @file monster-drop-generator.cpp
 * @brief モンスター生成時にドロップ品を所持品として生成する処理の実装
 */

#include "monster-floor/monster-drop-generator.h"
#include "floor/floor-object.h"
#include "monster-race/race-drop-flags.h"
#include "monster-race/race-kind-flags.h"
#include "monster-race/race-misc-flags.h"
#include "object-enchant/item-apply-magic.h"
#include "system/angband-system.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/services/baseitem-monrace-service.h"
#include "util/dice.h"
#include "util/enum-converter.h"
#include "util/probability-table.h"
#include <algorithm>
#include <limits>

namespace {
/*!
 * @brief drop_flags からアイテム生成品質モード (AM_GOOD 等) を決める
 */
BIT_FLAGS decide_drop_quality_mode(const MonraceDefinition &monrace)
{
    BIT_FLAGS mode = 0L;
    if (monrace.drop_flags.has(MonsterDropType::DROP_GOOD)) {
        mode |= AM_GOOD;
    }
    if (monrace.drop_flags.has(MonsterDropType::DROP_GREAT)) {
        mode |= (AM_GOOD | AM_GREAT);
    }
    if (monrace.drop_flags.has(MonsterDropType::DROP_NASTY)) {
        mode |= AM_NASTY;
    }
    return mode;
}

/*!
 * @brief drop_flags からドロップ個数を決める
 * @details monster_death() 時の decide_drop_numbers() と同等。生成時点で
 *          確定しているクローン/ペット/アリーナ等の抑制条件も反映する。
 */
int decide_drop_numbers(const CreatureEntity &monster, const MonraceDefinition &monrace, bool inside_arena)
{
    int drop_numbers = 0;
    if (monrace.drop_flags.has(MonsterDropType::DROP_60) && evaluate_percent(60)) {
        drop_numbers++;
    }
    if (monrace.drop_flags.has(MonsterDropType::DROP_90) && evaluate_percent(90)) {
        drop_numbers++;
    }
    if (monrace.drop_flags.has(MonsterDropType::DROP_1D2)) {
        drop_numbers += Dice::roll(1, 2);
    }
    if (monrace.drop_flags.has(MonsterDropType::DROP_2D2)) {
        drop_numbers += Dice::roll(2, 2);
    }
    if (monrace.drop_flags.has(MonsterDropType::DROP_3D2)) {
        drop_numbers += Dice::roll(3, 2);
    }
    if (monrace.drop_flags.has(MonsterDropType::DROP_4D2)) {
        drop_numbers += Dice::roll(4, 2);
    }

    const auto cloned = monster.has_constant_flag(MonsterConstantFlagType::CLONED);
    if (cloned && monrace.kind_flags.has_not(MonsterKindType::UNIQUE)) {
        drop_numbers = 0;
    }
    if (monster.is_pet() || AngbandSystem::get_instance().is_phase_out() || inside_arena) {
        drop_numbers = 0;
    }
    if (monrace.misc_flags.has(MonsterMiscType::MULTIPLY) && (monrace.r_akills > 1024)) {
        drop_numbers = 0;
    }
    return drop_numbers;
}
}

void generate_monster_drop_items(CreatureEntity &player, CreatureEntity &monster)
{
    auto &floor = *player.get_floor();
    const auto &monrace = monster.get_monrace();

    const auto do_gold = monrace.drop_flags.has_none_of({
        MonsterDropType::ONLY_ITEM,
        MonsterDropType::DROP_GOOD,
        MonsterDropType::DROP_GREAT,
    });
    auto do_item = monrace.drop_flags.has_not(MonsterDropType::ONLY_GOLD);
    do_item |= monrace.drop_flags.has_any_of({ MonsterDropType::DROP_GOOD, MonsterDropType::DROP_GREAT });

    auto drop_numbers = decide_drop_numbers(monster, monrace, floor.inside_arena);
    if (!do_item && !monrace.symbol_char_is_any_of("$")) {
        drop_numbers = 0;
    }
    if (drop_numbers <= 0) {
        return;
    }

    const auto mo_mode = decide_drop_quality_mode(monrace);

    // 死亡時 (monster_death) と同様に、生成基準階をモンスター種族レベルで底上げする。
    const auto backup_object_level = floor.object_level;
    floor.object_level = (floor.dun_level + monrace.level) / 2;

    for (auto i = 0; i < drop_numbers; i++) {
        if (do_gold && (!do_item || one_in_(2))) {
            const auto bi_key = BaseitemMonraceService::lookup_fixed_gold_drop(monrace.drop_flags);
            auto item = floor.make_gold(bi_key);
            // 構成材質に応じて金銭額を増減させる (貴金属系は増、紙・糞は減)。
            const auto gold_percent = monster.get_material_gold_drop_percent();
            if (gold_percent != 100) {
                const auto scaled = static_cast<int>(item.pval) * gold_percent / 100;
                item.pval = static_cast<PARAMETER_VALUE>(std::clamp(scaled, 1, static_cast<int>(std::numeric_limits<PARAMETER_VALUE>::max())));
            }
            monster.acquire_item(item);
        } else {
            if (auto item = make_object(player, mo_mode)) {
                monster.acquire_item(*item);
            }
        }
    }

    floor.object_level = backup_object_level;
}
