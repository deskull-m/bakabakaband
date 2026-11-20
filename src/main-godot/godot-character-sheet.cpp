/*!
 * @file godot-character-sheet.cpp
 * @brief ステータス画面のダンプの Godot 側へのブリッジ実装
 */

#include "main-godot/godot-character-sheet.h"

#include "io-dump/player-status-dump.h"
#include "locale/character-encoding.h"
#include "system/h-config.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "world/world.h"

#include <atomic>
#include <mutex>

namespace {

/// 端末の属性のうち色の部分 (z-term.cpp の AF_KANJIC と同じ)
constexpr uint8_t TERM_COLOR_MASK = 0x0F;

std::atomic<bool> g_enabled{ false };
std::atomic<bool> g_refresh_requested{ true };
std::atomic<uint64_t> g_version{ 0 };
std::mutex g_mutex;
GodotCharacterSheet g_sheet; ///< g_mutex で保護

/// ゲームスレッド専用
std::vector<PlayerStatusDumpLine> g_last_lines;
GAME_TURN g_last_turn = 0;
uint64_t g_last_holdings = 0;

/*!
 * @brief 所持金と持ち物の指紋 (ターンが進まない店での売買・装備の変更を拾う)
 * @details 店の中ではゲームターンが進まないので、ターンだけを見ると売買で変わった所持金が残る
 */
uint64_t calc_holdings_fingerprint(const PlayerType &player)
{
    auto hash = static_cast<uint64_t>(static_cast<uint32_t>(player.get_au()));
    for (const auto &item : player.inventory) {
        const auto value = item ? ((static_cast<uint64_t>(static_cast<uint16_t>(item->bi_id)) << 16) | static_cast<uint16_t>(item->number)) : 0;
        hash = (hash * 1099511628211ULL) ^ value;
    }
    return hash;
}

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
            // 日本語版の端末は全角の左右の桁に印 (AF_KANJI1 = 0x10 / AF_KANJI2 = 0x20) を色に重ねるので、色の部分だけを取る
            const auto color = static_cast<uint8_t>(line.colors[i] & TERM_COLOR_MASK);
            converted.cells.push_back({ static_cast<int>(i), length, to_utf8(text.substr(i, length)), color });
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

    // 1 回作るのにステータス画面を 5 枚描くので、ゲームターンが進んだか所持金・持ち物が変わったときだけ作り直す
    const auto holdings = calc_holdings_fingerprint(*player_ptr);
    if (!g_refresh_requested.exchange(false) && (world.game_turn == g_last_turn) && (holdings == g_last_holdings)) {
        return;
    }

    g_last_turn = world.game_turn;
    g_last_holdings = holdings;
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

bool character_sheet_is_refresh_requested()
{
    return g_enabled.load() && g_refresh_requested.load();
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
