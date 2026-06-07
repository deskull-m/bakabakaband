/*!
 * @file godot-input-handler.cpp
 * @brief Godot 入力処理 → term_key_push 変換 実装
 */

#include "godot-input-handler.h"

#include "term/z-term.h"
#include "util/string-processor.h"

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;
using namespace hengband_godot;

// ---------------------------------------------------------------------------
// Godot Key → Windows 互換スキャンコード マッピング
// ---------------------------------------------------------------------------

namespace {

struct KeyEntry {
    godot::Key godot_key;
    uint8_t scan_code;
    bool is_numpad;
};

// Windows scan code 参考:
// https://learn.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-6.0/aa299374(v=vs.60)
static constexpr KeyEntry KEY_MAP[] = {
    // 方向キー
    { Key::KEY_UP, 0x48, false },
    { Key::KEY_DOWN, 0x50, false },
    { Key::KEY_LEFT, 0x4B, false },
    { Key::KEY_RIGHT, 0x4D, false },

    // ナビゲーション
    { Key::KEY_HOME, 0x47, false },
    { Key::KEY_END, 0x4F, false },
    { Key::KEY_PAGEUP, 0x49, false },
    { Key::KEY_PAGEDOWN, 0x51, false },
    { Key::KEY_INSERT, 0x52, false },
    { Key::KEY_DELETE, 0x53, false },

    // ファンクションキー
    { Key::KEY_F1, 0x3B, false },
    { Key::KEY_F2, 0x3C, false },
    { Key::KEY_F3, 0x3D, false },
    { Key::KEY_F4, 0x3E, false },
    { Key::KEY_F5, 0x3F, false },
    { Key::KEY_F6, 0x40, false },
    { Key::KEY_F7, 0x41, false },
    { Key::KEY_F8, 0x42, false },
    { Key::KEY_F9, 0x43, false },
    { Key::KEY_F10, 0x44, false },
    { Key::KEY_F11, 0x57, false },
    { Key::KEY_F12, 0x58, false },

    // テンキー (is_numpad=true → 'K' フラグを付ける)
    { Key::KEY_KP_0, 0x52, true },
    { Key::KEY_KP_1, 0x4F, true },
    { Key::KEY_KP_2, 0x50, true },
    { Key::KEY_KP_3, 0x51, true },
    { Key::KEY_KP_4, 0x4B, true },
    { Key::KEY_KP_5, 0x4C, true },
    { Key::KEY_KP_6, 0x4D, true },
    { Key::KEY_KP_7, 0x47, true },
    { Key::KEY_KP_8, 0x48, true },
    { Key::KEY_KP_9, 0x49, true },
    { Key::KEY_KP_ADD, 0x4E, true },
    { Key::KEY_KP_SUBTRACT, 0x4A, true },
    { Key::KEY_KP_MULTIPLY, 0x37, true },
    { Key::KEY_KP_DIVIDE, 0x35, true },
    { Key::KEY_KP_ENTER, 0x1C, true },
    { Key::KEY_KP_PERIOD, 0x53, true },

    // Clear (テンキー中央)
    { Key::KEY_CLEAR, 0x4C, false },

    // Pause/Break
    { Key::KEY_PAUSE, 0x45, false },

    // 終端マーカー
    { Key::KEY_NONE, 0x00, false },
};

} // namespace

ScanCodeEntry hengband_godot::godot_key_to_scancode(Key key)
{
    for (const auto &entry : KEY_MAP) {
        if (entry.godot_key == Key::KEY_NONE) {
            break;
        }
        if (entry.godot_key == key) {
            return { entry.scan_code, entry.is_numpad };
        }
    }
    return { 0, false };
}

// ---------------------------------------------------------------------------
// GodotInputHandler 実装
// ---------------------------------------------------------------------------

void GodotInputHandler::_ready()
{
    set_process_unhandled_input(true);
}

