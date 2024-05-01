﻿#include "load/option-loader.h"
#include "cmd-io/cmd-gameoption.h"
#include "game-option/cheat-options.h"
#include "game-option/option-flags.h"
#include "game-option/special-options.h"
#include "load/load-util.h"
#include "load/load-zangband.h"
#include "system/angband.h"
#include "util/bit-flags-calculator.h"
#include "world/world.h"

/*!
 * @brief ゲームオプションを読み込む / Read options (ignore most pre-2.8.0 options)
 * @details
 * Note that the normal options are now stored as a set of 256 bit flags,
 * plus a set of 256 bit masks to indicate which bit flags were defined
 * at the time the savefile was created.  This will allow new options
 * to be added, and old options to be removed, at any time, without
 * hurting old savefiles.
 *
 * The window options are stored in the same way, but note that each
 * window gets 32 options, and their order is fixed by certain defines.
 */
void rd_options()
{
    strip_bytes(16);

    if (loading_savefile_version_is_older_than(12)) {
        auto b = rd_byte();
        delay_factor = b * b * b;
    } else {
        delay_factor = rd_s32b();
    }

    hitpoint_warn = rd_byte();
    mana_warn = rd_byte();

    auto c = rd_u16b();

    if (c & 0x0002) {
        w_ptr->wizard = true;
    }

    cheat_peek = any_bits(c, 0x0100);
    cheat_hear = any_bits(c, 0x0200);
    cheat_room = any_bits(c, 0x0400);
    cheat_xtra = any_bits(c, 0x0800);
    cheat_know = any_bits(c, 0x1000);
    cheat_live = any_bits(c, 0x2000);
    cheat_save = any_bits(c, 0x4000);
    cheat_diary_output = any_bits(c, 0x8000);
    cheat_turn = any_bits(c, 0x0080);
    cheat_sight = any_bits(c, 0x0040);
    cheat_immortal = any_bits(c, 0x0020);

    autosave_l = rd_bool();
    autosave_t = rd_bool();
    autosave_freq = rd_s16b();

    uint32_t flag[8]{};
    for (auto n = 0; n < MAX_WINDOW_ENTITIES; n++) {
        flag[n] = rd_u32b();
    }

    uint32_t mask[8]{};
    for (auto n = 0; n < MAX_WINDOW_ENTITIES; n++) {
        mask[n] = rd_u32b();
    }

    for (auto n = 0; n < MAX_WINDOW_ENTITIES; n++) {
        for (auto i = 0; i < 32; i++) {
            if (none_bits(mask[n], 1U << i) || none_bits(g_option_masks[n], 1U << i)) {
                continue;
            }

            auto &option_flag = g_option_flags[n];
            if (flag[n] & (1UL << i)) {
                option_flag |= (1UL << i);
            } else {
                option_flag &= ~(1UL << i);
            }
        }
    }

    extract_option_vars();
    for (auto n = 0; n < MAX_WINDOW_ENTITIES; n++) {
        flag[n] = rd_u32b();
    }

    for (auto n = 0; n < MAX_WINDOW_ENTITIES; n++) {
        mask[n] = rd_u32b();
    }

    for (auto n = 0; n < MAX_WINDOW_ENTITIES; n++) {
        for (int i = 0; i < 32; i++) {
            if (none_bits(mask[n], 1U << i) || none_bits(g_window_masks[n], 1U << i)) {
                continue;
            }

            auto &window_flag = g_window_flags[n];
            if (flag[n] & (1UL << i)) {
                window_flag.set(i2enum<SubWindowRedrawingFlag>(i));
            } else {
                window_flag.reset(i2enum<SubWindowRedrawingFlag>(i));
            }
        }
    }
}
