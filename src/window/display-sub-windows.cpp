#include "window/display-sub-windows.h"
#include "flavor/flavor-describer.h"
#include "game-option/option-flags.h"
#include "game-option/special-options.h"
#include "game-option/text-display-options.h"
#include "inventory/inventory-describer.h"
#include "inventory/inventory-util.h"
#include "locale/japanese.h"
#include "main/sound-of-music.h"
#include "mind/mind-elementalist.h"
#include "mind/mind-explanations-table.h"
#include "mind/mind-info.h"
#include "mind/mind-sniper.h"
#include "mind/mind-types.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "object/item-tester-hooker.h"
#include "object/object-info.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player/player-realm.h"
#include "player/player-spell-status.h"
#include "player/player-status-flags.h"
#include "player/player-status-table.h"
#include "player/player-status.h"
#include "spell/spells-execution.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "target/target-preparation.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "timed-effect/timed-effects.h"
#include "tracking/lore-tracker.h"
#include "util/buffer-shaper.h"
#include "util/int-char-converter.h"
#include "util/object-sort.h"
#include "view/display-lore.h"
#include "view/display-map.h"
#include "view/display-messages.h"
#include "view/display-player.h"
#include "view/object-describer.h"
#include "window/main-window-equipments.h"
#include "window/main-window-util.h"
#include "world/world.h"
#include <algorithm>
#include <concepts>
#include <mutex>
#include <sstream>
#include <string>

/*! サブウィンドウ表示用の ItemTester オブジェクト */
static std::unique_ptr<ItemTester> fix_item_tester = std::make_unique<AllMatchItemTester>();

FixItemTesterSetter::FixItemTesterSetter(const ItemTester &item_tester)
{
    fix_item_tester = item_tester.clone();
}

FixItemTesterSetter::~FixItemTesterSetter()
{
    fix_item_tester = std::make_unique<AllMatchItemTester>();
}

/*!
 * @brief サブウィンドウの描画を行う
 *
 * pw_flag で指定したウィンドウフラグが設定されているサブウィンドウに対し描画を行う。
 * 描画は display_func で指定したコールバック関数で行う。
 *
 * @param pw_flag 描画を行うフラグ
 * @param display_func 描画を行う関数
 */
static void display_sub_windows(SubWindowRedrawingFlag pw_flag, std::invocable auto display_func)
{
    auto current_term = game_term;

    for (auto i = 0U; i < angband_terms.size(); ++i) {
        auto term = angband_terms[i];
        if (term == nullptr) {
            continue;
        }

        if (!g_window_flags[i].has(pw_flag)) {
            continue;
        }

        term_activate(term);
        display_func();
        term_fresh();
    }

    term_activate(current_term);
}

/*!
 * @brief サブウィンドウに所持品一覧を表示する / Hack -- display inventory in sub-windows
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void fix_inventory(PlayerType *player_ptr)
{
    display_sub_windows(SubWindowRedrawingFlag::INVENTORY,
        [player_ptr] {
            display_inventory(player_ptr, *fix_item_tester);
        });
}

/*!
 * @brief モンスターの現在数を一行で表現する / Print monster info in line
 * @param x 表示列
 * @param y 表示行
 * @param m_ptr 思い出を表示するモンスター情報の参照ポインタ
 * @param n_same モンスター数の現在数
 * @param n_awake 起きている数
 * @details
 * <pre>
 * nnn X LV name
 *  nnn : number or unique(U) or wanted unique(W)
 *  X   : symbol of monster
 *  LV  : monster lv if known
 *  name: name of monster
 * </pre>
 */
static void print_monster_line(TERM_LEN x, TERM_LEN y, const MonsterEntity &monster, int n_same, int n_awake)
{
    term_erase(0, y);
    term_gotoxy(x, y);
    const auto &monrace = monster.get_appearance_monrace();
    if (!monrace.is_valid()) {
        return;
    }

    std::string buf;
    if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
        buf = format(_("%3s(覚%2d)", "%3s(%2d)"), monrace.is_bounty(true) ? "  W" : "  U", n_awake);
    } else {
        buf = format(_("%3d(覚%2d)", "%3d(%2d)"), n_same, n_awake);
        term_addstr(-1, TERM_WHITE, buf);
    }

    term_addstr(-1, TERM_WHITE, buf);
    term_addstr(-1, TERM_WHITE, " ");
    term_add_bigch(monrace.symbol_config);

    if (monrace.r_tkills && monster.mflag2.has_not(MonsterConstantFlagType::KAGE)) {
        buf = format(" %2d", monrace.level);
    } else {
        buf = "???";
    }

    term_addstr(-1, TERM_WHITE, buf);
    term_addstr(-1, TERM_WHITE, format(" %s ", monrace.name.data()));
}

