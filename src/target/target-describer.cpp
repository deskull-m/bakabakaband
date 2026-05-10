#include "target/target-describer.h"
#include "action/travel-execution.h"
#include "cmd-visual/cmd-draw.h"
#include "core/stuff-handler.h"
#include "dungeon/quest.h"
#include "flavor/flavor-describer.h"
#include "floor/geometry.h"
#include "floor/object-scanner.h"
#include "game-option/input-options.h"
#include "grid/grid.h"
#include "info-reader/fixed-map-parser.h"
#include "inventory/inventory-slot-types.h"
#include "io/cursor.h"
#include "io/input-key-acceptor.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "object/item-tester-hooker.h"
#include "player-base/player-race.h"
#include "player-info/race-types.h"
#include "player/player-status-table.h"
#include "system/building-type-definition.h"
#include "system/creature-entity.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/enums/grid-flow.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/floor-info.h"
#include "system/floor/town-info.h"
#include "system/floor/town-list.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/system-variables.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "target/target-types.h"
#include "term/screen-processor.h"
#include "timed-effect/timed-effects.h"
#include "tracking/lore-tracker.h"
#include "view/display-lore.h"
#include "view/display-messages.h"
#include "window/display-sub-windows.h"
#include "world/world.h"
#include <fmt/format.h>
#include <optional>
#include <vector>
#ifdef JP
#else
#include "locale/english.h"
#endif

namespace {
constexpr short CONTINUOUS_DESCRIPTION = 256;

class GridExamination {
public:
    GridExamination(FloorType &floor, POSITION y, POSITION x, target_type mode, concptr info);
    POSITION y;
    POSITION x;
    target_type mode;
    bool boring = true;
    concptr info;
    concptr s1 = "";
    concptr s2 = "";
    concptr s3 = "";
    concptr x_info = "";
    char query = '\001';
    std::vector<short> floor_item_index;
    Grid *g_ptr;
    CreatureEntity *m_ptr;
    OBJECT_IDX next_o_idx = 0;
    FEAT_IDX feat = 0;
    TerrainType *terrain_ptr = nullptr;
    std::string name = "";

    bool matches_terrain(TerrainTag tag) const;
    void set_terrain_id(TerrainTag tag);
    Pos2D get_position() const
    {
        return { this->y, this->x };
    }
};

GridExamination::GridExamination(FloorType &floor, POSITION y, POSITION x, target_type mode, concptr info)
    : y(y)
    , x(x)
    , mode(mode)
    , info(info)
{
    this->g_ptr = &floor.grid_array[y][x];
    this->m_ptr = &floor.get_monster(this->g_ptr->m_idx);
    this->next_o_idx = 0;
}

bool GridExamination::matches_terrain(TerrainTag tag) const
{
    return this->feat == TerrainList::get_instance().get_terrain_id(tag);
}

void GridExamination::set_terrain_id(TerrainTag tag)
{
    this->feat = TerrainList::get_instance().get_terrain_id(tag);
}
}

bool show_gold_on_floor = false;

/*
 * Evaluate number of kill needed to gain level
 */
static std::string evaluate_monster_exp(CreatureEntity &creature, const CreatureEntity &monster)
{
    const auto &monrace = monster.get_appearance_monrace();
    if ((creature.level >= PY_MAX_LEVEL) || CreatureRace(&creature).equals(PlayerRaceType::ANDROID)) {
        return "**";
    }

    if (!monrace.r_tkills || monster.get_monster_profile().mflag2.has(MonsterConstantFlagType::KAGE)) {
        if (!AngbandWorld::get_instance().wizard) {
            return "??";
        }
    }

    int32_t exp_mon = monrace.mexp * monrace.level;
    uint32_t exp_mon_frac = 0;
    s64b_div(&exp_mon, &exp_mon_frac, 0, (creature.max_plv + 2));

    int32_t exp_adv = player_exp[creature.level - 1] * creature.expfact;
    uint32_t exp_adv_frac = 0;
    s64b_div(&exp_adv, &exp_adv_frac, 0, 100);

    s64b_sub(&exp_adv, &exp_adv_frac, creature.exp, creature.exp_frac);

    s64b_add(&exp_adv, &exp_adv_frac, exp_mon, exp_mon_frac);
    s64b_sub(&exp_adv, &exp_adv_frac, 0, 1);

    s64b_div(&exp_adv, &exp_adv_frac, exp_mon, exp_mon_frac);

    const auto num = std::min<uint32_t>(999, exp_adv_frac);
    return fmt::format("{:03}", num);
}

