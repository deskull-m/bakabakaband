#include "io/input-intent.h"
#include "util/finalizer.h"
#include <doctest/doctest.h>

TEST_CASE("Point intents are taken once and only for the same purpose")
{
    const auto restore = util::make_finalizer([] { clear_point_intent(); });
    clear_point_intent();
    CHECK_FALSE(take_point_intent(PointIntentPurpose::POINT));

    push_point_intent(PointIntentPurpose::TARGET, { 3, 4 });

    // 用途が違えば取り出さず、残しておく
    CHECK_FALSE(take_point_intent(PointIntentPurpose::POINT));

    const auto pos = take_point_intent(PointIntentPurpose::TARGET);
    REQUIRE(pos);
    CHECK(*pos == Pos2D(3, 4));

    // 一度取り出したら無くなる
    CHECK_FALSE(take_point_intent(PointIntentPurpose::TARGET));
}

TEST_CASE("A newer point intent replaces the older one")
{
    const auto restore = util::make_finalizer([] { clear_point_intent(); });
    push_point_intent(PointIntentPurpose::POINT, { 1, 1 });
    push_point_intent(PointIntentPurpose::POINT, { 2, 5 });
    const auto pos = take_point_intent(PointIntentPurpose::POINT);
    REQUIRE(pos);
    CHECK(*pos == Pos2D(2, 5));

    push_point_intent(PointIntentPurpose::POINT, { 7, 7 });
    clear_point_intent();
    CHECK_FALSE(take_point_intent(PointIntentPurpose::POINT));
}
