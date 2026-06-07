#pragma once
/*!
 * @file godot-input-handler.h
 * @brief Godot 入力処理 → term_key_push 変換
 *
 * Godot の InputEventKey を Hengband のキーキューに注入する。
 *
 * エンコーディング (main-win.cpp の process_keydown と同じ規約):
 *   通常文字:  term_key_push(char_code)
 *   特殊キー:  31 [C] [S] [A] 'x' [K] hex_upper(sc) hex_lower(sc) 13
 *              C/S/A は Ctrl/Shift/Alt が押されているときのみ
 *              K はテンキーのときのみ
 *              sc = Windows 互換スキャンコード
 *
 * スレッドモデル (Phase 7 まではスタブ):
 *   入力イベントは Godot メインスレッドで受信し term_key_push() で
 *   キューに積む。ゲームスレッドは TERM_XTRA_EVENT で条件変数を待つ。
 *   Phase 4 では TERM_XTRA_EVENT は即時リターン（スレッドなし）。
 */

#include <condition_variable>
#include <cstdint>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/node.hpp>
#include <mutex>

namespace hengband_godot {

/*!
 * @brief 入力処理ノード
 *
 * シーンのどこかに配置するか HengbandGame の子として追加する。
 * _unhandled_input() で全キー入力を受け取る。
 */
class GodotInputHandler : public godot::Node {
    GDCLASS(GodotInputHandler, godot::Node)

public:
    GodotInputHandler() = default;
    ~GodotInputHandler() override = default;

    void _ready() override;
    void _unhandled_input(const godot::Ref<godot::InputEvent> &event) override;

    // --- Phase 7 で使用するスレッド同期プリミティブ ---

    /// TERM_XTRA_EVENT(wait=true) から呼ばれる。キーが来るまでブロック。
    void wait_for_key();

    /// TERM_XTRA_EVENT(wait=false) の non-blocking 版。
    void poll_events();

    /// ゲームスレッド停止時に wait_for_key() のブロックを解除する
    void request_stop();

    /// 任意のキーコードをキューに直接注入する（GDScript / 外部から呼び出す）
    void inject_key(int k);

protected:
    static void _bind_methods();

private:
    std::mutex key_mutex_;
    std::condition_variable key_cv_;
    bool key_available_{ false };
    bool stop_requested_{ false };

    /// キーを push してスレッドに通知する
    void push_key(int k);

    /// 通常文字の処理
    void handle_printable(char32_t unicode, bool ctrl);

    /// 特殊キーの処理
    /// @param scan_code Windows 互換スキャンコード
    /// @param ctrl/shift/alt 修飾キー
    /// @param is_numpad テンキーフラグ
    void handle_special_key(uint8_t scan_code,
        bool ctrl, bool shift, bool alt, bool is_numpad);
};

/// Godot Key → (scan_code, is_numpad) の検索
/// 見つからない場合は {0, false} を返す
struct ScanCodeEntry {
    uint8_t scan_code;
    bool is_numpad;
};
ScanCodeEntry godot_key_to_scancode(godot::Key key);

} // namespace hengband_godot