static void describe_scan_result(const FloorType &floor, GridExamination *ge_ptr)
{
    if (!easy_floor) {
        return;
    }

    ge_ptr->floor_item_index = scan_floor_items(floor, ge_ptr->get_position(), { ScanFloorMode::ONLY_MARKED }, AllMatchItemTester());
    if (!ge_ptr->floor_item_index.empty()) {
        ge_ptr->x_info = _("x物 ", "x,");
    }
}

static void describe_target(CreatureEntity &creature, GridExamination *ge_ptr)
{
    if (!creature.is_located_at({ ge_ptr->y, ge_ptr->x })) {
        ge_ptr->s1 = _("ターゲット:", "Target:");
        return;
    }

#ifdef JP
    ge_ptr->s1 = "あなたは";
    ge_ptr->s2 = "の上";
    ge_ptr->s3 = "にいる";
#else
    ge_ptr->s1 = "You are ";
    ge_ptr->s2 = "on ";
#endif
}

static ProcessResult describe_hallucinated_target(CreatureEntity &creature, GridExamination *ge_ptr)
{
    if (!creature.is_hallucinated()) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    concptr name = _("何か奇妙な物", "something strange");
#ifdef JP
    const auto out_val = format("%s%s%s%s [%s]", ge_ptr->s1, name, ge_ptr->s2, ge_ptr->s3, ge_ptr->info);
#else
    const auto out_val = format("%s%s%s%s [%s]", ge_ptr->s1, ge_ptr->s2, ge_ptr->s3, name, ge_ptr->info);
#endif
    prt(out_val, 0, 0);
    move_cursor_relative(ge_ptr->y, ge_ptr->x);
    ge_ptr->query = inkey();
    if ((ge_ptr->query != '\r') && (ge_ptr->query != '\n')) {
        return ProcessResult::PROCESS_TRUE;
    }

    return ProcessResult::PROCESS_FALSE;
}

static bool describe_grid_lore(CreatureEntity &creature, GridExamination *ge_ptr)
{
    screen_save();
    screen_roff(creature, ge_ptr->m_ptr->ap_r_idx, MONSTER_LORE_NORMAL);
    term_addstr(-1, TERM_WHITE, format(_("  [r思 %s%s]", "  [r,%s%s]"), ge_ptr->x_info, ge_ptr->info));
    ge_ptr->query = inkey();
    screen_load();
    return ge_ptr->query != 'r';
}

static void describe_grid_monster(CreatureEntity &creature, GridExamination *ge_ptr)
{
    bool recall = false;
    const auto m_name = monster_desc(creature, *ge_ptr->m_ptr, MD_INDEF_VISIBLE);
    while (true) {
        if (recall) {
            if (describe_grid_lore(creature, ge_ptr)) {
                return;
            }

            recall = false;
            continue;
        }

        std::string acount = evaluate_monster_exp(creature, *ge_ptr->m_ptr);
        const auto mon_desc = ge_ptr->m_ptr->build_looking_description(true);
#ifdef JP
        const auto out_val = format("[%s]%s%s(%s)%s%s [r思 %s%s]", acount.data(), ge_ptr->s1, m_name.data(), mon_desc.data(), ge_ptr->s2, ge_ptr->s3,
            ge_ptr->x_info, ge_ptr->info);
#else
        const auto out_val = format("[%s]%s%s%s%s(%s) [r, %s%s]", acount.data(), ge_ptr->s1, ge_ptr->s2, ge_ptr->s3, m_name.data(), mon_desc.data(),
            ge_ptr->x_info, ge_ptr->info);
#endif
        prt(out_val, 0, 0);
        move_cursor_relative(ge_ptr->y, ge_ptr->x);
        ge_ptr->query = inkey();
        if (ge_ptr->query == 'c') {
            do_cmd_player_status(*ge_ptr->m_ptr);
            return;
        } else if (ge_ptr->query != 'r') {
            return;
        }

        recall = true;
    }
}

