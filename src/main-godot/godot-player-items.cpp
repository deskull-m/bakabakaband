/*!
 * @file godot-player-items.cpp
 * @brief 持ち物・装備・メッセージの Godot 側へのブリッジ実装
 */

#include "main-godot/godot-player-items.h"

#include "locale/character-encoding.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"
#include "world/world.h"

#include <atomic>
#include <mutex>

namespace {

std::atomic<bool> g_enabled{ false };
std::atomic<uint64_t> g_inventory_version{ 0 };
std::atomic<uint64_t> g_message_version{ 0 };
std::mutex g_mutex;
GodotInventorySnapshot g_inventory; ///< g_mutex で保護
GodotMessageSnapshot g_messages; ///< g_mutex で保護

/// ゲームスレッド専用: 前回公開した内容 (内部の文字コード)
ObservedInventory g_last_inventory;
bool g_has_last_inventory = false;
int32_t g_last_message_count = -1;
std::string g_last_message;

std::string to_utf8(const std::string &text)
{
#ifdef JP
    const auto utf8 = sys_to_utf8(text);
    return utf8 ? *utf8 : std::string();
#else
    return text;
#endif
}

ObservedItem to_utf8(ObservedItem item)
{
    item.name = to_utf8(item.name);
    item.weight_text = to_utf8(item.weight_text);
    return item;
}

ObservedInventory to_utf8(const ObservedInventory &source)
{
    auto converted = source;
    for (auto &item : converted.pack) {
        item = to_utf8(item);
    }
    for (auto &slot : converted.equipment) {
        slot.slot_name = to_utf8(slot.slot_name);
        if (slot.has_item) {
            slot.item = to_utf8(slot.item);
        }
    }
    return converted;
}

void push_inventory(PlayerType &player)
{
    auto inventory = observe_inventory(player);
    if (g_has_last_inventory && (inventory == g_last_inventory)) {
        return;
    }

    auto converted = std::make_shared<const ObservedInventory>(to_utf8(inventory));
    g_last_inventory = std::move(inventory);
    g_has_last_inventory = true;
    std::lock_guard lock(g_mutex);
    g_inventory.version = g_inventory_version.load() + 1;
    g_inventory.inventory = std::move(converted);
    g_inventory_version.store(g_inventory.version);
}

void push_messages()
{
    // 件数と最新の 1 件が同じなら変わっていない (同じ文の繰り返しは最新の 1 件の "<xN>" に現れる)
    const auto count = message_num();
    const auto latest = count > 0 ? *message_str(0) : std::string();
    if ((count == g_last_message_count) && (latest == g_last_message)) {
        return;
    }

    g_last_message_count = count;
    g_last_message = latest;
    auto messages = observe_recent_messages(GODOT_MESSAGE_WINDOW_LINES);
    for (auto &message : messages) {
        message = to_utf8(message);
    }

    auto shared = std::make_shared<const std::vector<std::string>>(std::move(messages));
    std::lock_guard lock(g_mutex);
    g_messages.version = g_message_version.load() + 1;
    g_messages.messages = std::move(shared);
    g_message_version.store(g_messages.version);
}

} // namespace

void player_items_set_enabled(bool enabled)
{
    g_enabled.store(enabled);
}

bool player_items_is_enabled()
{
    return g_enabled.load();
}

void player_items_push(PlayerType *player_ptr)
{
    if (!g_enabled.load() || !player_ptr) {
        return;
    }

    // キャラクター作成中は持ち物・メッセージが定まっていない
    if (!AngbandWorld::get_instance().character_generated) {
        return;
    }

    push_inventory(*player_ptr);
    push_messages();
}

uint64_t player_inventory_get_version()
{
    return g_inventory_version.load();
}

GodotInventorySnapshot player_inventory_get()
{
    std::lock_guard lock(g_mutex);
    return g_inventory;
}

uint64_t player_messages_get_version()
{
    return g_message_version.load();
}

GodotMessageSnapshot player_messages_get()
{
    std::lock_guard lock(g_mutex);
    return g_messages;
}
