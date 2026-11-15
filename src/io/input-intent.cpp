#include "io/input-intent.h"
#include <chrono>
#include <mutex>

namespace {
/// 積んでから取り出されるまでの猶予 (キーの処理待ちを含めて十分な長さ)
constexpr auto INTENT_LIFETIME = std::chrono::seconds(2);

struct PendingPointIntent {
    PointIntentPurpose purpose;
    Pos2D pos;
    std::chrono::steady_clock::time_point pushed_at;
};

std::mutex intent_mutex;
tl::optional<PendingPointIntent> pending_intent;
}

void push_point_intent(PointIntentPurpose purpose, const Pos2D &pos)
{
    std::lock_guard lock(intent_mutex);
    pending_intent = PendingPointIntent{ purpose, pos, std::chrono::steady_clock::now() };
}

tl::optional<Pos2D> take_point_intent(PointIntentPurpose purpose)
{
    std::lock_guard lock(intent_mutex);
    if (!pending_intent) {
        return tl::nullopt;
    }

    if (std::chrono::steady_clock::now() - pending_intent->pushed_at > INTENT_LIFETIME) {
        pending_intent = tl::nullopt;
        return tl::nullopt;
    }

    if (pending_intent->purpose != purpose) {
        return tl::nullopt;
    }

    const auto pos = pending_intent->pos;
    pending_intent = tl::nullopt;
    return pos;
}

void clear_point_intent()
{
    std::lock_guard lock(intent_mutex);
    pending_intent = tl::nullopt;
}
