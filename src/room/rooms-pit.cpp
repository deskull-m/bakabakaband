#include "room/rooms-pit.h"
#include "game-option/cheat-options.h"
#include "game-option/cheat-types.h"
#include "grid/door.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster/monster-util.h"
#include "room/door-definition.h"
#include "room/pit-nest-util.h"
#include "room/space-finder.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"
#include "wizard/wizard-messages.h"

namespace {
constexpr auto NUM_PIT_MONRACES = 16; //!< pit内に存在する最大モンスター種族数.

/*!
 * @brief 生成するPitの情報テーブル
 */
const std::map<PitKind, nest_pit_type> pit_types = {
    { PitKind::ORC, { _("オーク", "orc"), vault_aux_orc, std::nullopt, 5, 6 } },
    { PitKind::TROLL, { _("トロル", "troll"), vault_aux_troll, std::nullopt, 20, 6 } },
    { PitKind::GIANT, { _("巨人", "giant"), vault_aux_giant, std::nullopt, 50, 6 } },
    { PitKind::LOVECRAFTIAN, { _("狂気", "lovecraftian"), vault_aux_cthulhu, std::nullopt, 80, 2 } },
    { PitKind::SYMBOL_GOOD, { _("シンボル(善)", "symbol good"), vault_aux_symbol_g, vault_prep_symbol, 70, 1 } },
    { PitKind::SYMBOL_EVIL, { _("シンボル(悪)", "symbol evil"), vault_aux_symbol_e, vault_prep_symbol, 70, 1 } },
    { PitKind::CHAPEL, { _("教会", "chapel"), vault_aux_chapel_g, std::nullopt, 65, 2 } },
    { PitKind::DRAGON, { _("ドラゴン", "dragon"), vault_aux_dragon, vault_prep_dragon, 70, 6 } },
    { PitKind::DEMON, { _("デーモン", "demon"), vault_aux_demon, std::nullopt, 80, 6 } },
    { PitKind::DARK_ELF, { _("ダークエルフ", "dark elf"), vault_aux_dark_elf, std::nullopt, 45, 4 } },
    { PitKind::GAY, { _("ホモ", "gay"), vault_aux_gay, std::nullopt, 5, 4 } },
    { PitKind::LES, { _("レズ", "lez"), vault_aux_les, std::nullopt, 5, 4 } },
};

class TrappedMonster {
public:
    TrappedMonster(const Pos2D &pos, int strength)
        : pos(pos)
        , strength(strength)
    {
    }