/*!
 * @brief モンスターの出現リストを表示する / Print monster info in line
 * @param x 表示列
 * @param y 表示行
 * @param max_lines 最大何行描画するか
 */
void print_monster_list(const FloorType &floor, const std::vector<MONSTER_IDX> &monster_list, TERM_LEN x, TERM_LEN y, TERM_LEN max_lines)
{
    TERM_LEN line = y;
    struct info {
        const MonsterEntity *monster_entity;
        int visible_count; // 現在数
        int awake_count; // 起きている数
    };

    // 出現リスト表示用のデータ
    std::vector<info> monster_list_info;

    // 描画に必要なデータを集める
    for (auto monster_index : monster_list) {
        const auto &monster = floor.m_list[monster_index];

        if (monster.is_pet()) {
            continue;
        } // pet
        if (!monster.is_valid()) {
            continue;
        } // dead?

        // ソート済みなので同じモンスターは連続する．これを利用して同じモンスターをカウント，まとめて表示する．
        if (monster_list_info.empty() || (monster_list_info.back().monster_entity->ap_r_idx != monster.ap_r_idx)) {
            monster_list_info.push_back({ &monster, 0, 0 });
        }

        // 出現数をカウント
        monster_list_info.back().visible_count++;

        // 起きているモンスターをカウント
        if (!monster.is_asleep()) {
            monster_list_info.back().awake_count++;
        }
    }

    // 集めたデータを元にリストを表示する
    for (const auto &info : monster_list_info) {
        print_monster_line(x, line++, *info.monster_entity, info.visible_count, info.awake_count);

        // 行数が足りなくなったら中断。
        if (line - y == max_lines) {
            // 行数が足りなかった場合、最終行にその旨表示。
            term_addstr(-1, TERM_WHITE, "-- and more --");
            break;
        }
    }

    for (; line < max_lines; line++) {
        term_erase(0, line);
    }
}

static void print_pet_list_oneline(PlayerType *player_ptr, const MonsterEntity &monster, TERM_LEN x, TERM_LEN y, TERM_LEN width)
{
    const auto &monrace = monster.get_appearance_monrace();
    const auto name = monster_desc(player_ptr, monster, MD_ASSUME_VISIBLE | MD_INDEF_VISIBLE | MD_NO_OWNER);
    const auto &[bar_color, bar_len] = monster.get_hp_bar_data();
    const auto is_visible = monster.ml && !player_ptr->effects()->hallucination().is_hallucinated();

    term_erase(0, y);
    if (is_visible) {
        term_putstr(x, y, -1, TERM_WHITE, "[----------]");
        term_putstr(x + 1, y, bar_len, bar_color, "**********");
    }

    term_gotoxy(x + 13, y);
    term_add_bigch(monrace.symbol_config);
    term_addstr(-1, TERM_WHITE, " ");
    term_addstr(-1, TERM_WHITE, name);

    if (width >= 50) {
        const auto location = format(" (X:%3d Y:%3d)", monster.fx, monster.fy);
        prt(is_visible ? location : "", y, width - location.length());
    }
}

static void print_pet_list(PlayerType *player_ptr, const std::vector<MONSTER_IDX> &pets, TERM_LEN x, TERM_LEN y, TERM_LEN width, TERM_LEN height)
{
    for (auto n = 0U; n < pets.size(); ++n) {
        const auto &monster = player_ptr->current_floor_ptr->m_list[pets[n]];
        const int line = y + n;

        print_pet_list_oneline(player_ptr, monster, x, line, width);

        if ((line == height - 2) && (n < pets.size() - 2)) {
            term_erase(0, line + 1);
            term_putstr(x, line + 1, -1, TERM_WHITE, "-- and more --");
            break;
        }
    }

    for (int n = pets.size(); n < height; ++n) {
        term_erase(0, y + n);
    }
}