static void describe_monster_person(GridExamination *ge_ptr)
{
    ge_ptr->s1 = _("それは", "It is ");
    if (ge_ptr->m_ptr->is_female()) {
        ge_ptr->s1 = _("彼女は", "She is ");
    } else if (ge_ptr->m_ptr->is_male()) {
        ge_ptr->s1 = _("彼は", "He is ");
    }

#ifdef JP
    ge_ptr->s2 = "を";
    ge_ptr->s3 = "持っている";
#else
    ge_ptr->s2 = "carrying ";
#endif
}

static short describe_monster_item(CreatureEntity &creature, GridExamination *ge_ptr)
{
    // [フェーズ B-4] 装備スロット → パックスロットの順で表示し、
    // 装備品は「装備している」、パックは「持っている」と区別する
    auto show_one = [&](const ItemEntity &item, bool is_equipped, bool is_first) -> std::optional<short> {
        const auto item_name = describe_flavor(creature, item, 0);
        if (is_equipped) {
            ge_ptr->s2 = is_first ? _("を", "wielding ") : _("をまた", "also wielding ");
            ge_ptr->s3 = _("装備している", "");
        } else {
            ge_ptr->s2 = is_first ? _("を", "carrying ") : _("をまた", "also carrying ");
            ge_ptr->s3 = _("持っている", "");
        }
#ifdef JP
        const auto out_val = format("%s%s%s%s[%s]", ge_ptr->s1, item_name.data(), ge_ptr->s2, ge_ptr->s3, ge_ptr->info);
#else
        const auto out_val = format("%s%s%s%s [%s]", ge_ptr->s1, ge_ptr->s2, ge_ptr->s3, item_name.data(), ge_ptr->info);
#endif
        prt(out_val, 0, 0);
        move_cursor_relative(ge_ptr->y, ge_ptr->x);
        ge_ptr->query = inkey();
        if ((ge_ptr->query != '\r') && (ge_ptr->query != '\n') && (ge_ptr->query != ' ') && (ge_ptr->query != 'x')) {
            return ge_ptr->query;
        }
        if ((ge_ptr->query == ' ') && !(ge_ptr->mode & TARGET_LOOK)) {
            return ge_ptr->query;
        }
        return std::nullopt;
    };

    bool is_first = true;
    // 装備スロット (INVEN_MAIN_HAND..INVEN_TOTAL)
    for (size_t i = INVEN_MAIN_HAND; i < INVEN_TOTAL && i < ge_ptr->m_ptr->inventory.size(); i++) {
        const auto &item = *ge_ptr->m_ptr->inventory[i];
        if (!item.is_valid()) {
            continue;
        }
        if (auto early = show_one(item, true, is_first); early) {
            return *early;
        }
        is_first = false;
    }
    // パックスロット (0..INVEN_PACK)
    for (size_t i = 0; i < INVEN_PACK && i < ge_ptr->m_ptr->inventory.size(); i++) {
        const auto &item = *ge_ptr->m_ptr->inventory[i];
        if (!item.is_valid()) {
            continue;
        }
        if (auto early = show_one(item, false, is_first); early) {
            return *early;
        }
        is_first = false;
    }

    return CONTINUOUS_DESCRIPTION;
}

static bool within_char_util(const short input)
{
    return (input > -127) && (input < 128);
}

