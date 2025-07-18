#include "view/display-map.h"
#include "autopick/autopick-finder.h"
#include "autopick/autopick-methods-table.h"
#include "autopick/autopick-util.h"
#include "game-option/map-screen-options.h"
#include "game-option/special-options.h"
#include "grid/grid.h"
#include "player/player-status.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "view/display-symbol.h"
#include "window/main-window-util.h"
#include "world/world.h"
#include <algorithm>
#include <span>

uint8_t display_autopick; /*!< 自動拾い状態の設定フラグ */

namespace {
/* 一般的にオブジェクトシンボルとして扱われる記号を定義する(幻覚処理向け) /  Hack -- Legal object codes */
const std::string image_objects = R"(?/|\"!$()_-=[]{},~)";

/* 一般的にモンスターシンボルとして扱われる記号を定義する(幻覚処理向け) / Hack -- Legal monster codes */
const std::string image_monsters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

/*!
 * @brief オブジェクトの表示を幻覚状態に差し替える
 * @return 差し替えたシンボルと色
 */
DisplaySymbol image_object()
{
    if (use_graphics) {
        const auto &baseitem = BaseitemList::get_instance().pick_one_at_random();
        return baseitem.symbol_config;
    }

    const auto color = randnum1<uint8_t>(15);
    const auto character = rand_choice(image_objects);
    return { color, character }; //!< @details 乱数引数の評価順を固定する.
}

/*!
 * @brief モンスターの表示を幻覚状態に差し替える
 * @return 差し替えたシンボルと色
 */
DisplaySymbol image_monster()
{
    if (use_graphics) {
        const auto &monraces = MonraceList::get_instance();
        const auto &monrace = monraces.pick_monrace_at_random();
        return monrace.symbol_config;
    }

    const auto color = randnum1<uint8_t>(15);
    const auto character = one_in_(25) ? rand_choice(image_objects) : rand_choice(image_monsters);
    return { color, character };
}

/*!
 * @brief オブジェクト＆モンスターの表示を幻覚状態に差し替える
 * @return 差し替えたシンボルと色
 */
DisplaySymbol image_random()
{
    if (evaluate_percent(75)) {
        return image_monster();
    } else {
        return image_object();
    }
}
}

/*!
 * @brief マップに表示されるべき地形(壁)かどうかを判定する
 * @param floor 階の情報への参照
 * @param pos グリッド座標
 * @return 表示されるべきならtrue、そうでないならfalse
 * @details
 * 周り全てが壁に囲まれている壁についてはオプション状態による。
 * 1か所でも空きがあるか、壁ではない地形、金を含む地形、永久岩は表示。
 */
static bool is_revealed_wall(const FloorType &floor, const Pos2D &pos)
{
    const auto &grid = floor.get_grid(pos);
    if (view_hidden_walls) {
        if (view_unsafe_walls) {
            return true;
        }
        if (none_bits(grid.info, CAVE_UNSAFE)) {
            return true;
        }
    }

    const auto &terrain = grid.get_terrain(TerrainKind::MIMIC);
    if (terrain.flags.has_not(TerrainCharacteristics::WALL) || terrain.flags.has(TerrainCharacteristics::HAS_GOLD)) {
        return true;
    }

    if (floor.contains(pos) && terrain.flags.has(TerrainCharacteristics::PERMANENT)) {
        return true;
    }

    const auto dirs = Direction::directions_8();
    const auto num_of_walls = std::count_if(dirs.begin(), dirs.end(),
        [&floor, &pos](const auto &d) {
            const auto pos_neighbor = pos + d.vec();
            if (!floor.contains(pos_neighbor)) {
                return true;
            }

            const auto &terrain_neighbor = floor.get_grid(pos_neighbor).get_terrain();
            return terrain_neighbor.flags.has(TerrainCharacteristics::WALL);
        });

    return num_of_walls != std::ssize(dirs);
}

/*!
 * @brief 指定した座標の地形の表示属性を取得する
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param pos 階の中の座標
 * @return シンボル表記
 * @todo 強力発動コピペの嵐…ポインタ引数の嵐……Fuuu^h^hck!!
 */