/*!
 * @brief 出現中モンスターのリストをサブウィンドウに表示する / Hack -- display monster list in sub-windows
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void fix_monster_list(PlayerType *player_ptr)
{
    static std::vector<MONSTER_IDX> monster_list;
    std::once_flag once;

    display_sub_windows(SubWindowRedrawingFlag::SIGHT_MONSTERS,
        [player_ptr, &once] {
            const auto &[wid, hgt] = term_get_size();
            std::call_once(once, target_sensing_monsters_prepare, player_ptr, monster_list);
            print_monster_list(*player_ptr->current_floor_ptr, monster_list, 0, 0, hgt);
        });

    if (use_music && has_monster_music) {
        std::call_once(once, target_sensing_monsters_prepare, player_ptr, monster_list);
        select_monster_music(player_ptr, monster_list);
    }
}

/*!
 * @brief 視界内のペットのリストをサブウィンドウに表示する
 */
void fix_pet_list(PlayerType *player_ptr)
{
    display_sub_windows(SubWindowRedrawingFlag::PETS,
        [player_ptr] {
            const auto &[wid, hgt] = term_get_size();
            const auto pets = target_pets_prepare(player_ptr);
            print_pet_list(player_ptr, pets, 0, 0, wid, hgt);
        });
}

/*!
 * @brief 装備アイテム一覧を表示する /
 * Choice window "shadow" of the "show_equip()" function
 */
static void display_equipment(PlayerType *player_ptr, const ItemTester &item_tester)
{
    if (!player_ptr || player_ptr->inventory.empty()) {
        return;
    }

    const auto &[wid, hgt] = term_get_size();
    byte attr = TERM_WHITE;
    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        int cur_row = i - INVEN_MAIN_HAND;
        if (cur_row >= hgt) {
            break;
        }

        const auto &item = *player_ptr->inventory[i];
        auto do_disp = player_ptr->select_ring_slot ? is_ring_slot(i) : item_tester.okay(&item);
        std::string tmp_val = "   ";

        if (do_disp) {
            tmp_val[0] = index_to_label(i);
            tmp_val[1] = ')';
        }

        int cur_col = 3;
        term_erase(cur_col, cur_row);
        term_putstr(0, cur_row, cur_col, TERM_WHITE, tmp_val);

        std::string item_name;
        auto is_two_handed = (i == INVEN_MAIN_HAND) && can_attack_with_sub_hand(player_ptr);
        is_two_handed |= (i == INVEN_SUB_HAND) && can_attack_with_main_hand(player_ptr);
        if (is_two_handed && has_two_handed_weapons(player_ptr)) {
            item_name = _("(武器を両手持ち)", "(wielding with two-hands)");
            attr = TERM_WHITE;
        } else {
            item_name = describe_flavor(player_ptr, item, 0);
            attr = tval_to_attr[enum2i(item.bi_key.tval()) % 128];
        }

        int n = item_name.length();
        if (item.timeout) {
            attr = TERM_L_DARK;
        }

        if (show_item_graph) {
            term_queue_bigchar(cur_col, cur_row, { item.get_symbol(), {} });
            if (use_bigtile) {
                cur_col++;
            }

            cur_col += 2;
        }

        term_putstr(cur_col, cur_row, n, attr, item_name);
        if (show_weights) {
            int wgt = item.weight * item.number;
            tmp_val = format(_("%3d.%1d kg", "%3d.%1d lb"), _(lb_to_kg_integer(wgt), wgt / 10), _(lb_to_kg_fraction(wgt), wgt % 10));
            prt(tmp_val, cur_row, wid - (show_labels ? 28 : 9));
        }

        if (show_labels) {
            term_putstr(wid - 20, cur_row, -1, TERM_WHITE, " <-- ");
            prt(mention_use(player_ptr, i), cur_row, wid - 15);
        }
    }

    for (int i = INVEN_TOTAL - INVEN_MAIN_HAND; i < hgt; i++) {
        term_erase(0, i);
    }
}

/*!
 * @brief 現在の装備品をサブウィンドウに表示する /
 * Hack -- display equipment in sub-windows
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void fix_equip(PlayerType *player_ptr)
{
    display_sub_windows(SubWindowRedrawingFlag::EQUIPMENT,
        [player_ptr] {
            display_equipment(player_ptr, *fix_item_tester);
        });
}

/*!
 * @brief 現在のプレイヤーステータスをサブウィンドウに表示する /
 * @param player_ptr プレイヤーへの参照ポインタ
 * Hack -- display character in sub-windows
 */