static short describe_grid(CreatureEntity &creature, GridExamination *ge_ptr)
{
    if (!ge_ptr->g_ptr->has_monster() || !creature.get_floor()->get_monster(ge_ptr->g_ptr->m_idx).is_visible_on_map()) {
        return CONTINUOUS_DESCRIPTION;
    }

    ge_ptr->boring = false;
    LoreTracker::get_instance().set_trackee(ge_ptr->m_ptr->ap_r_idx);
    health_track(creature, ge_ptr->g_ptr->m_idx);
    handle_stuff(creature);
    describe_grid_monster(creature, ge_ptr);
    if ((ge_ptr->query != '\r') && (ge_ptr->query != '\n') && (ge_ptr->query != ' ') && (ge_ptr->query != 'x')) {
        return ge_ptr->query;
    }

    if ((ge_ptr->query == ' ') && !(ge_ptr->mode & TARGET_LOOK)) {
        return ge_ptr->query;
    }

    describe_monster_person(ge_ptr);
    const auto monster_item_description = describe_monster_item(creature, ge_ptr);
    if (within_char_util(monster_item_description)) {
        return (char)monster_item_description;
    }

#ifdef JP
    ge_ptr->s2 = "の上";
    ge_ptr->s3 = "にいる";
#else
    ge_ptr->s2 = "on ";
#endif
    return CONTINUOUS_DESCRIPTION;
}

static short describe_footing(CreatureEntity &creature, GridExamination *ge_ptr)
{
    if (ge_ptr->floor_item_index.size() != 1) {
        return CONTINUOUS_DESCRIPTION;
    }

    const auto &item = *creature.get_floor()->o_list[ge_ptr->floor_item_index[0]];
    const auto item_name = describe_flavor(creature, item, 0);
#ifdef JP
    const auto out_val = format("%s%s%s%s[%s]", ge_ptr->s1, item_name.data(), ge_ptr->s2, ge_ptr->s3, ge_ptr->info);
#else
    const auto out_val = format("%s%s%s%s [%s]", ge_ptr->s1, ge_ptr->s2, ge_ptr->s3, item_name.data(), ge_ptr->info);
#endif
    prt(out_val, 0, 0);
    move_cursor_relative(ge_ptr->y, ge_ptr->x);
    ge_ptr->query = inkey();
    return ge_ptr->query;
}

static short describe_footing_items(GridExamination *ge_ptr)
{
    if (!ge_ptr->boring) {
        return CONTINUOUS_DESCRIPTION;
    }

#ifdef JP
    const auto out_val = fmt::format("{} {}個のアイテム{}{} ['x'で一覧, {}]", ge_ptr->s1, ge_ptr->floor_item_index.size(), ge_ptr->s2, ge_ptr->s3, ge_ptr->info);
#else
    const auto out_val = fmt::format("{}{}{}a pile of {} items [x,{}]", ge_ptr->s1, ge_ptr->s2, ge_ptr->s3, ge_ptr->floor_item_index.size(), ge_ptr->info);
#endif
    prt(out_val, 0, 0);
    move_cursor_relative(ge_ptr->y, ge_ptr->x);
    ge_ptr->query = inkey();
    if (ge_ptr->query != 'x' && ge_ptr->query != ' ') {
        return ge_ptr->query;
    }

    return CONTINUOUS_DESCRIPTION;
}

static char describe_footing_many_items(CreatureEntity &creature, GridExamination *ge_ptr, int *min_width)
{
    while (true) {
        screen_save();
        show_gold_on_floor = true;
        (void)show_floor_items(creature, 0, ge_ptr->y, ge_ptr->x, min_width, AllMatchItemTester());
        show_gold_on_floor = false;
#ifdef JP
        const auto out_val = fmt::format("{} {}個のアイテム{}{} [Enterで次へ, {}]", ge_ptr->s1, ge_ptr->floor_item_index.size(), ge_ptr->s2, ge_ptr->s3, ge_ptr->info);
#else
        const auto out_val = fmt::format("{}{}{}a pile of {} items [Enter,{}]", ge_ptr->s1, ge_ptr->s2, ge_ptr->s3, ge_ptr->floor_item_index.size(), ge_ptr->info);
#endif
        prt(out_val, 0, 0);
        ge_ptr->query = inkey();
        screen_load();
        if (ge_ptr->query != '\n' && ge_ptr->query != '\r') {
            return ge_ptr->query;
        }

        if (ge_ptr->g_ptr->o_idx_list.size() < 2) {
            continue;
        }

        ge_ptr->g_ptr->o_idx_list.rotate(*creature.get_floor());

        // ターゲットしている床の座標を渡す必要があるので、window_stuff経由ではなく直接呼び出す
        fix_floor_item_list(creature, { ge_ptr->y, ge_ptr->x });
    }
}