    Pos2D pos;
    int strength;
};

// clang-format off
/*!
 * @brief 開門トラップのモンスター配置テーブル
 * @details
 * 中央からの相対座標(X,Y)、モンスターの強さ
 */
const std::vector<TrappedMonster> place_table_trapped_pit = {
    { { -2, -9 }, 0 }, { { -2, -8 }, 0 }, { { -3, -7 }, 0 }, { { -3, -6 }, 0 }, { { +2, -9 }, 0 }, { { +2, -8 }, 0 }, { { +3, -7 }, 0 }, { { +3, -6 }, 0 },
    { { -2, +9 }, 0 }, { { -2, +8 }, 0 }, { { -3, +7 }, 0 }, { { -3, +6 }, 0 }, { { +2, +9 }, 0 }, { { +2, +8 }, 0 }, { { +3, +7 }, 0 }, { { +3, +6 }, 0 },
    { { -2, -7 }, 1 }, { { -3, -5 }, 1 }, { { -3, -4 }, 1 }, { { -2, +7 }, 1 }, { { -3, +5 }, 1 }, { { -3, +4 }, 1 },
    { { +2, -7 }, 1 }, { { +3, -5 }, 1 }, { { +3, -4 }, 1 }, { { +2, +7 }, 1 }, { { +3, +5 }, 1 }, { { +3, +4 }, 1 },
    { { -2, -6 }, 2 }, { { -2, -5 }, 2 }, { { -3, -3 }, 2 }, { { -2, +6 }, 2 }, { { -2, +5 }, 2 }, { { -3, +3 }, 2 },
    { { +2, -6 }, 2 }, { { +2, -5 }, 2 }, { { +3, -3 }, 2 }, { { +2, +6 }, 2 }, { { +2, +5 }, 2 }, { { +3, +3 }, 2 },
    { { -2, -4 }, 3 }, { { -3, -2 }, 3 }, { { -2, +4 }, 3 }, { { -3, +2 }, 3 },
    { { +2, -4 }, 3 }, { { +3, -2 }, 3 }, { { +2, +4 }, 3 }, { { +3, +2 }, 3 },
    { { -2, -3 }, 4 }, { { -3, -1 }, 4 }, { { +2, -3 }, 4 }, { { +3, -1 }, 4 },
    { { -2, +3 }, 4 }, { { -3, +1 }, 4 }, { { +2, +3 }, 4 }, { { +3, +1 }, 4 },
    { { -2, -2 }, 5 }, { { -3, 0 }, 5 }, { { -2, +2 }, 5 }, { { +2, -2 }, 5 }, { { +3, 0 }, 5 }, { { +2, +2 }, 5 },
    { { -2, -1 }, 6 }, { { -2, +1 }, 6 }, { { +2, -1 }, 6 }, { { +2, +1 }, 6 },
    { { -2, 0 }, 7 }, { { +2, 0 }, 7 },
};
// clang-format on

std::optional<std::array<MonsterRaceId, NUM_PIT_MONRACES>> pick_pit_monraces(PlayerType *player_ptr, MonsterEntity &align, int boost = 0)
{
    std::array<MonsterRaceId, NUM_PIT_MONRACES> whats{};
    for (auto &what : whats) {
        const auto monrace_id = select_pit_nest_monrace_id(player_ptr, align, boost);
        if (!monrace_id) {
            return std::nullopt;
        }

        const auto &monrace = monraces_info[*monrace_id];
        if (monrace.kind_flags.has(MonsterKindType::EVIL)) {
            align.sub_align |= SUB_ALIGN_EVIL;
        }

        if (monrace.kind_flags.has(MonsterKindType::GOOD)) {
            align.sub_align |= SUB_ALIGN_GOOD;
        }

        what = *monrace_id;
    }

    return whats;
}
}

/*!
 * @brief タイプ6の部屋…pitを生成する / Type 6 -- Monster pits
 * @details
 * A monster pit is a "big" room, with an "inner" room, containing\n
 * a "collection" of monsters of a given type organized in the room.\n
 *\n
 * The inside room in a monster pit appears as shown below, where the\n
 * actual monsters in each location depend on the type of the pit\n
 *\n
 *   XXXXXXXXXXXXXXXXXXXXX\n
 *   X0000000000000000000X\n
 *   X0112233455543322110X\n
 *   X0112233467643322110X\n
 *   X0112233455543322110X\n
 *   X0000000000000000000X\n
 *   XXXXXXXXXXXXXXXXXXXXX\n
 *\n
 * Note that the monsters in the pit are now chosen by using "get_mon_num()"\n
 * to request 16 "appropriate" monsters, sorting them by level, and using\n
 * the "even" entries in this sorted list for the contents of the pit.\n
 *\n
 * Hack -- all of the "dragons" in a "dragon" pit must be the same "color",\n
 * which is handled by requiring a specific "breath" attack for all of the\n
 * dragons.  This may include "multi-hued" breath.  Note that "wyrms" may\n
 * be present in many of the dragon pits, if they have the proper breath.\n
 *\n
 * Note the use of the "get_mon_num_prep()" function, and the special\n
 * "get_mon_num_hook()" restriction function, to prepare the "monster\n
 * allocation table" in such a way as to optimize the selection of\n
 * "appropriate" non-unique monsters for the pit.\n
 *\n
 * Note that the "get_mon_num()" function may (rarely) fail, in which case\n
 * the pit will be empty.\n
 *\n
 * Note that "monster pits" will never contain "unique" monsters.\n
 */