void fix_player(PlayerType *player_ptr)
{
    AngbandWorld::get_instance().play_time.update();
    display_sub_windows(SubWindowRedrawingFlag::PLAYER,
        [player_ptr] {
            display_player(player_ptr, 0);
        });
}

/*!
 * @brief ゲームメッセージ履歴をサブウィンドウに表示する /
 * Hack -- display recent messages in sub-windows
 * Adjust for width and split messages
 */
void fix_message(void)
{
    display_sub_windows(SubWindowRedrawingFlag::MESSAGE,
        [] {
            const auto &[wid, hgt] = term_get_size();

            for (auto y = 0; y < hgt; ++y) {
                term_erase(0, y);
            }

            auto displayed_lines = 0;
            for (auto i = 0; i < message_num() && displayed_lines < hgt; ++i) {
                const auto color = (i < now_message) ? TERM_WHITE : TERM_SLATE;

                const auto msg = message_str(i);
                auto lines = shape_buffer(*msg, wid);
                std::reverse(lines.begin(), lines.end());

                for (const auto &line : lines) {
                    const auto y = hgt - 1 - displayed_lines;
                    if (y < 0) {
                        break;
                    }

                    term_putstr(0, y, -1, color, line);
                    displayed_lines++;
                }
            }
        });
}

/*!
 * @brief 簡易マップをサブウィンドウに表示する /
 * Hack -- display overhead view in sub-windows
 * Adjust for width and split messages
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * Note that the "player" symbol does NOT appear on the map.
 */
void fix_overhead(PlayerType *player_ptr)
{
    display_sub_windows(SubWindowRedrawingFlag::OVERHEAD,
        [player_ptr] {
            const auto &[wid, hgt] = term_get_size();
            if (wid > COL_MAP + 2 && hgt > ROW_MAP + 2) {
                int cy, cx;
                display_map(player_ptr, &cy, &cx);
            }
        });
}

/*!
 * @brief 自分の周辺の地形をTermに表示する
 * @param プレイヤー情報への参照ポインタ
 */
static void display_dungeon(PlayerType *player_ptr)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto p_pos = player_ptr->get_position();
    for (auto x = p_pos.x - game_term->wid / 2 + 1; x <= p_pos.x + game_term->wid / 2; x++) {
        for (auto y = p_pos.y - game_term->hgt / 2 + 1; y <= p_pos.y + game_term->hgt / 2; y++) {
            const Pos2D pos(y, x);
            const auto pos_drawing = pos - p_pos + Pos2DVec(game_term->hgt / 2 - 1, game_term->wid / 2 - 1);
            if (!floor.contains(pos, FloorBoundary::OUTER_WALL_INCLUSIVE)) {
                const auto &terrain = TerrainList::get_instance().get_terrain(TerrainTag::NONE);
                const auto &symbol_foreground = terrain.symbol_configs.at(F_LIT_STANDARD);
                term_queue_char(pos_drawing.x, pos_drawing.y, { symbol_foreground, {} });
                continue;
            }

            auto symbol_pair = map_info(player_ptr, pos);
            symbol_pair.symbol_foreground.color = get_monochrome_display_color(player_ptr).value_or(symbol_pair.symbol_foreground.color);
            term_queue_char(pos_drawing.x, pos_drawing.y, symbol_pair);
        }
    }
}

/*!
 * @brief 自分の周辺のダンジョンの地形をサブウィンドウに表示する / display dungeon view around player in a sub window
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void fix_dungeon(PlayerType *player_ptr)
{
    display_sub_windows(SubWindowRedrawingFlag::DUNGEON,
        [player_ptr] {
            display_dungeon(player_ptr);
        });
}

/*!
 * @brief モンスターの思い出をサブウィンドウに表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void fix_monster(PlayerType *player_ptr)
{
    if (!LoreTracker::get_instance().is_tracking()) {
        return;
    }

    display_sub_windows(SubWindowRedrawingFlag::MONSTER_LORE,
        [player_ptr] {
            display_roff(player_ptr);
        });
}

/*!
 * @brief ベースアイテム情報をサブウィンドウに表示する /
 * Hack -- display object recall in sub-windows
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void fix_object(PlayerType *player_ptr)
{
    display_sub_windows(SubWindowRedrawingFlag::ITEM_KNOWLEDGE,
        [player_ptr] {
            display_koff(player_ptr);
        });
}

/*!
 * @brief 指定したグリッドにモンスターが見えるかどうかを判定する
 * @param floor フロアへの参照
 * @param grid グリッドへの参照
 * @return モンスターが見えるかどうか
 * @details
 * Lookコマンドでカーソルを合わせた場合に合わせてミミックは考慮しない。
 */
