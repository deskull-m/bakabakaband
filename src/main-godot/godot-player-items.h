#pragma once
/*!
 * @file godot-player-items.h
 * @brief 持ち物・装備・メッセージ (view/observable-inventory.h) の Godot 側へのブリッジ
 *
 * ゲームスレッドが画面の更新 (TERM_XTRA_FRESH) で取り出し、中身が変わったときだけ版を進めて公開する。
 * Godot メインスレッドは版を見て、変わったときだけ受け取る。
 * 作るのは player_items_set_enabled(true) の間だけ (既定は無効)。
 */

#include "view/observable-inventory.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class PlayerType;

/// 一度に公開するメッセージの最大数
constexpr int GODOT_MESSAGE_WINDOW_LINES = 200;

/// 文字列を UTF-8 にした持ち物と装備
struct GodotInventorySnapshot {
    uint64_t version{ 0 };
    std::shared_ptr<const ObservedInventory> inventory;
};

struct GodotMessageSnapshot {
    uint64_t version{ 0 };
    std::shared_ptr<const std::vector<std::string>> messages; ///< 古い順 (UTF-8)
};

void player_items_set_enabled(bool enabled);
bool player_items_is_enabled();

/// ゲームスレッドから呼ぶ: 有効なら取り出し、変わっていれば公開する
void player_items_push(PlayerType *player_ptr);

uint64_t player_inventory_get_version();
GodotInventorySnapshot player_inventory_get();

uint64_t player_messages_get_version();
GodotMessageSnapshot player_messages_get();