bool build_type6(PlayerType *player_ptr, dun_data_type *dd_ptr)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const auto pit_type = pick_pit_type(floor, pit_types);
    if (!pit_type) {
        return false;
    }

    const auto &pit = pit_types.at(*pit_type);

    /* Process a preparation function if necessary */
    if (pit.prep_func) {
        (*(pit.prep_func))(player_ptr);
    }

    get_mon_num_prep(player_ptr, pit.hook_func, nullptr);
    MonsterEntity align;
    align.sub_align = SUB_ALIGN_NEUTRAL;

    auto whats = pick_pit_monraces(player_ptr, align, 11);
    if (!whats) {
        return false;
    }

    int yval;
    int xval;
    if (!find_space(player_ptr, dd_ptr, &yval, &xval, 11, 25)) {
        return false;
    }

    const Pos2D center(yval, xval);
    constexpr Pos2DVec vec(4, 11);
    const Rect2D rectangle(center, vec);

    /* Place the floor area */
    for (auto y = rectangle.top_left.y - 1; y <= rectangle.bottom_right.y + 1; y++) {
        for (auto x = rectangle.top_left.x - 1; x <= rectangle.bottom_right.x + 1; x++) {
            auto &grid = floor.get_grid({ y, x });
            place_grid(player_ptr, &grid, GB_FLOOR);
            grid.add_info(CAVE_ROOM);
        }
    }

    /* Place the outer walls */
    for (auto y = rectangle.top_left.y - 1; y <= rectangle.bottom_right.y + 1; y++) {
        place_grid(player_ptr, &floor.get_grid({ y, rectangle.top_left.x - 1 }), GB_OUTER);
        place_grid(player_ptr, &floor.get_grid({ y, rectangle.bottom_right.x + 1 }), GB_OUTER);
    }
    for (auto x = rectangle.top_left.x - 1; x <= rectangle.bottom_right.x + 1; x++) {
        place_grid(player_ptr, &floor.get_grid({ rectangle.top_left.y - 1, x }), GB_OUTER);
        place_grid(player_ptr, &floor.get_grid({ rectangle.bottom_right.y + 1, x }), GB_OUTER);
    }

    /* Advance to the center room */
    const auto rectangle_inner = rectangle.resized(-2);

    /* The inner walls */
    for (auto y = rectangle_inner.top_left.y - 1; y <= rectangle_inner.bottom_right.y + 1; y++) {
        place_grid(player_ptr, &floor.get_grid({ y, rectangle_inner.top_left.x - 1 }), GB_INNER);
        place_grid(player_ptr, &floor.get_grid({ y, rectangle_inner.bottom_right.x + 1 }), GB_INNER);
    }
    for (auto x = rectangle_inner.top_left.x - 1; x <= rectangle_inner.bottom_right.x + 1; x++) {
        place_grid(player_ptr, &floor.get_grid({ rectangle_inner.top_left.y - 1, x }), GB_INNER);
        place_grid(player_ptr, &floor.get_grid({ rectangle_inner.bottom_right.y + 1, x }), GB_INNER);
    }
    for (auto y = rectangle_inner.top_left.y; y <= rectangle_inner.bottom_right.y; y++) {
        for (auto x = rectangle_inner.top_left.x; x <= rectangle_inner.bottom_right.x; x++) {
            floor.get_grid({ y, x }).add_info(CAVE_ICKY);
        }
    }

    /* Place a secret door */
    switch (randint1(4)) {
    case 1:
        place_secret_door(player_ptr, rectangle_inner.top_left.y - 1, xval, DOOR_DEFAULT);
        break;
    case 2:
        place_secret_door(player_ptr, rectangle_inner.bottom_right.y + 1, xval, DOOR_DEFAULT);
        break;
    case 3:
        place_secret_door(player_ptr, yval, rectangle_inner.top_left.x - 1, DOOR_DEFAULT);
        break;
    case 4:
        place_secret_door(player_ptr, yval, rectangle_inner.bottom_right.x + 1, DOOR_DEFAULT);
        break;
    }

    /* Sort the entries */
    for (auto i = 0; i < NUM_PIT_MONRACES - 1; i++) {
        /* Sort the entries */
        for (auto j = 0; j < NUM_PIT_MONRACES - 1; j++) {
            int i1 = j;
            int i2 = j + 1;

            int p1 = monraces_info[(*whats)[i1]].level;
            int p2 = monraces_info[(*whats)[i2]].level;

            /* Bubble */
            if (p1 > p2) {
                std::swap((*whats)[i1], (*whats)[i2]);
            }
        }
    }

    constexpr auto fmt_generate = _("モンスター部屋(pit)(%s%s)を生成します。", "Monster pit (%s%s)");
    msg_format_wizard(
        player_ptr, CHEAT_DUNGEON, fmt_generate, pit.name.data(), pit_subtype_string(*pit_type).data());

    /* Select the entries */
    for (auto i = 0; i < NUM_PIT_MONRACES / 2; i++) {
        /* Every other entry */
        (*whats)[i] = (*whats)[i * 2];
        constexpr auto fmt_pit_num = _("Pit構成モンスター選択No.%d:%s", "Pit Monster Select No.%d:%s");
        msg_format_wizard(player_ptr, CHEAT_DUNGEON, fmt_pit_num, i, monraces_info[(*whats)[i]].name.data());
    }

    /* Top and bottom rows */
    for (auto x = xval - 9; x <= xval + 9; x++) {
        place_specific_monster(player_ptr, 0, yval - 2, x, (*whats)[0], PM_NO_KAGE);
        place_specific_monster(player_ptr, 0, yval + 2, x, (*whats)[0], PM_NO_KAGE);
    }

    /* Middle columns */
    for (auto y = yval - 1; y <= yval + 1; y++) {
        place_specific_monster(player_ptr, 0, y, xval - 9, (*whats)[0], PM_NO_KAGE);
        place_specific_monster(player_ptr, 0, y, xval + 9, (*whats)[0], PM_NO_KAGE);

        place_specific_monster(player_ptr, 0, y, xval - 8, (*whats)[1], PM_NO_KAGE);
        place_specific_monster(player_ptr, 0, y, xval + 8, (*whats)[1], PM_NO_KAGE);

        place_specific_monster(player_ptr, 0, y, xval - 7, (*whats)[1], PM_NO_KAGE);
        place_specific_monster(player_ptr, 0, y, xval + 7, (*whats)[1], PM_NO_KAGE);

        place_specific_monster(player_ptr, 0, y, xval - 6, (*whats)[2], PM_NO_KAGE);
        place_specific_monster(player_ptr, 0, y, xval + 6, (*whats)[2], PM_NO_KAGE);

        place_specific_monster(player_ptr, 0, y, xval - 5, (*whats)[2], PM_NO_KAGE);
        place_specific_monster(player_ptr, 0, y, xval + 5, (*whats)[2], PM_NO_KAGE);

        place_specific_monster(player_ptr, 0, y, xval - 4, (*whats)[3], PM_NO_KAGE);
        place_specific_monster(player_ptr, 0, y, xval + 4, (*whats)[3], PM_NO_KAGE);

        place_specific_monster(player_ptr, 0, y, xval - 3, (*whats)[3], PM_NO_KAGE);
        place_specific_monster(player_ptr, 0, y, xval + 3, (*whats)[3], PM_NO_KAGE);

        place_specific_monster(player_ptr, 0, y, xval - 2, (*whats)[4], PM_NO_KAGE);
        place_specific_monster(player_ptr, 0, y, xval + 2, (*whats)[4], PM_NO_KAGE);
    }

    /* Above/Below the center monster */
    for (auto x = xval - 1; x <= xval + 1; x++) {
        place_specific_monster(player_ptr, 0, yval + 1, x, (*whats)[5], PM_NO_KAGE);
        place_specific_monster(player_ptr, 0, yval - 1, x, (*whats)[5], PM_NO_KAGE);
    }

    /* Next to the center monster */
    place_specific_monster(player_ptr, 0, yval, xval + 1, (*whats)[6], PM_NO_KAGE);
    place_specific_monster(player_ptr, 0, yval, xval - 1, (*whats)[6], PM_NO_KAGE);

    /* Center monster */
    place_specific_monster(player_ptr, 0, yval, xval, (*whats)[7], PM_NO_KAGE);

    return true;
}