static bool is_seeing_monster_on(const FloorType &floor, const Grid &grid)
{
    if (!grid.has_monster()) {
        return false;
    }

    const auto &monster = floor.m_list[grid.m_idx];
    return monster.is_valid() && monster.ml;
}

/*!
 * @brief 床上のアイテム一覧を作成し、表示する
 * @param プレイヤー情報への参照ポインタ
 * @param y 参照する座標グリッドのy座標
 * @param x 参照する座標グリッドのx座標
 */
static void display_floor_item_list(PlayerType *player_ptr, const Pos2D &pos)
{
    const auto &[wid, hgt] = term_get_size();
    if (hgt <= 0) {
        return;
    }

    term_clear();
    term_gotoxy(0, 0);

    const auto floor_ptr = player_ptr->current_floor_ptr;
    const auto *g_ptr = &floor_ptr->get_grid(pos);
    std::string line;

    // 先頭行を書く。
    const auto is_hallucinated = player_ptr->effects()->hallucination().is_hallucinated();
    if (player_ptr->is_located_at(pos)) {
        line = format(_("(X:%03d Y:%03d) あなたの足元のアイテム一覧", "Items at (%03d,%03d) under you"), pos.x, pos.y);
    } else if (is_seeing_monster_on(*floor_ptr, *g_ptr)) {
        if (is_hallucinated) {
            line = format(_("(X:%03d Y:%03d) 何か奇妙な物の足元の発見済みアイテム一覧", "Found items at (%03d,%03d) under something strange"), pos.x, pos.y);
        } else {
            const auto &monster = floor_ptr->m_list[g_ptr->m_idx];
            const auto &monrace = monster.get_appearance_monrace();
            line = format(_("(X:%03d Y:%03d) %sの足元の発見済みアイテム一覧", "Found items at (%03d,%03d) under %s"), pos.x, pos.y, monrace.name.data());
        }
    } else {
        const auto &terrain = g_ptr->get_terrain();
        const auto fn = terrain.name.data();
        std::string buf;
        if (terrain.flags.has(TerrainCharacteristics::STORE) || (terrain.flags.has(TerrainCharacteristics::BLDG) && !floor_ptr->inside_arena)) {
            buf = format(_("%sの入口", "on the entrance of %s"), fn);
        } else if (terrain.flags.has(TerrainCharacteristics::WALL)) {
            buf = format(_("%sの中", "in %s"), fn);
        } else {
            buf = format(_("%s", "on %s"), fn);
        }
        line = format(_("(X:%03d Y:%03d) %sの上の発見済みアイテム一覧", "Found items at (X:%03d Y:%03d) %s"), pos.x, pos.y, buf.data());
    }
    term_addstr(-1, TERM_WHITE, line);

    // (y,x) のアイテムを1行に1個ずつ書く。
    TERM_LEN term_y = 1;
    for (const auto o_idx : g_ptr->o_idx_list) {
        const auto &item = *floor_ptr->o_list[o_idx];
        const auto tval = item.bi_key.tval();
        if (item.marked.has_not(OmType::FOUND) || tval == ItemKindType::GOLD) {
            continue;
        }

        // 途中で行数が足りなくなったら最終行にその旨追記して終了。
        if (term_y >= hgt) {
            term_addstr(-1, TERM_WHITE, "-- more --");
            break;
        }

        term_gotoxy(0, term_y);

        if (is_hallucinated) {
            term_addstr(-1, TERM_WHITE, _("何か奇妙な物", "something strange"));
        } else {
            const auto item_name = describe_flavor(player_ptr, item, 0);
            TERM_COLOR attr = tval_to_attr[enum2i(tval) % 128];
            term_addstr(-1, attr, item_name);
        }

        ++term_y;
    }
}

/*!
 * @brief (y,x) のアイテム一覧をサブウィンドウに表示する / display item at (y,x) in sub-windows
 */
void fix_floor_item_list(PlayerType *player_ptr, const Pos2D &pos)
{
    display_sub_windows(SubWindowRedrawingFlag::FLOOR_ITEMS,
        [player_ptr, pos] {
            display_floor_item_list(player_ptr, pos);
        });
}

/*!
 * @brief 発見済みのアイテム一覧を作成し、表示する
 * @param プレイヤー情報への参照ポインタ
 */
