#include "room/pit-nest-util.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race-hook.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "util/enum-converter.h"
#include "util/probability-table.h"

void nest_pit_type::prepare_filter(PlayerType *player_ptr) const
{
    auto &filter = PitNestFilter::get_instance();
    switch (this->pn_hook) {
    case PitNestHook::NONE:
        break;
    case PitNestHook::CLONE: {
        get_mon_num_prep_enum(player_ptr, MonraceHook::VAULT);
        const auto monrace_id = get_mon_num(player_ptr, 0, player_ptr->current_floor_ptr->dun_level + 10, PM_NONE);
        filter.set_monrace_id(monrace_id);
        get_mon_num_prep_enum(player_ptr);
        break;
    }
    case PitNestHook::SYMBOL: {
        get_mon_num_prep_enum(player_ptr, MonraceHook::VAULT);
        const auto monrace_id = get_mon_num(player_ptr, 0, player_ptr->current_floor_ptr->dun_level + 10, PM_NONE);
        get_mon_num_prep_enum(player_ptr);
        const auto symbol = MonraceList::get_instance().get_monrace(monrace_id).symbol_definition.character;
        filter.set_monrace_symbol(symbol);
        break;
    }
    case PitNestHook::DRAGON:
        filter.set_dragon_breaths();
        break;
    default:
        THROW_EXCEPTION(std::logic_error, format("Invalid hook! %d", enum2i(this->pn_hook)));
    }
}

/*!
 * @brief ダンジョン毎に指定されたピット配列を基準にランダムなnestタイプを決める
 * @param floor フロアへの参照
 * @param nest_types nest定義のマップ
 * @return 選択されたnestのID、選択失敗した場合nullopt.
 */
tl::optional<NestKind> pick_nest_type(const FloorType &floor, const std::map<NestKind, nest_pit_type> &nest_types)
{
    ProbabilityTable<NestKind> table;
    for (const auto &[nest_kind, nest] : nest_types) {
        if (nest.level > floor.dun_level) {
            continue;
        }

        if (none_bits(floor.get_dungeon_definition().nest, (1UL << enum2i(nest_kind)))) {
            continue;
        }

        table.entry_item(nest_kind, nest.chance * MAX_DEPTH / (std::min(floor.dun_level, MAX_DEPTH - 1) - nest.level + 5));
    }

    if (table.empty()) {
        return tl::nullopt;
    }

    return table.pick_one_at_random();
}

/*!
 * @brief ダンジョン毎に指定されたピット配列を基準にランダムなpitタイプを決める
 * @param floor フロアへの参照
 * @param pit_types pit定義のマップ
 * @return 選択されたpitのID、選択失敗した場合nullopt.
 */
tl::optional<PitKind> pick_pit_type(const FloorType &floor, const std::map<PitKind, nest_pit_type> &pit_types)
{
    ProbabilityTable<PitKind> table;
    for (const auto &[pit_kind, pit] : pit_types) {
        if (pit.level > floor.dun_level) {
            continue;
        }

        if (none_bits(floor.get_dungeon_definition().pit, (1UL << enum2i(pit_kind)))) {
            continue;
        }

        table.entry_item(pit_kind, pit.chance * MAX_DEPTH / (std::min(floor.dun_level, MAX_DEPTH - 1) - pit.level + 5));
    }

    if (table.empty()) {
        return tl::nullopt;
    }

    return table.pick_one_at_random();
}

/*!
 * @brief Pit/Nestに格納するモンスターを選択する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param align アライメントが中立に設定されたモンスター実体 (その他の中身は空)
 * @param boost 選択基準となるフロアの増分
 * @return モンスター種族ID (見つからなかったらnullopt)
 * @details Nestにはそのフロアの通常レベルより11高いモンスターを中心に選ぶ
 */
tl::optional<MonraceId> select_pit_nest_monrace_id(PlayerType *player_ptr, MonsterEntity &align, int boost)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &monraces = MonraceList::get_instance();
    for (auto attempts = 100; attempts > 0; attempts--) {
        const auto monrace_id = get_mon_num(player_ptr, 0, floor.dun_level + boost, PM_NONE);
        const auto &monrace = monraces.get_monrace(monrace_id);
        if (monster_has_hostile_to_other_monster(align, monrace)) {
            continue;
        }

        if (MonraceList::is_valid(monrace_id)) {
            return monrace_id;
        }

        return tl::nullopt;
    }

    return tl::nullopt;
}