/*!
 * @brief 開門トラップに配置するモンスターの条件フィルタ
 * @detai;
 * 穴を掘るモンスター、壁を抜けるモンスターは却下
 */
static bool vault_aux_trapped_pit(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    /* Unused */
    (void)player_ptr;

    auto *r_ptr = &monraces_info[r_idx];

    if (!vault_monster_okay(player_ptr, r_idx)) {
        return false;
    }

    /* No wall passing monster */
    if (r_ptr->feature_flags.has_any_of({ MonsterFeatureType::PASS_WALL, MonsterFeatureType::KILL_WALL })) {
        return false;
    }

    return true;
}

/*!
 * @brief タイプ13の部屋…開門トラップpitの生成 / Type 13 -- Trapped monster pits
 * @details
 * A trapped monster pit is a "big" room with a straight corridor in\n
 * which wall opening traps are placed, and with two "inner" rooms\n
 * containing a "collection" of monsters of a given type organized in\n
 * the room.\n
 *\n
 * The trapped monster pit appears as shown below, where the actual\n
 * monsters in each location depend on the type of the pit\n
 *\n
 *  XXXXXXXXXXXXXXXXXXXXXXXXX\n
 *  X                       X\n
 *  XXXXXXXXXXXXXXXXXXXXXXX X\n
 *  XXXXX001123454321100XXX X\n
 *  XXX0012234567654322100X X\n
 *  XXXXXXXXXXXXXXXXXXXXXXX X\n
 *  X           ^           X\n
 *  X XXXXXXXXXXXXXXXXXXXXXXX\n
 *  X X0012234567654322100XXX\n
 *  X XXX001123454321100XXXXX\n
 *  X XXXXXXXXXXXXXXXXXXXXXXX\n
 *  X                       X\n
 *  XXXXXXXXXXXXXXXXXXXXXXXXX\n
 *\n
 * Note that the monsters in the pit are now chosen by using "get_mon_num()"\n
 * to request 16 "appropriate" monsters, sorting them by level, and using\n
 * the "even" entries in this sorted list for the contents of the pit.\n
 *\n
 * Hack -- all of the "dragons" in a "dragon" pit must be the same "color",\n
 * which is handled by requiring a specific "breath" attack for all of the\n
 * dragons.  This may include "multi-hued" breath.  Note that "wyrms" may\n
 * be present in many of the dragon pits, if they have the proper breath.\n
 *\n
 * Note the use of the "get_mon_num_prep()" function, and the special\n
 * "get_mon_num_hook()" restriction function, to prepare the "monster\n
 * allocation table" in such a way as to optimize the selection of\n
 * "appropriate" non-unique monsters for the pit.\n
 *\n
 * Note that the "get_mon_num()" function may (rarely) fail, in which case\n
 * the pit will be empty.\n
 *\n
 * Note that "monster pits" will never contain "unique" monsters.\n
 */