static short loop_describing_grid(CreatureEntity &creature, GridExamination *ge_ptr)
{
    if (ge_ptr->floor_item_index.empty()) {
        return CONTINUOUS_DESCRIPTION;
    }

    auto min_width = 0;
    while (true) {
        const auto footing_description = describe_footing(creature, ge_ptr);
        if (within_char_util(footing_description)) {
            return (char)footing_description;
        }

        const auto footing_descriptions = describe_footing_items(ge_ptr);
        if (within_char_util(footing_descriptions)) {
            return (char)footing_descriptions;
        }

        return describe_footing_many_items(creature, ge_ptr, &min_width);
    }
}

static short describe_footing_sight(CreatureEntity &creature, GridExamination *ge_ptr, const ItemEntity &item)
{
    if (item.marked.has_not(OmType::FOUND)) {
        return CONTINUOUS_DESCRIPTION;
    }

    ge_ptr->boring = false;
    const auto item_name = describe_flavor(creature, item, 0);
#ifdef JP
    const auto out_val = format("%s%s%s%s[%s]", ge_ptr->s1, item_name.data(), ge_ptr->s2, ge_ptr->s3, ge_ptr->info);
#else
    const auto out_val = format("%s%s%s%s [%s]", ge_ptr->s1, ge_ptr->s2, ge_ptr->s3, item_name.data(), ge_ptr->info);
#endif
    prt(out_val, 0, 0);
    move_cursor_relative(ge_ptr->y, ge_ptr->x);
    ge_ptr->query = inkey();
    if ((ge_ptr->query != '\r') && (ge_ptr->query != '\n') && (ge_ptr->query != ' ') && (ge_ptr->query != 'x')) {
        return ge_ptr->query;
    }

    if ((ge_ptr->query == ' ') && !(ge_ptr->mode & TARGET_LOOK)) {
        return ge_ptr->query;
    }

    ge_ptr->s1 = item.number == 1 ? _("それは", "It is ") : _("それらは", "They are ");
#ifdef JP
    ge_ptr->s2 = "の上";
    ge_ptr->s3 = "に見える";
#else
    ge_ptr->s2 = "on ";
#endif
    return CONTINUOUS_DESCRIPTION;
}

static int16_t sweep_footing_items(CreatureEntity &creature, GridExamination *ge_ptr)
{
    for (const auto this_o_idx : ge_ptr->g_ptr->o_idx_list) {
        const auto &item = *creature.get_floor()->o_list[this_o_idx];
        const auto ret = describe_footing_sight(creature, ge_ptr, item);
        if (within_char_util(ret)) {
            return (char)ret;
        }
    }

    return CONTINUOUS_DESCRIPTION;
}

