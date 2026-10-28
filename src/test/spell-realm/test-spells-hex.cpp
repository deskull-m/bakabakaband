/*!
 * @brief SpellHex のテスト
 */

#include "player-base/player-class.h"
#include "realm/realm-types.h"
#include "spell-realm/spells-hex.h"
#include "system/player-type-definition.h"
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

TEST_CASE("SpellHex revenge setters request redrawing the status bar only when the shown revenge changes")
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    const auto restore = util::make_finalizer([&rfu, timed_effect = rfu.has(MainWindowRedrawingFlag::TIMED_EFFECT)] {
        restore_flag(rfu, MainWindowRedrawingFlag::TIMED_EFFECT, timed_effect);
    });

    PlayerType player;
    player.set_pclass(PlayerClassType::HIGH_MAGE);
    player.set_realm1(RealmType::HEX);
    CreatureClass(player).init_specific_data();
    SpellHex spell_hex(player);

    SUBCASE("setting the type without remaining turns does not change the display")
    {
        rfu.reset_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
        spell_hex.set_revenge_type(SpellHexRevengeType::PATIENCE);
        CHECK_FALSE(rfu.has(MainWindowRedrawingFlag::TIMED_EFFECT));
    }

    SUBCASE("starting, counting down and ending the revenge")
    {
        spell_hex.set_revenge_type(SpellHexRevengeType::PATIENCE);
        rfu.reset_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
        spell_hex.set_revenge_turn(2, true);
        CHECK(rfu.has(MainWindowRedrawingFlag::TIMED_EFFECT));

        rfu.reset_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
        spell_hex.set_revenge_turn(1, false);
        CHECK_FALSE(rfu.has(MainWindowRedrawingFlag::TIMED_EFFECT));

        spell_hex.set_revenge_turn(1, false);
        CHECK(rfu.has(MainWindowRedrawingFlag::TIMED_EFFECT));
    }

    SUBCASE("changing the type while it is shown")
    {
        spell_hex.set_revenge_type(SpellHexRevengeType::PATIENCE);
        spell_hex.set_revenge_turn(2, true);
        rfu.reset_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
        spell_hex.set_revenge_type(SpellHexRevengeType::REVENGE);
        CHECK(rfu.has(MainWindowRedrawingFlag::TIMED_EFFECT));
    }

    SUBCASE("clearing the revenge")
    {
        rfu.reset_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
        spell_hex.clear_revenge();
        CHECK_FALSE(rfu.has(MainWindowRedrawingFlag::TIMED_EFFECT));

        spell_hex.set_revenge_type(SpellHexRevengeType::REVENGE);
        spell_hex.set_revenge_turn(2, true);
        rfu.reset_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
        spell_hex.clear_revenge();
        CHECK(rfu.has(MainWindowRedrawingFlag::TIMED_EFFECT));
        CHECK(spell_hex.get_revenge_turn() == 0);
        CHECK(spell_hex.get_revenge_type() == SpellHexRevengeType::NONE);
    }
}
