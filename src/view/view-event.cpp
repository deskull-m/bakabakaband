#include "view/view-event.h"
#include "system/creature-entity.h"

namespace {
ViewEventSink *view_event_sink = nullptr;

/// プレイヤーから見えるか (プレイヤー自身は常に見える)
bool is_perceived(const CreatureEntity &creature)
{
    return creature.is_player() || creature.is_visible_on_map();
}
}

void set_view_event_sink(ViewEventSink *sink)
{
    view_event_sink = sink;
}

bool is_view_event_sink_active()
{
    return view_event_sink != nullptr;
}

void notify_view_event(const ViewEvent &event)
{
    if (view_event_sink != nullptr) {
        view_event_sink->on_view_event(event);
    }
}

ViewEventActor make_view_event_actor(const CreatureEntity &creature)
{
    ViewEventActor actor;
    actor.is_player = creature.is_player();
    if (!is_perceived(creature)) {
        return actor;
    }

    actor.id = creature.get_display_id();
    actor.pos = creature.get_position();
    actor.has_pos = true;
    return actor;
}

void notify_view_melee(const CreatureEntity &attacker, const CreatureEntity &defender, bool hit, int method)
{
    if (!is_view_event_sink_active() || (!is_perceived(attacker) && !is_perceived(defender))) {
        return;
    }

    ViewEvent event;
    event.type = ViewEventType::MELEE;
    event.actor = make_view_event_actor(attacker);
    event.target = make_view_event_actor(defender);
    event.hit = hit;
    event.method = method;
    notify_view_event(event);
}

void notify_view_damage(const CreatureEntity &victim, int damage)
{
    if (!is_view_event_sink_active() || (damage <= 0) || !is_perceived(victim)) {
        return;
    }

    ViewEvent event;
    event.type = ViewEventType::DAMAGE;
    event.actor = make_view_event_actor(victim);
    event.value = damage;
    notify_view_event(event);
}

void notify_view_death(const CreatureEntity &victim)
{
    if (!is_view_event_sink_active() || !is_perceived(victim)) {
        return;
    }

    ViewEvent event;
    event.type = ViewEventType::DEATH;
    event.actor = make_view_event_actor(victim);
    notify_view_event(event);
}