static std::string decide_target_floor(CreatureEntity &creature, GridExamination *ge_ptr)
{
    auto &floor = *creature.get_floor();
    if (ge_ptr->terrain_ptr->flags.has(TerrainCharacteristics::QUEST_ENTER)) {
        const auto old_quest = floor.quest_number;
        const auto &quests = QuestList::get_instance();
        const auto quest_id = i2enum<QuestId>(ge_ptr->g_ptr->special);
        const auto &quest = quests.get_quest(quest_id);
        constexpr auto fmt = _("クエスト「%s」(%d階相当)", "the entrance to the quest '%s'(level %d)");

        quest_text_lines.clear();

        floor.quest_number = quest_id;
        init_flags = INIT_NAME_ONLY;
        parse_fixed_map(creature, QUEST_DEFINITION_LIST, 0, 0, 0, 0);
        floor.quest_number = old_quest;
        return format(fmt, quest.name.data(), quest.level);
    }

    if (ge_ptr->terrain_ptr->flags.has(TerrainCharacteristics::BLDG) && !floor.inside_arena) {
        return buildings[ge_ptr->terrain_ptr->subtype].name;
    }

    if (ge_ptr->terrain_ptr->flags.has(TerrainCharacteristics::ENTRANCE)) {
        const auto dungeon_id = i2enum<DungeonId>(ge_ptr->g_ptr->special);
        const auto &dungeon = DungeonList::get_instance().get_dungeon(dungeon_id);
        return dungeon.describe_depth();
    }

    if (ge_ptr->terrain_ptr->flags.has(TerrainCharacteristics::TOWN)) {
        return towns_info[ge_ptr->g_ptr->special].name;
    }

    if (AngbandWorld::get_instance().is_wild_mode() && (ge_ptr->matches_terrain(TerrainTag::FLOOR))) {
        return _("道", "road");
    }

    return ge_ptr->terrain_ptr->name;
}

static std::string describe_grid_monster_all(GridExamination *ge_ptr)
{
    if (!AngbandWorld::get_instance().wizard) {
#ifdef JP
        return format("%s%s%s%s[%s]", ge_ptr->s1, ge_ptr->name.data(), ge_ptr->s2, ge_ptr->s3, ge_ptr->info);
#else
        return format("%s%s%s%s [%s]", ge_ptr->s1, ge_ptr->s2, ge_ptr->s3, ge_ptr->name.data(), ge_ptr->info);
#endif
    }

    std::string f_idx_str;
    if (ge_ptr->g_ptr->mimic) {
        f_idx_str = format("%d/%d", ge_ptr->g_ptr->feat, ge_ptr->g_ptr->mimic);
    } else {
        f_idx_str = std::to_string(ge_ptr->g_ptr->feat);
    }

#ifdef JP
    return format("%s%s%s%s[%s] %x %s %d %d %d (%d,%d) %d", ge_ptr->s1, ge_ptr->name.data(), ge_ptr->s2, ge_ptr->s3, ge_ptr->info,
        (uint)ge_ptr->g_ptr->info, f_idx_str.data(), ge_ptr->g_ptr->dists[GridFlow::NORMAL], ge_ptr->g_ptr->costs[GridFlow::NORMAL], ge_ptr->g_ptr->when,
        ge_ptr->y, ge_ptr->x, Travel::get_instance().get_cost({ ge_ptr->y, ge_ptr->x }));
#else
    return format("%s%s%s%s [%s] %x %s %d %d %d (%d,%d)", ge_ptr->s1, ge_ptr->s2, ge_ptr->s3, ge_ptr->name.data(), ge_ptr->info, ge_ptr->g_ptr->info,
        f_idx_str.data(), ge_ptr->g_ptr->dists[GridFlow::NORMAL], ge_ptr->g_ptr->costs[GridFlow::NORMAL], ge_ptr->g_ptr->when, ge_ptr->y, ge_ptr->x);
#endif
}

/*!
 * @brief xまたはlで指定したグリッドにあるアイテムやモンスターの説明を記述する
 * @param creature クリーチャーへの参照
 * @param y 指定グリッドのY座標
 * @param x 指定グリッドのX座標
 * @param mode x (KILL)かl (LOOK)
 * @param info 記述用文字列
 * @return 入力キー
 * @todo xとlで処理を分ける？
 */
