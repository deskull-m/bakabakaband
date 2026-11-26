/*!
 * @brief 死亡からの復活によるゲーム内時刻の経過のテスト
 */

#include "system/gamevalue.h"
#include "system/inner-game-data.h"
#include "world/world.h"
#include <doctest/doctest.h>

namespace {
constexpr GAME_TURN A_DAY = TURNS_PER_TICK * TOWN_DAWN;

/*!
 * @brief テスト中だけゲーム内時刻を差し替え、抜けるときに元へ戻す
 */
class ScopedGameTurn {
public:
    explicit ScopedGameTurn(GAME_TURN turn)
        : saved_turn(AngbandWorld::get_instance().game_turn)
    {
        InnerGameData::get_instance().init_turn_limit();
        AngbandWorld::get_instance().game_turn = turn;
    }

    ~ScopedGameTurn()
    {
        AngbandWorld::get_instance().game_turn = this->saved_turn;
    }

    ScopedGameTurn(const ScopedGameTurn &) = delete;
    ScopedGameTurn &operator=(const ScopedGameTurn &) = delete;

private:
    GAME_TURN saved_turn;
};
}

TEST_CASE("get_start_turn_in_day: アンデッドは夜、それ以外は朝から始まる")
{
    auto &world = AngbandWorld::get_instance();
    const ScopedGameTurn scoped(AngbandWorld::get_start_turn_in_day(true));
    CHECK_FALSE(world.is_daytime());

    world.game_turn = AngbandWorld::get_start_turn_in_day(false);
    CHECK(world.is_daytime());
}

TEST_CASE("pass_game_turn_by_death: 3 日後のゲーム開始時刻まで進む")
{
    auto &world = AngbandWorld::get_instance();
    for (const auto is_undead : { false, true }) {
        CAPTURE(is_undead);
        const auto start = AngbandWorld::get_start_turn_in_day(is_undead);

        // 開始時刻ちょうど・その日の途中・翌日の開始時刻の直前のいずれでも、その日を基準に 3 日進む
        for (const auto offset : { GAME_TURN{ 0 }, A_DAY / 3, A_DAY - 1 }) {
            CAPTURE(offset);
            const ScopedGameTurn scoped(start + 5 * A_DAY + offset);
            world.pass_game_turn_by_death(is_undead);
            CHECK(world.game_turn == start + 8 * A_DAY);
            CHECK(world.is_daytime() == !is_undead);
        }
    }
}

TEST_CASE("pass_game_turn_by_death: 開始時刻より前のターンでも過去へは戻らない")
{
    // 人間で始めて夜明け前に吸血鬼へ変身した場合など
    auto &world = AngbandWorld::get_instance();
    const auto undead_start = AngbandWorld::get_start_turn_in_day(true);
    const ScopedGameTurn scoped(undead_start - 1);
    world.pass_game_turn_by_death(true);
    CHECK(world.game_turn == undead_start + 2 * A_DAY);
    CHECK(world.game_turn > undead_start - 1);
    CHECK_FALSE(world.is_daytime());
}