bool build_type13(PlayerType *player_ptr, dun_data_type *dd_ptr)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const auto pit_type = pick_pit_type(floor, pit_types);

    /* Only in Angband */
    if (floor.dungeon_idx != DUNGEON_ANGBAND) {
        return false;
    }

    /* No type available */
    if (!pit_type) {
        return false;
    }

    const auto &pit = pit_types.at(*pit_type);

    /* Process a preparation function if necessary */
    if (pit.prep_func) {
        (*(pit.prep_func))(player_ptr);
    }

    get_mon_num_prep(player_ptr, pit.hook_func, vault_aux_trapped_pit);
    MonsterEntity align;
    align.sub_align = SUB_ALIGN_NEUTRAL;
    auto whats = pick_pit_monraces(player_ptr, align);
    if (!whats) {
        return false;
    }

    int yval;
    int xval;
    if (!find_space(player_ptr, dd_ptr, &yval, &xval, 13, 25)) {
        return false;
    }

    const Pos2D center(yval, xval);
    constexpr Pos2DVec vec_rectangle(5, 11);
    const Rect2D rectangle(center, vec_rectangle);

    /* Fill with inner walls */
    for (auto y = rectangle.top_left.y - 1; y <= rectangle.bottom_right.y + 1; y++) {
        for (auto x = rectangle.top_left.x - 1; x <= rectangle.bottom_right.x + 1; x++) {
            auto &grid = floor.get_grid({ y, x });
            place_grid(player_ptr, &grid, GB_INNER);
            grid.add_info(CAVE_ROOM);
        }
    }

    /* Place the floor area 1 */
    for (auto x = rectangle.top_left.x + 3; x <= rectangle.bottom_right.x - 3; x++) {
        auto &grid_top = floor.get_grid({ yval - 2, x });
        place_grid(player_ptr, &grid_top, GB_FLOOR);
        grid_top.add_info(CAVE_ICKY);

        auto &grid_bottom = floor.get_grid({ yval + 2, x });
        place_grid(player_ptr, &grid_bottom, GB_FLOOR);
        grid_bottom.add_info(CAVE_ICKY);
    }

    /* Place the floor area 2 */
    for (auto x = rectangle.top_left.x + 5; x <= rectangle.bottom_right.x - 5; x++) {
        auto &grid_left = floor.get_grid({ yval - 3, x });
        place_grid(player_ptr, &grid_left, GB_FLOOR);
        grid_left.add_info(CAVE_ICKY);

        auto &grid_right = floor.get_grid({ yval + 3, x });
        place_grid(player_ptr, &grid_right, GB_FLOOR);
        grid_right.add_info(CAVE_ICKY);
    }

    /* Corridor */
    for (auto x = rectangle.top_left.x; x <= rectangle.bottom_right.x; x++) {
        place_grid(player_ptr, &floor.get_grid({ yval, x }), GB_FLOOR);
        place_grid(player_ptr, &floor.get_grid({ rectangle.top_left.y, x }), GB_FLOOR);
        place_grid(player_ptr, &floor.get_grid({ rectangle.bottom_right.y, x }), GB_FLOOR);
    }

    /* Place the outer walls */
    for (auto y = rectangle.top_left.y - 1; y <= rectangle.bottom_right.y + 1; y++) {
        place_grid(player_ptr, &floor.get_grid({ y, rectangle.top_left.x - 1 }), GB_OUTER);
        place_grid(player_ptr, &floor.get_grid({ y, rectangle.bottom_right.x + 1 }), GB_OUTER);
    }

    for (auto x = rectangle.top_left.x - 1; x <= rectangle.bottom_right.x + 1; x++) {
        place_grid(player_ptr, &floor.get_grid({ rectangle.top_left.y - 1, x }), GB_OUTER);
        place_grid(player_ptr, &floor.get_grid({ rectangle.bottom_right.y + 1, x }), GB_OUTER);
    }

    /* Random corridor */
    if (one_in_(2)) {
        for (auto y = rectangle.top_left.y; y <= yval; y++) {
            place_bold(player_ptr, y, rectangle.bottom_right.x, GB_FLOOR);
            place_bold(player_ptr, y, rectangle.top_left.x - 1, GB_SOLID);
        }
        for (auto y = yval; y <= rectangle.bottom_right.y + 1; y++) {
            place_bold(player_ptr, y, rectangle.top_left.x, GB_FLOOR);
            place_bold(player_ptr, y, rectangle.bottom_right.x + 1, GB_SOLID);
        }
    } else {
        for (auto y = yval; y <= rectangle.bottom_right.y + 1; y++) {
            place_bold(player_ptr, y, rectangle.top_left.x, GB_FLOOR);
            place_bold(player_ptr, y, rectangle.bottom_right.x + 1, GB_SOLID);
        }
        for (auto y = rectangle.top_left.y; y <= yval; y++) {
            place_bold(player_ptr, y, rectangle.bottom_right.x, GB_FLOOR);
            place_bold(player_ptr, y, rectangle.top_left.x - 1, GB_SOLID);
        }
    }

    /* Place the wall open trap */
    const Pos2D pos(yval, xval);
    auto &grid = floor.get_grid(pos);
    grid.mimic = grid.feat;
    grid.feat = feat_trap_open;

    /* Sort the entries */
    for (auto i = 0; i < NUM_PIT_MONRACES - 1; i++) {
        /* Sort the entries */
        for (auto j = 0; j < NUM_PIT_MONRACES - 1; j++) {
            int i1 = j;
            int i2 = j + 1;

            int p1 = monraces_info[(*whats)[i1]].level;
            int p2 = monraces_info[(*whats)[i2]].level;

            /* Bubble */
            if (p1 > p2) {
                std::swap((*whats)[i1], (*whats)[i2]);
            }
        }
    }

    constexpr auto fmt = _("%s%sの罠ピットが生成されました。", "Trapped monster pit (%s%s)");
    msg_format_wizard(player_ptr, CHEAT_DUNGEON, fmt, pit.name.data(), pit_subtype_string(*pit_type).data());

    /* Select the entries */
    for (auto i = 0; i < NUM_PIT_MONRACES / 2; i++) {
        /* Every other entry */
        (*whats)[i] = (*whats)[i * 2];

        if (cheat_hear) {
            msg_print(monraces_info[(*whats)[i]].name);
        }
    }

    const Pos2DVec vec(yval, xval);
    for (const auto &trapped_monster : place_table_trapped_pit) {
        const auto trapped_pos = trapped_monster.pos + vec;
        place_specific_monster(player_ptr, 0, trapped_pos.y, trapped_pos.x, (*whats)[trapped_monster.strength], PM_NO_KAGE);
    }

    return true;
}
