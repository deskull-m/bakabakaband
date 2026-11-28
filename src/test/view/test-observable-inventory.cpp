#include "view/display-messages.h"
#include "view/observable-inventory.h"
#include <doctest/doctest.h>

TEST_CASE("Recent messages are returned oldest first and merge repeats")
{
    message_add("observable-inventory test: first");
    message_add("observable-inventory test: second");
    message_add("observable-inventory test: second");

    const auto messages = observe_recent_messages(2);
    REQUIRE(messages.size() == 2);
    CHECK(messages[0] == "observable-inventory test: first");
    CHECK(messages[1] == "observable-inventory test: second <x2>");

    // 件数を超えて求めても、ある分だけ返す
    CHECK(observe_recent_messages(1 << 20).size() == static_cast<size_t>(message_num()));
    CHECK(observe_recent_messages(0).empty());
    CHECK(observe_recent_messages(-1).empty());
}
