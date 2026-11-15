/*!
 * @file godot-game-events.cpp
 * @brief 表示用のゲームイベントの Godot 側へのブリッジ実装
 */

#include "main-godot/godot-game-events.h"

#include <atomic>
#include <deque>
#include <mutex>

namespace {

constexpr size_t MAX_QUEUED_EVENTS = 1024;

std::atomic<bool> g_enabled{ false };
std::mutex g_mutex;
std::deque<ViewEvent> g_queue; ///< g_mutex で保護

class GameEventQueueSink : public ViewEventSink {
public:
    void on_view_event(const ViewEvent &event) override
    {
        game_events_push(event);
    }
};

GameEventQueueSink g_sink;

} // namespace

void game_events_install_sink()
{
    set_view_event_sink(&g_sink);
}

void game_events_set_enabled(bool enabled)
{
    g_enabled.store(enabled);
    if (!enabled) {
        std::lock_guard lock(g_mutex);
        g_queue.clear();
    }
}

bool game_events_is_enabled()
{
    return g_enabled.load();
}

void game_events_push(const ViewEvent &event)
{
    if (!g_enabled.load(std::memory_order_relaxed)) {
        return;
    }

    std::lock_guard lock(g_mutex);
    if (g_queue.size() >= MAX_QUEUED_EVENTS) {
        g_queue.pop_front();
    }
    g_queue.push_back(event);
}

std::vector<ViewEvent> game_events_drain()
{
    std::lock_guard lock(g_mutex);
    std::vector<ViewEvent> events(std::make_move_iterator(g_queue.begin()), std::make_move_iterator(g_queue.end()));
    g_queue.clear();
    return events;
}
