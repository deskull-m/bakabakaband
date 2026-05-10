#include "spell-kind/spells-polymorph.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "floor/floor-object.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-processor.h"
#include "market/building-util.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/monster-remover.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "player/player-sex.h"
#include "system/angband-system.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/redrawing-flags-updater.h"
#include "target/target-checker.h"
#include "term/screen-processor.h"
#include "tracking/health-bar-tracker.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"

/*!
 * @brief 変身処理向けにモンスターの近隣レベル帯モンスターを返す
 * @param creature クリーチャーへの参照
 * @param monrace_id 基準となるモンスター種族ID
 * @return 変更先のモンスター種族ID
 */
static MonraceId select_polymorph_monrace_id(CreatureEntity &creature, MonraceId monrace_id)
{
    const auto &monraces = MonraceList::get_instance();
    const auto &monrace = monraces.get_monrace(monrace_id);
    if (monrace.kind_flags.has(MonsterKindType::UNIQUE) || monrace.misc_flags.has(MonsterMiscType::QUESTOR)) {
        return monrace_id;
    }

    const auto lev1 = monrace.level - ((randint1(20) / randint1(9)) + 1);
    const auto lev2 = monrace.level + ((randint1(20) / randint1(9)) + 1);
    for (auto i = 0; i < 1000; i++) {
        const auto new_monrace_id = get_mon_num(creature, 0, (creature.get_floor()->dun_level + monrace.level) / 2 + 5, PM_NONE);
        if (!MonraceList::is_valid(new_monrace_id)) {
            break;
        }

        const auto &new_monrace = monraces.get_monrace(new_monrace_id);
        if (new_monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
            continue;
        }

        if ((new_monrace.level < lev1) || (new_monrace.level > lev2)) {
            continue;
        }

        return new_monrace_id;
    }

    return monrace_id;
}

/*!
 * @brief 指定座標にいるモンスターを変身させる /
 * Helper function -- return a "nearby" race for polymorphing
 * @param creature クリーチャーへの参照
 * @param y 指定のY座標
 * @param x 指定のX座標
 * @return 実際に変身したらTRUEを返す
 */
bool polymorph_monster(CreatureEntity &creature, POSITION y, POSITION x)
{
    auto &floor = *creature.get_floor();
    const auto &grid = floor.grid_array[y][x];
    auto &monster = floor.get_monster(grid.m_idx);
    MonraceId new_r_idx;
    MonraceId old_r_idx = monster.r_idx;
    const auto target_m_idx = Target::get_last_target().get_m_idx();
    const auto targeted = target_m_idx == grid.m_idx;
    auto health_tracked = HealthBarTracker::get_instance().is_tracking(grid.m_idx);

    if (floor.inside_arena || AngbandSystem::get_instance().is_phase_out()) {
        return false;
    }
    if (monster.is_riding() || monster.is_kage()) {
        return false;
    }

    auto back_m = floor.m_list[grid.m_idx];
    new_r_idx = select_polymorph_monrace_id(creature, old_r_idx);
    if (new_r_idx == old_r_idx) {
        return false;
    }

    // [フェーズ A-4] 所持アイテムは inventory[] (CreatureEntity 共通) で判定する
    bool preserve_hold_objects = false;
    for (const auto &item : back_m.inventory) {
        if (item->is_valid()) {
            preserve_hold_objects = true;
            break;
        }
    }

    BIT_FLAGS mode = 0L;
    if (monster.is_friendly()) {
        mode |= PM_FORCE_FRIENDLY;
    }
    if (monster.is_pet()) {
        mode |= PM_FORCE_PET;
    }
    if (monster.is_nopet()) {
        mode |= PM_NO_PET;
    }

    delete_monster_idx(creature, grid.m_idx);
    bool polymorphed = false;
    auto m_idx = place_specific_monster(creature, y, x, new_r_idx, mode);
    if (m_idx) {
        auto &monster_polymorphed = floor.get_monster(*m_idx);
        monster_polymorphed.name = back_m.name;
        monster_polymorphed.set_parent_m_idx(back_m.get_parent_m_idx());
        // 所持アイテムは inventory[] を引き継ぐ
        if (preserve_hold_objects) {
            for (size_t i = 0; i < back_m.inventory.size(); i++) {
                if (back_m.inventory[i]->is_valid()) {
                    *monster_polymorphed.inventory[i] = back_m.inventory[i]->clone();
                }
            }
            monster_polymorphed.inven_cnt = back_m.inven_cnt;
            monster_polymorphed.equip_cnt = back_m.equip_cnt;
        }
        polymorphed = true;
    } else {
        m_idx = place_specific_monster(creature, y, x, old_r_idx, (mode | PM_NO_KAGE | PM_IGNORE_TERRAIN));
        if (m_idx) {
            floor.m_list[*m_idx] = back_m; // inventory[] を含めて丸ごと復元
            floor.reset_mproc();
        }
    }

    if (targeted) {
        if (m_idx) {
            Target::set_last_target(Target::create_monster_target(creature, *m_idx));
        } else {
            Target::clear_last_target();
        }
    }
    if (health_tracked) {
        health_track(creature, m_idx.value_or(0));
    }
    return polymorphed;
}

/*!
 * @brief 性転換処理
 * @param creature クリーチャーへの参照
 * @return テレポート処理を決定したか否か
 */
bool trans_sex(CreatureEntity &creature)
{
    screen_save();
    clear_bldg(4, 10);

    int i;
    for (i = 0; i < MAX_SEXES; i++) {
        char buf[80];

        if (i == creature.psex) {
            continue;
        }

        sprintf(buf, "%c) %-20s", I2A(i), sex_info[i].title.data());
        prt(buf, 5 + i, 5);
    }

    prt(_("どの性別に変わりますか:", "Which sex do you chenge: "), 0, 0);
    while (true) {
        i = inkey();

        if (i == ESCAPE) {
            screen_load();
            return false;
        }

        else if ((i < 'a') || (i > ('a' + MAX_SEXES - 1))) {
            continue;

        } else if (i - 'a' == creature.psex) {
            continue;
        }
        break;
    }

    creature.psex = static_cast<player_sex>(i - 'a');

    screen_load();
    const auto flags = { StatusRecalculatingFlag::BONUS, StatusRecalculatingFlag::HP, StatusRecalculatingFlag::MP, StatusRecalculatingFlag::SPELLS };
    RedrawingFlagsUpdater::get_instance().set_flags(flags);
    const auto flags2 = {
        MainWindowRedrawingFlag::BASIC,
        MainWindowRedrawingFlag::HP,
        MainWindowRedrawingFlag::MP,
        MainWindowRedrawingFlag::ABILITY_SCORE,
    };
    RedrawingFlagsUpdater::get_instance().set_flags(flags2);
    const auto flags3 = { SubWindowRedrawingFlag::PLAYER };
    RedrawingFlagsUpdater::get_instance().set_flags(flags3);

    sp_ptr = &sex_info[creature.psex];
    handle_stuff(creature);
    return true;
}
