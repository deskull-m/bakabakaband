/*!
 * @file godot-screen-state.cpp
 * @brief 画面の状態の Godot 側へのブリッジ実装
 */

#include "main-godot/godot-screen-state.h"

#include "game-option/special-options.h"
#include "locale/character-encoding.h"
#include "window/main-window-util.h"
#include "world/world.h"

#include <atomic>
#include <mutex>

namespace {

std::atomic<uint64_t> g_version{ 0 };
std::mutex g_mutex;
GodotScreenState g_state; ///< g_mutex で保護

/// ゲームスレッド専用: 前回公開した内容 (変化の判定用)
ScreenStateInfo g_last_info;
int g_last_depth = -1;
std::string g_last_prompt;
GodotScreenState::MapPanel g_last_panel;

std::string to_utf8(const std::string &text)
{
#ifdef JP
    const auto utf8 = sys_to_utf8(text);
    return utf8 ? *utf8 : std::string();
#else
    return text;
#endif
}

} // namespace

void screen_state_update()
{
    const auto depth = static_cast<int>(AngbandWorld::get_instance().character_icky_depth);
    const auto inputs = collect_screen_state_inputs();
    const auto info = classify_screen_state(inputs);
    const auto prompt = inputs.input_context ? inputs.input_context->prompt : std::string();
    const GodotScreenState::MapPanel panel{ panel_col_of(panel_col_min), panel_col_min, panel_row_prt, use_bigtile };
    if ((info == g_last_info) && (depth == g_last_depth) && (prompt == g_last_prompt) && (panel == g_last_panel)) {
        return;
    }

    g_last_info = info;
    g_last_depth = depth;
    g_last_prompt = prompt;
    g_last_panel = panel;

    std::lock_guard lock(g_mutex);
    g_state.version = g_version.load() + 1;
    g_state.info = info;
    g_state.icky_depth = depth;
    g_state.prompt = to_utf8(prompt);
    g_state.map_panel = panel;
    g_version.store(g_state.version);
}

uint64_t screen_state_get_version()
{
    return g_version.load();
}

GodotScreenState screen_state_get()
{
    std::lock_guard lock(g_mutex);
    return g_state;
}
