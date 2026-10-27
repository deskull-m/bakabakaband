/*!
 * @brief SpellHex のテスト
 */

#include "spell-realm/spells-hex.h"
#include "system/redrawing-flags-updater.h"
#include "util/finalizer.h"
#include <doctest/doctest.h>

namespace {

/*!
 * @brief フラグを元の状態に戻す
 * @param rfu 更新フラグ
 * @param flag 戻すフラグ
 * @param was_set 元の状態でフラグが立っていたか
 */
template <typename Flag>
void restore_flag(RedrawingFlagsUpdater &rfu, Flag flag, bool was_set)
{
    if (was_set) {
        rfu.set_flag(flag);
    } else {
        rfu.reset_flag(flag);
    }
}

}

TEST_CASE("SpellHex::request_casting_change_updates requests recalculating the player status and redrawing the extra status, the HP and the MP")
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    const auto restore = util::make_finalizer([&rfu,
                                                  bonus = rfu.has(StatusRecalculatingFlag::BONUS),
                                                  recalc_hp = rfu.has(StatusRecalculatingFlag::HP),
                                                  recalc_mp = rfu.has(StatusRecalculatingFlag::MP),
                                                  spells = rfu.has(StatusRecalculatingFlag::SPELLS),
                                                  extra = rfu.has(MainWindowRedrawingFlag::EXTRA),
                                                  draw_hp = rfu.has(MainWindowRedrawingFlag::HP),
                                                  draw_mp = rfu.has(MainWindowRedrawingFlag::MP)] {
        restore_flag(rfu, StatusRecalculatingFlag::BONUS, bonus);
        restore_flag(rfu, StatusRecalculatingFlag::HP, recalc_hp);
        restore_flag(rfu, StatusRecalculatingFlag::MP, recalc_mp);
        restore_flag(rfu, StatusRecalculatingFlag::SPELLS, spells);
        restore_flag(rfu, MainWindowRedrawingFlag::EXTRA, extra);
        restore_flag(rfu, MainWindowRedrawingFlag::HP, draw_hp);
        restore_flag(rfu, MainWindowRedrawingFlag::MP, draw_mp);
    });

    rfu.reset_flag(StatusRecalculatingFlag::BONUS);
    rfu.reset_flag(StatusRecalculatingFlag::HP);
    rfu.reset_flag(StatusRecalculatingFlag::MP);
    rfu.reset_flag(StatusRecalculatingFlag::SPELLS);
    rfu.reset_flag(MainWindowRedrawingFlag::EXTRA);
    rfu.reset_flag(MainWindowRedrawingFlag::HP);
    rfu.reset_flag(MainWindowRedrawingFlag::MP);

    SpellHex::request_casting_change_updates();

    CHECK(rfu.has(StatusRecalculatingFlag::BONUS));
    CHECK(rfu.has(StatusRecalculatingFlag::HP));
    CHECK(rfu.has(StatusRecalculatingFlag::MP));
    CHECK(rfu.has(StatusRecalculatingFlag::SPELLS));
    CHECK(rfu.has(MainWindowRedrawingFlag::EXTRA));
    CHECK(rfu.has(MainWindowRedrawingFlag::HP));
    CHECK(rfu.has(MainWindowRedrawingFlag::MP));
}
