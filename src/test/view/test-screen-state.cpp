#include "io/input-context.h"
#include "term/screen-processor.h"
#include "term/z-term.h"
#include "view/screen-state.h"
#include <doctest/doctest.h>

namespace {
ScreenStateInputs make_inputs(int icky_depth, tl::optional<InputContext> context, bool cleared = false)
{
    ScreenStateInputs inputs;
    inputs.has_dungeon = true;
    inputs.icky_depth = icky_depth;
    inputs.cleared_since_save = cleared;
    inputs.input_context = std::move(context);
    return inputs;
}

InputContext context_of(InputKind kind, InputDetails details = std::monostate{})
{
    return { kind, "", std::move(details) };
}
}

TEST_CASE("Screen state is NO_GAME before a floor exists")
{
    ScreenStateInputs inputs;
    inputs.input_context = context_of(InputKind::BIRTH, BirthInputDetails{ BirthStep::RACE });
    const auto info = classify_screen_state(inputs);
    CHECK(info.state == ScreenState::NO_GAME);
    REQUIRE(info.input_kind);
    CHECK(*info.input_kind == InputKind::BIRTH);
}

TEST_CASE("Screen state on the map")
{
    CHECK(classify_screen_state(make_inputs(0, tl::nullopt)).state == ScreenState::MAP);
    CHECK(classify_screen_state(make_inputs(0, context_of(InputKind::COMMAND, CommandInputDetails{ false }))).state == ScreenState::MAP);

    for (const auto kind : { InputKind::MORE, InputKind::DIRECTION, InputKind::CONFIRM, InputKind::QUANTITY, InputKind::STRING, InputKind::ITEM }) {
        CHECK(classify_screen_state(make_inputs(0, context_of(kind))).state == ScreenState::MAP_WITH_PROMPT);
    }
}

TEST_CASE("Targeting exposes the cursor position even over a saved screen")
{
    const Pos2D pos{ 7, 11 };
    const TargetInputDetails details{ .look = true, .free_mode = false, .can_target = false, .pos = pos };
    for (const auto depth : { 0, 1 }) {
        const auto info = classify_screen_state(make_inputs(depth, context_of(InputKind::TARGET, details)));
        CHECK(info.state == ScreenState::TARGETING);
        REQUIRE(info.cursor_pos);
        CHECK(*info.cursor_pos == pos);
    }

    const auto point = classify_screen_state(make_inputs(0, context_of(InputKind::POINT, PointInputDetails{ { 3, 4 } })));
    CHECK(point.state == ScreenState::TARGETING);
    REQUIRE(point.cursor_pos);
    CHECK(*point.cursor_pos == Pos2D(3, 4));
}

TEST_CASE("Saved screens are menus unless cleared or full-screen by kind")
{
    // 持ち物の一覧を重ねている
    CHECK(classify_screen_state(make_inputs(1, context_of(InputKind::ITEM))).state == ScreenState::OVERLAY_MENU);
    CHECK(classify_screen_state(make_inputs(1, tl::nullopt)).state == ScreenState::OVERLAY_MENU);

    // 開いてから画面全体を消した (キャラクター情報など)
    CHECK(classify_screen_state(make_inputs(1, context_of(InputKind::ANY_KEY), true)).state == ScreenState::FULLSCREEN_TEXT);

    // 入力の種類だけで全画面と分かるもの
    for (const auto kind : { InputKind::FILE_VIEW, InputKind::BROWSER, InputKind::OPTION, InputKind::AUTOPICK_EDITOR, InputKind::BUILDING }) {
        CHECK(classify_screen_state(make_inputs(1, context_of(kind))).state == ScreenState::FULLSCREEN_TEXT);
    }
    CHECK(classify_screen_state(make_inputs(0, context_of(InputKind::COMMAND, CommandInputDetails{ true }))).state == ScreenState::FULLSCREEN_TEXT);
}

TEST_CASE("The overlay region encloses the cells that differ from the saved map")
{
    const auto base = term_win::create(10, 6);
    auto current = base->clone();

    // 同じ画面なら重ねた部分は無い
    CHECK_FALSE(calc_screen_overlay_region(*base, *current, 1));

    // 一覧を重ねた範囲 (文字か色が変わったセル) を囲む
    current->c[2][3] = 'a';
    current->a[4][6] = 5;
    const auto region = calc_screen_overlay_region(*base, *current, 1);
    REQUIRE(region);
    CHECK(*region == ScreenRegion{ 3, 2, 4, 3 });

    // first_row より上 (メッセージの行) の変化は数えない
    auto message_only = base->clone();
    message_only->c[0][0] = 'x';
    CHECK_FALSE(calc_screen_overlay_region(*base, *message_only, 1));
    CHECK(calc_screen_overlay_region(*base, *message_only, 0) == ScreenRegion{ 0, 0, 1, 1 });

    // 大きさが違う画面とは比べない
    const auto resized = term_win::create(12, 6);
    CHECK_FALSE(calc_screen_overlay_region(*base, *resized, 1));
}
