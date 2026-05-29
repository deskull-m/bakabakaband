#include "load/birth-loader.h"
#include "avatar/avatar.h"
#include "birth/quick-start.h"
#include "load/load-util.h"
#include "player-ability/player-ability-types.h"
#include "system/angband.h"
#include "system/system-variables.h"

/*!
 * @brief クイックスタート情報を読み込む / Load quick start data
 */
void load_quick_start(void)
{
    previous_char.psex = i2enum<player_sex>(rd_byte());
    previous_char.prace = i2enum<PlayerRaceType>(rd_byte());
    previous_char.pclass = i2enum<PlayerClassType>(rd_byte());
    previous_char.ppersonality = i2enum<player_personality_type>(rd_byte());
    previous_char.realm1 = rd_byte();
    previous_char.realm2 = rd_byte();

    previous_char.death_count = rd_s32b();
    previous_char.age = rd_s16b();
    previous_char.ht = rd_s16b();
    previous_char.wt = rd_s16b();
    previous_char.prestige = rd_s16b();
    previous_char.au = rd_s32b();

    for (int i = 0; i < A_MAX; i++) {
        previous_char.stat_max[i] = rd_s16b();
    }
    for (int i = 0; i < A_MAX; i++) {
        previous_char.stat_max_max[i] = rd_s16b();
    }

    // [セーブ ver49] 最大プレイヤーレベル拡張 (50→60) に伴い、クイックスタートの
    // player_hp 配列は件数を保持せず PY_MAX_LEVEL 件保存される。旧セーブ (<49) は
    // 50 件で保存されているため、その件数だけ読み込む (残りは 0 のまま)。
    constexpr int OLD_PY_MAX_LEVEL = 50;
    const int hp_count = loading_savefile_version_is_older_than(49) ? OLD_PY_MAX_LEVEL : PY_MAX_LEVEL;
    for (int i = 0; i < hp_count; i++) {
        previous_char.player_hp[i] = rd_s16b();
    }

    previous_char.patron = rd_s16b();

    // Load previous character virtues (only types, values not saved)
    previous_char.virtues.clear();
    for (int i = 0; i < 8; i++) {
        auto vir_type = i2enum<Virtue>(rd_s16b());
        if (vir_type != Virtue::NONE && vir_type < Virtue::MAX) {
            previous_char.virtues[vir_type] = 0;
        }
    }

    for (int i = 0; i < 4; i++) {
        const auto history = rd_string();
        const auto len = history.copy(previous_char.history[i], sizeof(previous_char.history[i]) - 1);
        previous_char.history[i][len] = '\0';
    }

    strip_bytes(1);
    previous_char.quick_ok = rd_byte() != 0;
}