DisplaySymbolPair map_info(PlayerType *player_ptr, const Pos2D &pos)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &grid = floor.get_grid(pos);
    const auto &terrains = TerrainList::get_instance();
    const auto &world = AngbandWorld::get_instance();
    const auto is_wild_mode = world.is_wild_mode();
    const auto is_blind = player_ptr->effects()->blindness().is_blind();
    const auto has_nocto = player_ptr->see_nocto != 0;
    const auto is_darkened = !has_nocto && grid.is_darkened();
    const auto tag_unsafe = (view_unsafe_grids && (grid.info & CAVE_UNSAFE)) ? TerrainTag::UNDETECTED : TerrainTag::NONE;
    const auto *terrain_mimic_ptr = &grid.get_terrain(TerrainKind::MIMIC);
    DisplaySymbol symbol_config;
    if (terrain_mimic_ptr->flags.has_not(TerrainCharacteristics::REMEMBER)) {
        const auto is_visible = any_bits(grid.info, (CAVE_MARK | CAVE_LITE | CAVE_MNLT));
        const auto is_glowing = match_bits(grid.info, CAVE_GLOW | CAVE_MNDK, CAVE_GLOW);
        const auto can_view = grid.is_view() && (is_glowing || has_nocto);
        if (!is_blind && (is_visible || can_view)) {
            symbol_config = terrain_mimic_ptr->symbol_configs.at(F_LIT_STANDARD);
            if (is_wild_mode) {
                if (view_special_lite && !world.is_daytime()) {
                    symbol_config = terrain_mimic_ptr->symbol_configs.at(F_LIT_DARK);
                }
            } else if (is_darkened) {
                terrain_mimic_ptr = &terrains.get_terrain(tag_unsafe);
                symbol_config = terrain_mimic_ptr->symbol_configs.at(F_LIT_STANDARD);
            } else if (view_special_lite) {
                if (grid.info & (CAVE_LITE | CAVE_MNLT)) {
                    if (view_yellow_lite) {
                        symbol_config = terrain_mimic_ptr->symbol_configs.at(F_LIT_LITE);
                    }
                } else if ((grid.info & (CAVE_GLOW | CAVE_MNDK)) != CAVE_GLOW) {
                    symbol_config = terrain_mimic_ptr->symbol_configs.at(F_LIT_DARK);
                } else if (!(grid.info & CAVE_VIEW)) {
                    if (view_bright_lite) {
                        symbol_config = terrain_mimic_ptr->symbol_configs.at(F_LIT_DARK);
                    }
                }
            }
        } else {
            terrain_mimic_ptr = &terrains.get_terrain(tag_unsafe);
            symbol_config = terrain_mimic_ptr->symbol_configs.at(F_LIT_STANDARD);
        }
    } else {
        if (grid.is_mark() && is_revealed_wall(floor, pos)) {
            symbol_config = terrain_mimic_ptr->symbol_configs.at(F_LIT_STANDARD);
            if (is_wild_mode) {
                if (view_granite_lite && (is_blind || !world.is_daytime())) {
                    symbol_config = terrain_mimic_ptr->symbol_configs.at(F_LIT_DARK);
                }
            } else if (is_darkened && !is_blind) {
                if (terrain_mimic_ptr->flags.has_all_of({ TerrainCharacteristics::LOS, TerrainCharacteristics::PROJECTION })) {
                    terrain_mimic_ptr = &terrains.get_terrain(tag_unsafe);
                    symbol_config = terrain_mimic_ptr->symbol_configs.at(F_LIT_STANDARD);
                } else if (view_granite_lite && view_bright_lite) {
                    symbol_config = terrain_mimic_ptr->symbol_configs.at(F_LIT_DARK);
                }
            } else if (view_granite_lite) {
                if (is_blind) {
                    symbol_config = terrain_mimic_ptr->symbol_configs.at(F_LIT_DARK);
                } else if (grid.info & (CAVE_LITE | CAVE_MNLT)) {
                    if (view_yellow_lite) {
                        symbol_config = terrain_mimic_ptr->symbol_configs.at(F_LIT_LITE);
                    }
                } else if (view_bright_lite) {
                    if (!(grid.info & CAVE_VIEW)) {
                        symbol_config = terrain_mimic_ptr->symbol_configs.at(F_LIT_DARK);
                    } else if ((grid.info & (CAVE_GLOW | CAVE_MNDK)) != CAVE_GLOW) {
                        symbol_config = terrain_mimic_ptr->symbol_configs.at(F_LIT_DARK);
                    } else if (terrain_mimic_ptr->flags.has_not(TerrainCharacteristics::LOS) && !check_local_illumination(player_ptr, pos.y, pos.x)) {
                        symbol_config = terrain_mimic_ptr->symbol_configs.at(F_LIT_DARK);
                    }
                }
            }
        } else {
            terrain_mimic_ptr = &terrains.get_terrain(tag_unsafe);
            symbol_config = terrain_mimic_ptr->symbol_configs.at(F_LIT_STANDARD);
        }
    }

    if (feat_priority == -1) {
        feat_priority = terrain_mimic_ptr->priority;
    }

    DisplaySymbolPair symbol_pair(symbol_config, symbol_config);
    const auto is_hallucinated = player_ptr->effects()->hallucination().is_hallucinated();
    if (is_hallucinated && one_in_(256)) {
        symbol_pair.symbol_foreground = image_random();
    }

    for (const auto this_o_idx : grid.o_idx_list) {
        const auto &item = *floor.o_list[this_o_idx];
        if (item.marked.has_not(OmType::FOUND)) {
            continue;
        }

        if (display_autopick) {
            match_autopick = find_autopick_list(player_ptr, &item);
            if (match_autopick == -1) {
                continue;
            }

            const auto act = autopick_list[match_autopick].action;
            if ((act & DO_DISPLAY) && (act & display_autopick)) {
                autopick_obj = &item;
            } else {
                match_autopick = -1;
                continue;
            }
        }

        symbol_pair.symbol_foreground = item.get_symbol();
        feat_priority = 20;
        if (is_hallucinated) {
            symbol_pair.symbol_foreground = image_object();
        }

        break;
    }

    if (grid.has_monster() && display_autopick != 0) {
        symbol_pair.symbol_foreground = set_term_color(player_ptr, pos, symbol_pair.symbol_foreground);
        return symbol_pair;
    }

    const auto &monster = floor.m_list[grid.m_idx];
    if (!monster.ml) {
        symbol_pair.symbol_foreground = set_term_color(player_ptr, pos, symbol_pair.symbol_foreground);
        return symbol_pair;
    }

    const auto &monrace_ap = monster.get_appearance_monrace();
    feat_priority = 30;
    if (is_hallucinated) {
        if (!monrace_ap.visual_flags.has_all_of({ MonsterVisualType::CLEAR, MonsterVisualType::CLEAR_COLOR })) {
            symbol_pair.symbol_foreground = image_monster();
        }

        symbol_pair.symbol_foreground = set_term_color(player_ptr, pos, symbol_pair.symbol_foreground);
        return symbol_pair;
    }

    symbol_config = monrace_ap.symbol_config;
    if (monrace_ap.visual_flags.has_none_of({ MonsterVisualType::CLEAR, MonsterVisualType::SHAPECHANGER, MonsterVisualType::CLEAR_COLOR, MonsterVisualType::MULTI_COLOR, MonsterVisualType::RANDOM_COLOR })) {
        symbol_pair.symbol_foreground = set_term_color(player_ptr, pos, symbol_config);
        return symbol_pair;
    }

    if (monrace_ap.visual_flags.has_all_of({ MonsterVisualType::CLEAR, MonsterVisualType::CLEAR_COLOR })) {
        symbol_pair.symbol_foreground = set_term_color(player_ptr, pos, symbol_pair.symbol_foreground);
        return symbol_pair;
    }

    if (monrace_ap.visual_flags.has(MonsterVisualType::CLEAR_COLOR) && (symbol_pair.symbol_foreground.color != TERM_DARK) && !use_graphics) {
        /* Do nothing */
    } else if (monrace_ap.visual_flags.has(MonsterVisualType::MULTI_COLOR) && !use_graphics) {
        if (monrace_ap.visual_flags.has(MonsterVisualType::ANY_COLOR)) {
            symbol_pair.symbol_foreground.color = randnum1<uint8_t>(15);
        } else {
            constexpr static auto colors = {
                TERM_RED,
                TERM_L_RED,
                TERM_WHITE,
                TERM_L_GREEN,
                TERM_BLUE,
                TERM_L_DARK,
                TERM_GREEN,
            };

            symbol_pair.symbol_foreground.color = rand_choice(colors);
        }
    } else if (monrace_ap.visual_flags.has(MonsterVisualType::RANDOM_COLOR) && !use_graphics) {
        symbol_pair.symbol_foreground.color = grid.m_idx % 15 + 1;
    } else {
        symbol_pair.symbol_foreground.color = symbol_config.color;
    }

    if (monrace_ap.visual_flags.has(MonsterVisualType::CLEAR) && (symbol_pair.symbol_foreground.character != ' ') && !use_graphics) {
        symbol_pair.symbol_foreground = set_term_color(player_ptr, pos, symbol_pair.symbol_foreground);
        return symbol_pair;
    }

    if (monrace_ap.visual_flags.has(MonsterVisualType::SHAPECHANGER)) {
        if (use_graphics) {
            const auto &monraces = MonraceList::get_instance();
            const auto &monrace = monraces.pick_monrace_at_random();
            symbol_pair.symbol_foreground = monrace.symbol_config;
        } else {
            symbol_pair.symbol_foreground.character = one_in_(25) ? rand_choice(image_objects) : rand_choice(image_monsters);
        }

        symbol_pair.symbol_foreground = set_term_color(player_ptr, pos, symbol_pair.symbol_foreground);
        return symbol_pair;
    }

    symbol_pair.symbol_foreground = set_term_color(player_ptr, pos, { symbol_pair.symbol_foreground.color, symbol_config.character });
    return symbol_pair;
}

/*!
 * @brief 単色表示色を取得する
 *
 * 以下のゲームの状態にしたがってマップを単色表示する場合にその色を返す。
 * なお、タイル表示モードでは適用されない。
 *
 * - モンスターが時間停止スキルを使用中: TERM_DARK(マップが黒一色となりなにも表示されない)
 * - プレイヤーが無敵状態もしくは時間停止スキルを使用中: TERM_WHITE
 * - プレイヤーが幽体化スキルを使用中: TERM_L_DARK
 *
 * @return 単色表示色。単色表示を行わない場合はtl::nullopt
 */
tl::optional<uint8_t> get_monochrome_display_color(PlayerType *player_ptr)
{
    if (use_graphics) {
        return tl::nullopt;
    }

    if (AngbandWorld::get_instance().timewalk_m_idx) {
        return TERM_DARK;
    }
    if (is_invuln(player_ptr) || player_ptr->timewalk) {
        return TERM_WHITE;
    }
    if (player_ptr->wraith_form) {
        return TERM_L_DARK;
    }

    return tl::nullopt;
}