static void display_found_item_list(PlayerType *player_ptr)
{
    const auto &[wid, hgt] = term_get_size();
    if (hgt <= 0) {
        return;
    }

    const auto &floor = *player_ptr->current_floor_ptr;

    // 所持品一覧と同じ順にソートする
    // あらかじめfloor.o_list から↓項目を取り除く
    // bi_idが0
    // OM_FOUNDフラグが立っていない
    // ItemKindTypeがGOLD
    std::vector<const ItemEntity *> found_item_list;
    for (auto &item_ptr : floor.o_list) {
        const auto is_item_to_display =
            item_ptr->is_valid() && (item_ptr->number > 0) &&
            item_ptr->marked.has(OmType::FOUND) && (item_ptr->bi_key.tval() != ItemKindType::GOLD);

        if (is_item_to_display) {
            found_item_list.push_back(item_ptr.get());
        }
    }

    std::sort(
        found_item_list.begin(), found_item_list.end(),
        [player_ptr](const ItemEntity *left, const ItemEntity *right) -> bool {
            return object_sort_comp(player_ptr, *left, *right);
        });

    term_clear();
    term_gotoxy(0, 0);

    // 先頭行を書く。
    term_addstr(-1, TERM_WHITE, _("発見済みのアイテム一覧", "Found items"));

    // 発見済みのアイテムを表示
    TERM_LEN term_y = 1;
    for (auto item_ptr : found_item_list) {
        // 途中で行数が足りなくなったら終了。
        if (term_y >= hgt) {
            break;
        }

        term_gotoxy(0, term_y);

        // アイテムシンボル表示
        const auto symbol = item_ptr->get_symbol();
        const auto symbol_str = format(" %c ", symbol.character);
        term_addstr(-1, symbol.color, symbol_str);

        const auto item_name = describe_flavor(player_ptr, *item_ptr, 0);
        const auto color_code_for_item = tval_to_attr[enum2i(item_ptr->bi_key.tval()) % 128];
        term_addstr(-1, color_code_for_item, item_name);

        // アイテム座標表示
        const auto item_location = format("(X:%3d Y:%3d)", item_ptr->ix, item_ptr->iy);
        prt(item_location, term_y, wid - item_location.length() - 1);

        ++term_y;
    }
}

/*!
 * @brief 発見済みのアイテム一覧をサブウィンドウに表示する
 */
void fix_found_item_list(PlayerType *player_ptr)
{
    display_sub_windows(SubWindowRedrawingFlag::FOUND_ITEMS,
        [player_ptr] {
            display_found_item_list(player_ptr);
        });
}

/*!
 * @brief プレイヤーの全既知呪文を表示する / Display all known spells in a window
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * Need to analyze size of the window.
 * Need more color coding.
 */
