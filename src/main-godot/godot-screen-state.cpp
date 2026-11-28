/*!
 * @file godot-screen-state.cpp
 * @brief 画面の状態の Godot 側へのブリッジ実装
 */

#include "main-godot/godot-screen-state.h"

#include "game-option/special-options.h"
#include "locale/character-encoding.h"
#include "system/player-type-definition.h"
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
tl::optional<ScreenRegion> g_last_overlay;
tl::optional<ObservedTargetPath> g_last_target_path;

/// ターゲットの選択 (見るコマンドを除く) でカーソルが地図の上にあるなら、プレイヤーからの射線を求める
tl::optional<ObservedTargetPath> collect_target_path(const ScreenStateInputs &inputs, const ScreenStateInfo &info)
{
    if ((info.state != ScreenState::TARGETING) || !info.cursor_pos || !inputs.input_context) {
        return tl::nullopt;
    }

    const auto *details = std::get_if<TargetInputDetails>(&inputs.input_context->details);
    if ((details == nullptr) || details->look) {
        return tl::nullopt;
    }

    return observe_target_path(PlayerType::get_instance(), *info.cursor_pos);
}

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
    const auto overlay = calc_main_screen_overlay_region();
    const auto target_path = collect_target_path(inputs, info);
    if ((info == g_last_info) && (depth == g_last_depth) && (prompt == g_last_prompt) && (panel == g_last_panel) && (overlay == g_last_overlay) && (target_path == g_last_target_path)) {
        return;
    }

    g_last_info = info;
    g_last_depth = depth;
    g_last_prompt = prompt;
    g_last_panel = panel;
    g_last_overlay = overlay;
    g_last_target_path = target_path;

    std::lock_guard lock(g_mutex);
    g_state.version = g_version.load() + 1;
    g_state.info = info;
    g_state.icky_depth = depth;
    g_state.prompt = to_utf8(prompt);
    g_state.map_panel = panel;
    g_state.overlay_region = overlay;
    g_state.target_path = target_path;
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
