/*!
 * @file game-data-initializer.cpp
 * @brief 馬鹿馬鹿蛮怒のゲームデータ初期化定義
 */

#include "main/game-data-initializer.h"
#include "cmd-io/macro-util.h"
#include "dungeon/quest.h"
#include "floor/floor-util.h"
#include "game-option/option-flags.h"
#include "game-option/option-types-table.h"
#include "system/alloc-entries.h"
#include "system/baseitem-info.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/gameterm.h"
#include "util/angband-files.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include <algorithm>

/*!
 * @brief マクロ登録の最大数 / Maximum number of macros (see "io.c")
 * @note Default: assume at most 256 macros are used
 */
constexpr int MACRO_MAX = 256;

static void init_gf_colors()
{
    for (auto i = 0; i < enum2i(AttributeType::MAX); i++) {
        gf_colors.emplace(i2enum<AttributeType>(i), "");
    }
}

/*!
 * @brief その他の初期情報更新 /
 * Initialize some other arrays
 * @return エラーコード
 */
void init_other(PlayerType *player_ptr)
{
    player_ptr->current_floor_ptr = &floor_info; // TODO:本当はこんなところで初期化したくない
    auto *floor_ptr = player_ptr->current_floor_ptr;
    floor_ptr->o_list.assign(MAX_FLOOR_ITEMS, {});
    floor_ptr->m_list.assign(MAX_FLOOR_MONSTERS, {});

    max_dlv.assign(dungeons_info.size(), {});
    floor_ptr->grid_array.assign(MAX_HGT, std::vector<Grid>(MAX_WID));
    init_gf_colors();

    macro_patterns.assign(MACRO_MAX, {});
    macro_actions.assign(MACRO_MAX, {});
    macro_buffers.assign(FILE_READ_BUFF_SIZE, {});
    for (auto i = 0; option_info[i].o_desc; i++) {
        int os = option_info[i].o_set;
        int ob = option_info[i].o_bit;
        if (option_info[i].o_var == nullptr) {
            continue;
        }

        g_option_masks[os] |= (1UL << ob);
        if (option_info[i].o_norm) {
            set_bits(g_option_flags[os], 1U << ob);
        } else {
            reset_bits(g_option_flags[os], 1U << ob);
        }
    }

    for (auto &window_mask : g_window_masks) {
        for (auto i = 0; i < 32; i++) {
            if (window_flag_desc[i]) {
                window_mask.set(i2enum<SubWindowRedrawingFlag>(i));
            }
        }
    }

    g_window_flags[1].clear();
    g_window_flags[1].set(SubWindowRedrawingFlag::MESSAGE);
    g_window_flags[2].clear();
    g_window_flags[2].set(SubWindowRedrawingFlag::INVENTORY);
}

/*!
 * @brief モンスター生成テーブルを初期化する
 */
void init_monsters_alloc()
{
    auto &table = MonraceAllocationTable::get_instance();
    table.initialize();
}

/*!
 * @brief アイテム生成テーブルを初期化する
 */
void init_items_alloc()
{
    short num[MAX_DEPTH]{};
    auto alloc_kind_size = 0;
    const auto &baseitems = BaseitemList::get_instance();
    for (const auto &baseitem : baseitems) {
        for (const auto &[level, chance] : baseitem.alloc_tables) {
            if (chance != 0) {
                alloc_kind_size++;
                num[level]++;
            }
        }
    }

    for (auto i = 1; i < MAX_DEPTH; i++) {
        num[i] += num[i - 1];
    }

    if (num[0] == 0) {
        quit(_("町のアイテムがない！", "No town objects!"));
    }

    alloc_kind_table.assign(alloc_kind_size, {});
    short aux[MAX_DEPTH]{};
    for (const auto &baseitem : baseitems) {
        for (const auto &[level, chance] : baseitem.alloc_tables) {
            if (chance == 0) {
                continue;
            }

            const auto x = level;
            const short p = 100 / chance;
            const auto y = (x > 0) ? num[x - 1] : 0;
            const auto z = y + aux[x];
            alloc_kind_table[z].index = baseitem.idx;
            alloc_kind_table[z].level = x;
            alloc_kind_table[z].prob1 = p;
            alloc_kind_table[z].prob2 = p;
            aux[x]++;
        }
    }
}