char examine_grid(CreatureEntity &creature, const POSITION y, const POSITION x, target_type mode, concptr info)
{
    auto &floor = *creature.get_floor();
    GridExamination tmp_eg(floor, y, x, mode, info);
    GridExamination *ge_ptr = &tmp_eg;
    describe_scan_result(floor, ge_ptr);
    describe_target(creature, ge_ptr);
    ProcessResult next_target = describe_hallucinated_target(creature, ge_ptr);
    switch (next_target) {
    case ProcessResult::PROCESS_FALSE:
        return '\0';
    case ProcessResult::PROCESS_TRUE:
        return ge_ptr->query;
    default:
        break;
    }

    const auto description_grid = describe_grid(creature, ge_ptr);
    if (within_char_util(description_grid)) {
        return (char)description_grid;
    }

    const auto loop_description = loop_describing_grid(creature, ge_ptr);
    if (within_char_util(loop_description)) {
        return (char)loop_description;
    }

    const auto footing_items_description = sweep_footing_items(creature, ge_ptr);
    if (within_char_util(footing_items_description)) {
        return (char)footing_items_description;
    }

    ge_ptr->feat = ge_ptr->g_ptr->get_terrain_id(TerrainKind::MIMIC);
    if (!ge_ptr->g_ptr->is_mark() && !player_can_see_bold(creature, y, x)) {
        ge_ptr->set_terrain_id(TerrainTag::NONE);
    }

    ge_ptr->terrain_ptr = &TerrainList::get_instance().get_terrain(ge_ptr->feat);
    if (!ge_ptr->boring && ge_ptr->terrain_ptr->flags.has_not(TerrainCharacteristics::REMEMBER)) {
        return (ge_ptr->query != '\r') && (ge_ptr->query != '\n') ? ge_ptr->query : 0;
    }

    /*
     * グローバル変数への代入をここで行っているので動かしたくない
     * 安全を確保できたら構造体から外すことも検討する
     */
    ge_ptr->name = decide_target_floor(creature, ge_ptr);
    auto is_in = ge_ptr->terrain_ptr->flags.has_none_of({ TerrainCharacteristics::MOVE, TerrainCharacteristics::CAN_FLY });
    is_in |= ge_ptr->terrain_ptr->flags.has_none_of({ TerrainCharacteristics::LOS, TerrainCharacteristics::TREE });
    is_in |= ge_ptr->terrain_ptr->flags.has(TerrainCharacteristics::TOWN);
    if (*ge_ptr->s2 && is_in) {
        ge_ptr->s2 = _("の中", "in ");
    }

    auto is_entrance = ge_ptr->terrain_ptr->flags.has(TerrainCharacteristics::STORE);
    is_entrance |= ge_ptr->terrain_ptr->flags.has(TerrainCharacteristics::QUEST_ENTER);
    is_entrance |= ge_ptr->terrain_ptr->flags.has(TerrainCharacteristics::BLDG) && !creature.get_floor()->inside_arena;
    is_entrance |= ge_ptr->terrain_ptr->flags.has(TerrainCharacteristics::ENTRANCE);
    if (is_entrance) {
        ge_ptr->s2 = _("の入口", "");
    }
#ifdef JP
#else
    else {
        auto is_normal_terrain = ge_ptr->terrain_ptr->flags.has(TerrainCharacteristics::FLOOR);
        is_normal_terrain |= ge_ptr->terrain_ptr->flags.has(TerrainCharacteristics::TOWN);
        is_normal_terrain |= ge_ptr->terrain_ptr->flags.has(TerrainCharacteristics::SHALLOW);
        is_normal_terrain |= ge_ptr->terrain_ptr->flags.has(TerrainCharacteristics::DEEP);
        if (is_normal_terrain) {
            ge_ptr->s3 = "";
        } else {
            ge_ptr->s3 = (is_a_vowel(ge_ptr->name[0])) ? "an " : "a ";
        }
    }
#endif

    prt(describe_grid_monster_all(ge_ptr), 0, 0);
    move_cursor_relative(y, x);
    ge_ptr->query = inkey();
    if ((ge_ptr->query != '\r') && (ge_ptr->query != '\n')) {
        return ge_ptr->query;
    }

    return 0;
}