void GodotInputHandler::_unhandled_input(const Ref<InputEvent> &event)
{
    const auto *key_event = Object::cast_to<InputEventKey>(*event);
    if (!key_event || !key_event->is_pressed()) {
        return; // キーダウンのみ処理
    }

    const Key kc = key_event->get_keycode();
    const bool ctrl = key_event->is_ctrl_pressed();
    const bool shift = key_event->is_shift_pressed();
    const bool alt = key_event->is_alt_pressed();

    // 修飾キー単体は無視
    if (kc == Key::KEY_CTRL || kc == Key::KEY_SHIFT ||
        kc == Key::KEY_ALT || kc == Key::KEY_META) {
        return;
    }

    // ESC: Angband では 0x1B をそのままプッシュ
    if (kc == Key::KEY_ESCAPE) {
        push_key(0x1B);
        return;
    }

    // Enter / Return
    if (kc == Key::KEY_ENTER || kc == Key::KEY_KP_ENTER) {
        push_key('\r');
        return;
    }

    // Tab
    if (kc == Key::KEY_TAB) {
        push_key('\t');
        return;
    }

    // BackSpace
    if (kc == Key::KEY_BACKSPACE) {
        push_key(0x08);
        return;
    }

    // Space
    if (kc == Key::KEY_SPACE) {
        push_key(ctrl ? 0x00 : ' ');
        return;
    }

    // 特殊キー (スキャンコードマッピングあり)
    const auto sc_entry = godot_key_to_scancode(kc);
    if (sc_entry.scan_code != 0) {
        handle_special_key(sc_entry.scan_code,
            ctrl, shift, alt, sc_entry.is_numpad);
        return;
    }

    // Ctrl+A〜Z: Windows の Godot では get_unicode() が 0 を返すことがあるため
    // キーコードから制御文字 (0x01〜0x1A) を直接計算するフォールバック
    if (ctrl && kc >= Key::KEY_A && kc <= Key::KEY_Z) {
        push_key(static_cast<int>(kc) - static_cast<int>(Key::KEY_A) + 1);
        return;
    }

    // 通常の印字可能文字
    const char32_t unicode = static_cast<char32_t>(key_event->get_unicode());
    if (unicode >= 0x20) {
        handle_printable(unicode, ctrl);
        return;
    }

    // ASCII 制御文字 (Ctrl+記号 など): get_unicode() が正しく返した場合
    if (unicode > 0 && unicode < 0x20) {
        push_key(static_cast<int>(unicode));
    }
}

void GodotInputHandler::handle_printable(char32_t unicode, bool ctrl)
{
    if (ctrl && unicode >= 'a' && unicode <= 'z') {
        // Ctrl+a〜z → 0x01〜0x1A
        push_key(static_cast<int>(unicode) - 'a' + 1);
    } else if (ctrl && unicode >= 'A' && unicode <= 'Z') {
        push_key(static_cast<int>(unicode) - 'A' + 1);
    } else if (unicode < 0x80) {
        // ASCII 範囲はそのままプッシュ
        push_key(static_cast<int>(unicode));
    } else {
        // マルチバイト Unicode (日本語等): UTF-8 に変換してバイト列をプッシュ
        // Hengband 内部は UTF-8 を想定
        if (unicode < 0x800) {
            push_key(0xC0 | (unicode >> 6));
            push_key(0x80 | (unicode & 0x3F));
        } else if (unicode < 0x10000) {
            push_key(0xE0 | (unicode >> 12));
            push_key(0x80 | ((unicode >> 6) & 0x3F));
            push_key(0x80 | (unicode & 0x3F));
        }
    }
}

void GodotInputHandler::handle_special_key(uint8_t scan_code,
    bool ctrl, bool shift, bool alt, bool is_numpad)
{
    // Angband マクロトリガーエンコーディング
    push_key(31); // prefix (0x1F)
    if (ctrl) {
        push_key('C');
    }
    if (shift) {
        push_key('S');
    }
    if (alt) {
        push_key('A');
    }
    push_key('x');
    if (is_numpad) {
        push_key('K');
    }
    push_key(hexify_upper(scan_code));
    push_key(hexify_lower(scan_code));
    push_key(13); // terminator
}

void GodotInputHandler::push_key(int k)
{
    // term_key_push() はキュー先頭へのプリペンドで逆順になるため使用しない。
    // main-win.cpp の term_keypress() と同じ: key_head に追加する FIFO 方式。
    if (!k || !game_term) {
        return;
    }
    game_term->key_queue[game_term->key_head++] = static_cast<char>(k);
    if (game_term->key_head == game_term->key_size) {
        game_term->key_head = 0;
    }
    {
        std::lock_guard<std::mutex> lock(key_mutex_);
        key_available_ = true;
    }
    key_cv_.notify_one();
}

void GodotInputHandler::wait_for_key()
{
    // ゲームスレッドからキーが来るまでブロック
    std::unique_lock<std::mutex> lock(key_mutex_);
    key_cv_.wait(lock, [this] { return key_available_ || stop_requested_; });
    key_available_ = false;
}

void GodotInputHandler::poll_events()
{
    // ノンブロッキング: キーが既にキューにあれば通知フラグだけリセット
    std::lock_guard<std::mutex> lock(key_mutex_);
    key_available_ = false;
}

void GodotInputHandler::request_stop()
{
    {
        std::lock_guard<std::mutex> lock(key_mutex_);
        stop_requested_ = true;
    }
    key_cv_.notify_all();
}

void GodotInputHandler::inject_key(int k)
{
    push_key(k);
}

void GodotInputHandler::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("wait_for_key"), &GodotInputHandler::wait_for_key);
    ClassDB::bind_method(D_METHOD("poll_events"), &GodotInputHandler::poll_events);
    ClassDB::bind_method(D_METHOD("request_stop"), &GodotInputHandler::request_stop);
    ClassDB::bind_method(D_METHOD("inject_key", "key_code"), &GodotInputHandler::inject_key);
}
