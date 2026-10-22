/*!
 * @brief SpellHex のテスト
 */

#include "player-base/player-class.h"
#include "realm/realm-types.h"
#include "spell-realm/spells-hex.h"
#include "system/player-type-definition.h"
#include "system/deferred-update-requests.h"
#include "util/finalizer.h"
#include <doctest/doctest.h>

namespace {

/*!
 * @brief フラグを元の状態に戻す
 * @param dur 更新フラグ
 * @param flag 戻すフラグ
 * @param was_set 元の状態でフラグが立っていたか
 */
template <typename Flag>
void restore_flag(DeferredUpdateRequests &dur, Flag flag, bool was_set)
{
    if (was_set) {
        dur.set_flag(flag);
    } else {
        dur.reset_flag(flag);
    }
}

}

TEST_CASE("SpellHex::request_casting_change_updates requests recalculating the player status and redrawing the extra status, the HP and the MP")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    const auto restore = util::make_finalizer([&dur,
                                                  bonus = dur.has(StatusRecalculatingFlag::BONUS),
                                                  recalc_hp = dur.has(StatusRecalculatingFlag::HP),
                                                  recalc_mp = dur.has(StatusRecalculatingFlag::MP),
                                                  spells = dur.has(StatusRecalculatingFlag::SPELLS),
                                                  extra = dur.has(MainWindowRedrawingFlag::EXTRA),
                                                  draw_hp = dur.has(MainWindowRedrawingFlag::HP),
                                                  draw_mp = dur.has(MainWindowRedrawingFlag::MP)] {
        restore_flag(dur, StatusRecalculatingFlag::BONUS, bonus);
        restore_flag(dur, StatusRecalculatingFlag::HP, recalc_hp);
        restore_flag(dur, StatusRecalculatingFlag::MP, recalc_mp);
        restore_flag(dur, StatusRecalculatingFlag::SPELLS, spells);
        restore_flag(dur, MainWindowRedrawingFlag::EXTRA, extra);
        restore_flag(dur, MainWindowRedrawingFlag::HP, draw_hp);
        restore_flag(dur, MainWindowRedrawingFlag::MP, draw_mp);
    });

    dur.reset_flag(StatusRecalculatingFlag::BONUS);
    dur.reset_flag(StatusRecalculatingFlag::HP);
    dur.reset_flag(StatusRecalculatingFlag::MP);
    dur.reset_flag(StatusRecalculatingFlag::SPELLS);
    dur.reset_flag(MainWindowRedrawingFlag::EXTRA);
    dur.reset_flag(MainWindowRedrawingFlag::HP);
    dur.reset_flag(MainWindowRedrawingFlag::MP);

    SpellHex::request_casting_change_updates();

    CHECK(dur.has(StatusRecalculatingFlag::BONUS));
    CHECK(dur.has(StatusRecalculatingFlag::HP));
    CHECK(dur.has(StatusRecalculatingFlag::MP));
    CHECK(dur.has(StatusRecalculatingFlag::SPELLS));
    CHECK(dur.has(MainWindowRedrawingFlag::EXTRA));
    CHECK(dur.has(MainWindowRedrawingFlag::HP));
    CHECK(dur.has(MainWindowRedrawingFlag::MP));
}

TEST_CASE("SpellHex revenge setters request redrawing the status bar only when the shown revenge changes")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    const auto restore = util::make_finalizer([&dur, timed_effect = dur.has(MainWindowRedrawingFlag::TIMED_EFFECT)] {
        restore_flag(dur, MainWindowRedrawingFlag::TIMED_EFFECT, timed_effect);
    });

    PlayerType player;
    player.set_pclass(PlayerClassType::HIGH_MAGE);
    player.set_realm1(RealmType::HEX);
    CreatureClass(player).init_specific_data();
    SpellHex spell_hex(player);

    SUBCASE("setting the type without remaining turns does not change the display")
    {
        dur.reset_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
        spell_hex.set_revenge_type(SpellHexRevengeType::PATIENCE);
        CHECK_FALSE(dur.has(MainWindowRedrawingFlag::TIMED_EFFECT));
    }

    SUBCASE("starting, counting down and ending the revenge")
    {
        spell_hex.set_revenge_type(SpellHexRevengeType::PATIENCE);
        dur.reset_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
        spell_hex.set_revenge_turn(2, true);
        CHECK(dur.has(MainWindowRedrawingFlag::TIMED_EFFECT));

        dur.reset_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
        spell_hex.set_revenge_turn(1, false);
        CHECK_FALSE(dur.has(MainWindowRedrawingFlag::TIMED_EFFECT));

        spell_hex.set_revenge_turn(1, false);
        CHECK(dur.has(MainWindowRedrawingFlag::TIMED_EFFECT));
    }

    SUBCASE("changing the type while it is shown")
    {
        spell_hex.set_revenge_type(SpellHexRevengeType::PATIENCE);
        spell_hex.set_revenge_turn(2, true);
        dur.reset_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
        spell_hex.set_revenge_type(SpellHexRevengeType::REVENGE);
        CHECK(dur.has(MainWindowRedrawingFlag::TIMED_EFFECT));
    }

    SUBCASE("clearing the revenge")
    {
        dur.reset_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
        spell_hex.clear_revenge();
        CHECK_FALSE(dur.has(MainWindowRedrawingFlag::TIMED_EFFECT));

        spell_hex.set_revenge_type(SpellHexRevengeType::REVENGE);
        spell_hex.set_revenge_turn(2, true);
        dur.reset_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
        spell_hex.clear_revenge();
        CHECK(dur.has(MainWindowRedrawingFlag::TIMED_EFFECT));
        CHECK(spell_hex.get_revenge_turn() == 0);
        CHECK(spell_hex.get_revenge_type() == SpellHexRevengeType::NONE);
    }
}
