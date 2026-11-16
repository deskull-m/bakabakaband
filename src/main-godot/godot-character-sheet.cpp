/*!
 * @file godot-character-sheet.cpp
 * @brief ステータス画面のダンプの Godot 側へのブリッジ実装
 */

#include "main-godot/godot-character-sheet.h"

#include "io-dump/player-status-dump.h"
#include "locale/character-encoding.h"
#include "system/h-config.h"
#include "system/player-type-definition.h"
#include "world/world.h"

#include <atomic>
#include <mutex>

namespace {

std::atomic<bool> g_enabled{ false };
std::atomic<bool> g_refresh_requested{ true };
std::atomic<uint64_t> g_version{ 0 };
std::mutex g_mutex;
GodotCharacterSheet g_sheet; ///< g_mutex で保護

/// ゲームスレッド専用
std::vector<PlayerStatusDumpLine> g_last_lines;
GAME_TURN g_last_turn = 0;

std::string to_utf8(const std::string &text)
{
    const auto utf8 = sys_to_utf8(text);
    return utf8 ? *utf8 : std::string();
}

/// 1 行を文字ごとに分ける。全角 (2 バイト文字) は 2 桁を占める
GodotSheetLine split_line(const PlayerStatusDumpLine &line)
{
    GodotSheetLine converted;
    converted.text = to_utf8(line.text);
    const auto &text = line.text;
    for (size_t i = 0; i < text.size();) {
        auto length = 1;
#ifdef JP
        if (iskanji(text[i]) && (i + 1 < text.size())) {
            length = 2;
        }
#endif
        if (text[i] != ' ') {
            converted.cells.push_back({ static_cast<int>(i), length, to_utf8(text.substr(i, length)), line.colors[i] });
        }

        i += length;
    }

    return converted;
}

} // namespace

void character_sheet_set_enabled(bool enabled)
{
    g_enabled.store(enabled);
    if (enabled) {
        g_refresh_requested.store(true);
    }
}

void character_sheet_request_refresh()
{
    g_refresh_requested.store(true);
}

void character_sheet_push(PlayerType *player_ptr)
{
    if (!g_enabled.load() || !player_ptr) {
        return;
    }

    const auto &world = AngbandWorld::get_instance();
    if (!world.character_generated) {
        return;
    }

    // 1 回作るのにステータス画面を 5 枚描くので、ゲームターンが進んだときだけ作り直す
    if (!g_refresh_requested.exchange(false) && (world.game_turn == g_last_turn)) {
        return;
    }

    g_last_turn = world.game_turn;
    auto lines = collect_player_status_dump_offscreen(*player_ptr);
    if (lines == g_last_lines) {
        return;
    }

    auto converted = std::make_shared<std::vector<GodotSheetLine>>();
    converted->reserve(lines.size());
    for (const auto &line : lines) {
        converted->push_back(split_line(line));
    }

    g_last_lines = std::move(lines);
    std::lock_guard lock(g_mutex);
    g_sheet.version = g_version.load() + 1;
    g_sheet.lines = std::move(converted);
    g_version.store(g_sheet.version);
}

uint64_t character_sheet_get_version()
{
    return g_version.load();
}

GodotCharacterSheet character_sheet_get()
{
    std::lock_guard lock(g_mutex);
    return g_sheet;
}