static void display_spell_list(PlayerType *player_ptr)
{
    TERM_LEN y, x;
    int m[9]{};

    clear_from(0);

    PlayerClass pc(player_ptr);
    if (pc.is_every_magic()) {
        return;
    }

    if (pc.equals(PlayerClassType::SNIPER)) {
        display_snipe_list(player_ptr);
        return;
    }

    if (pc.equals(PlayerClassType::ELEMENTALIST)) {
        display_element_spell_list(player_ptr);
        return;
    }

    if (pc.has_listed_magics()) {
        PERCENTAGE minfail = 0;
        PLAYER_LEVEL plev = player_ptr->lev;
        PERCENTAGE chance = 0;
        MindKindType use_mind;
        bool use_hp = false;

        y = 1;
        x = 1;

        prt("", y, x);
        put_str(_("名前", "Name"), y, x + 5);
        put_str(_("Lv   MP 失率 効果", "Lv Mana Fail Info"), y, x + 35);

        switch (player_ptr->pclass) {
        case PlayerClassType::MINDCRAFTER:
            use_mind = MindKindType::MINDCRAFTER;
            break;
        case PlayerClassType::FORCETRAINER:
            use_mind = MindKindType::KI;
            break;
        case PlayerClassType::BERSERKER:
            use_mind = MindKindType::BERSERKER;
            use_hp = true;
            break;
        case PlayerClassType::MIRROR_MASTER:
            use_mind = MindKindType::MIRROR_MASTER;
            break;
        case PlayerClassType::NINJA:
            use_mind = MindKindType::NINJUTSU;
            use_hp = true;
            break;
        default:
            use_mind = MindKindType::MINDCRAFTER;
            break;
        }

        const auto &mind_power = mind_powers[enum2i(use_mind)];
        for (int i = 0; i < std::ssize(mind_power.info); i++) {
            byte a = TERM_WHITE;
            const auto &spell = mind_power.info[i];
            if (spell.min_lev > plev) {
                break;
            }

            chance = spell.fail;
            chance -= 3 * (player_ptr->lev - spell.min_lev);
            chance -= 3 * (adj_mag_stat[player_ptr->stat_index[mp_ptr->spell_stat]] - 1);
            if (!use_hp) {
                if (spell.mana_cost > player_ptr->csp) {
                    chance += 5 * (spell.mana_cost - player_ptr->csp);
                    a = TERM_ORANGE;
                }
            } else {
                if (spell.mana_cost > player_ptr->chp) {
                    chance += 100;
                    a = TERM_RED;
                }
            }

            minfail = adj_mag_fail[player_ptr->stat_index[mp_ptr->spell_stat]];
            if (chance < minfail) {
                chance = minfail;
            }

            chance += player_ptr->effects()->stun().get_magic_chance_penalty();
            if (chance > 95) {
                chance = 95;
            }

            const auto comment = mindcraft_info(player_ptr, use_mind, i);
            constexpr auto fmt = "  %c) %-30s%2d %4d %3d%%%s";
            term_putstr(x, y + i + 1, -1, a, format(fmt, I2A(i), spell.name, spell.min_lev, spell.mana_cost, chance, comment.data()));
        }

        return;
    }

    PlayerRealm pr(player_ptr);
    if (!pr.realm1().is_available()) {
        return;
    }

    for (int j = 0; j < (pr.realm2().is_available() ? 2 : 1); j++) {
        m[j] = 0;
        y = (j < 3) ? 0 : (m[j - 3] + 2);
        x = 27 * (j % 3);
        int n = 0;

        PlayerSpellStatus pss(player_ptr);
        const auto realm_status = (j < 1) ? pss.realm1() : pss.realm2();

        for (auto spell_id = 0; spell_id < 32; spell_id++) {
            byte a = TERM_WHITE;

            const auto &realm = (j < 1) ? pr.realm1() : pr.realm2();
            const auto &spell = realm.get_spell_info(spell_id);
            const auto &spell_name = realm.get_spell_name(spell_id);
            auto name = spell_name.data();

            if (spell.slevel >= 99) {
                name = _("(判読不能)", "(illegible)");
                a = TERM_L_DARK;
            } else if (realm_status.is_forgotten(spell_id)) {
                a = TERM_ORANGE;
            } else if (!realm_status.is_learned(spell_id)) {
                a = TERM_RED;
            } else if (!realm_status.is_worked(spell_id)) {
                a = TERM_YELLOW;
            }

            m[j] = y + n;
            term_putstr(x, m[j], -1, a, format("%c/%c) %-20.20s", I2A(n / 8), I2A(n % 8), name));
            n++;
        }
    }
}

/*!
 * @brief 現在の習得済魔法をサブウィンドウに表示する /
 * @param player_ptr プレイヤーへの参照ポインタ
 * Hack -- display spells in sub-windows
 */
void fix_spell(PlayerType *player_ptr)
{
    display_sub_windows(SubWindowRedrawingFlag::SPELL,
        [player_ptr] {
            display_spell_list(player_ptr);
        });
}

/*!
 * @brief サブウィンドウに所持品、装備品リストの表示を行う
 */
void toggle_inventory_equipment()
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    for (auto i = 0U; i < angband_terms.size(); ++i) {
        if (!angband_terms[i]) {
            continue;
        }

        if (g_window_flags[i].has(SubWindowRedrawingFlag::INVENTORY)) {
            g_window_flags[i].reset(SubWindowRedrawingFlag::INVENTORY);
            g_window_flags[i].set(SubWindowRedrawingFlag::EQUIPMENT);
            rfu.set_flag(SubWindowRedrawingFlag::EQUIPMENT);
            continue;
        }

        if (g_window_flags[i].has(SubWindowRedrawingFlag::EQUIPMENT)) {
            g_window_flags[i].reset(SubWindowRedrawingFlag::EQUIPMENT);
            g_window_flags[i].set(SubWindowRedrawingFlag::INVENTORY);
            rfu.set_flag(SubWindowRedrawingFlag::INVENTORY);
        }
    }
}
