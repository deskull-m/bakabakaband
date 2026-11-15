#include "system/creature-entity.h"
#include "system/player-type-definition.h"
#include "util/finalizer.h"
#include "view/view-event.h"
#include <doctest/doctest.h>
#include <memory>
#include <vector>

namespace {
struct RecordingSink : public ViewEventSink {
    std::vector<ViewEvent> events;
    void on_view_event(const ViewEvent &event) override
    {
        this->events.push_back(event);
    }
};
}

TEST_CASE("View events are only delivered while a sink is registered")
{
    auto player_ptr = std::make_unique<PlayerType>();
    auto &player = *player_ptr;
    player.y = 3;
    player.x = 4;

    // 未登録なら何も起きない (落ちない)
    CHECK_FALSE(is_view_event_sink_active());
    notify_view_damage(player, 10);

    RecordingSink sink;
    set_view_event_sink(&sink);
    const auto restore = util::make_finalizer([] { set_view_event_sink(nullptr); });
    CHECK(is_view_event_sink_active());

    notify_view_damage(player, 10);
    notify_view_damage(player, 0);
    notify_view_death(player);
    REQUIRE(sink.events.size() == 2);
    CHECK(sink.events[0].type == ViewEventType::DAMAGE);
    CHECK(sink.events[0].value == 10);
    CHECK(sink.events[0].actor.is_player);
    CHECK(sink.events[0].actor.has_pos);
    CHECK(sink.events[0].actor.pos == Pos2D(3, 4));
    CHECK(sink.events[0].actor.id == player.get_display_id());
    CHECK(sink.events[1].type == ViewEventType::DEATH);
}

TEST_CASE("View events hide creatures the player cannot see")
{
    auto player_ptr = std::make_unique<PlayerType>();
    auto &player = *player_ptr;
    CreatureEntity monster;
    monster.init_monster_profile();
    monster.y = 5;
    monster.x = 6;
    monster.set_visible_on_map(false);

    RecordingSink sink;
    set_view_event_sink(&sink);
    const auto restore = util::make_finalizer([] { set_view_event_sink(nullptr); });

    // 見えないモンスターの被弾・死亡は知らせない
    notify_view_damage(monster, 5);
    notify_view_death(monster);
    CHECK(sink.events.empty());

    // プレイヤーが見えないモンスターを殴った: 起こした側だけ分かり、相手の識別番号と座標は伏せる
    notify_view_melee(player, monster, true);
    REQUIRE(sink.events.size() == 1);
    CHECK(sink.events[0].type == ViewEventType::MELEE);
    CHECK(sink.events[0].hit);
    CHECK(sink.events[0].actor.is_player);
    CHECK(sink.events[0].target.id == 0);
    CHECK_FALSE(sink.events[0].target.has_pos);

    // 見えるようになれば分かる
    monster.set_visible_on_map(true);
    notify_view_damage(monster, 5);
    REQUIRE(sink.events.size() == 2);
    CHECK(sink.events[1].actor.id == monster.get_display_id());
    CHECK(sink.events[1].actor.pos == Pos2D(5, 6));
}
