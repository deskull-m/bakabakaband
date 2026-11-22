/*!
 * @file godot-init.cpp
 * @brief Godot 版ゲーム初期化実装
 */

#include "main-godot/godot-init.h"
#include "main-godot/godot-game-events.h"
#include "main-godot/godot-world-snapshot.h"

#include "core/game-play.h"
#include "io/files-util.h"
#include "locale/character-encoding.h"
#include "main/angband-initializer.h"
#include "system/player-type-definition.h"
#include "system/system-variables.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/z-term.h"
#include "term/z-util.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <string>
#include <thread>

namespace hengband_godot {

namespace {
    /// ゲームスレッドの ID (run_game_thread() が設定する)
    std::thread::id game_thread_id;

    /*!
     * @brief ゲームスレッドで quit() が呼ばれたことを run_game_thread() へ伝える例外
     * @details quit() は quit_aux から戻ると std::exit() を呼ぶ。ゲームスレッドから std::exit() すると
     * Godot の実行中にプロセスの後始末 (静的オブジェクトの破棄) が始まり、異常終了 (signal 11) する。
     * そこで quit_aux から戻らずにこの例外でゲームスレッドの先頭まで巻き戻し、スレッドを普通に終わらせる。
     */
    struct GameThreadQuit {
    };

    /// Godot の SceneTree::quit() が受け付ける終了コードの上限
    constexpr int MAX_GODOT_EXIT_CODE = 125;

    /// quit() の文字列が終了コードの指定 ('-' / '+' で始まる数値) か。quit() はこの場合メッセージを出さない
    bool is_exit_code_message(std::string_view msg)
    {
        return !msg.empty() && ((msg.front() == '-') || (msg.front() == '+'));
    }

    /// ゲームの内部の文字コード (日本語版は Shift_JIS / EUC-JP) の文字列を Godot の String にする
    godot::String sys_message_to_godot(std::string_view msg)
    {
        const auto utf8 = sys_to_utf8(msg);
        return godot::String::utf8(utf8 ? utf8->c_str() : std::string(msg).c_str());
    }

    /*!
     * @brief quit() に渡された文字列から、プロセスの終了コードを決める (quit() が std::exit() に渡す値と同じ規則)
     * @details 空なら成功 (0)、'-' / '+' で始まれば数値の終了コード、それ以外はエラー (EXIT_FAILURE)。
     * Godot の SceneTree::quit() は 0〜125 の範囲を想定し負の値を無視して成功扱いにするので、
     * 範囲外の数値の終了コードは EXIT_FAILURE にする。
     */
    int exit_status_from_quit_message(std::string_view msg)
    {
        if (msg.empty()) {
            return 0;
        }

        if (!is_exit_code_message(msg)) {
            return EXIT_FAILURE;
        }

        const auto code = std::atoi(std::string(msg).c_str());
        return (code >= 0 && code <= MAX_GODOT_EXIT_CODE) ? code : EXIT_FAILURE;
    }

    /// メインループ (シーンツリー) に終了を頼む
    void request_tree_quit(int exit_status)
    {
        auto *engine = godot::Engine::get_singleton();
        if (!engine) {
            return;
        }

        auto *tree = godot::Object::cast_to<godot::SceneTree>(engine->get_main_loop());
        if (tree) {
            tree->quit(exit_status);
        }
    }
}

/// quit_aux: std::exit() の代わりに Godot のシーンツリーを終了させる
static void godot_quit_aux(std::string_view msg)
{
    // quit("メッセージ") で渡された終了理由をコンソールに出力する。
    // これが無いと init_angband / play_game がエラーで quit した際に、
    // 理由が分からないままウィンドウだけ閉じてしまう。
    if (!msg.empty() && !is_exit_code_message(msg)) {
        const auto text = sys_message_to_godot(msg);
        godot::UtilityFunctions::printerr(godot::String("Hengband quit: ") + text);
        // ウィンドウから起動した場合はコンソールが見えないので、閉じる前に理由を知らせる
        if (auto *os = godot::OS::get_singleton(); os) {
            os->alert(text, "Bakabakaband");
        }
    }

    request_tree_quit(exit_status_from_quit_message(msg));

    // ゲームスレッドからの quit() は、ここから戻ると std::exit() されてしまうので巻き戻す
    if (std::this_thread::get_id() == game_thread_id) {
        throw GameThreadQuit{};
    }
}

void init_godot_game_paths(const std::filesystem::path &lib_path)
{
    // 例外トレーサーを最初に登録する (以降の例外スローを捕捉できるように)
    install_cpp_exception_tracer();

    init_file_paths(lib_path);

    // システム識別子 (プリファレンスファイル選択に使用される)
    // "win" を指定することで pref-win.prf のマクロトリガー定義を読み込む
    // (Windows スキャンコード形式 ^_[CSA]x[K]sc1sc2\r を使用するため)
    ANGBAND_SYS = "win";
    ANGBAND_KEYBOARD = "JAPAN";

    // quit() が std::exit() を呼ばないよう Godot 用フックを設定
    quit_aux = godot_quit_aux;
}

void run_game_thread(const std::filesystem::path &lib_path,
    const std::filesystem::path &save_path)
{
    init_godot_game_paths(lib_path);

    // セーブファイルパスが指定されていればグローバル変数に設定する
    if (!save_path.empty()) {
        savefile = save_path;
        savefile_base = save_path.filename();
    }

    // 地図表示の変化をワールドスナップショットの差分更新に使う (#9293)
    world_snapshot_install_observer();
    // 表示用のゲームイベントを Godot 側へ流す (#9295)
    game_events_install_sink();
    // 3D 表示の上に一覧 (持ち物・装備など) を写すため、地図の上に重ねた範囲を求められるようにする
    set_main_screen_overlay_tracking(true);

    term_activate(term_screen);

    auto &player = PlayerType::get_instance();
    game_thread_id = std::this_thread::get_id();

    // ゲームスレッドで未捕捉の C++ 例外が発生すると std::terminate で
    // プロセスごとサイレントに落ちてしまう (Godot 側にもログが出ない)。
    // ここで捕捉してコンソールに出力し、Godot を正常終了させる。
    try {
        init_angband(player, false);

        // セーブファイルが指定されていなければ新規ゲーム
        const bool new_game = savefile.empty();
        play_game(player, new_game, false);

        quit({});
    } catch (const GameThreadQuit &) {
        // quit() による終了。シーンツリーへの終了の依頼は godot_quit_aux が済ませている
    } catch (const std::exception &e) {
        godot::UtilityFunctions::printerr(
            godot::String("Hengband game thread exception: ") + sys_message_to_godot(e.what()));
        request_tree_quit(EXIT_FAILURE);
    } catch (...) {
        godot::UtilityFunctions::printerr(
            "Hengband game thread terminated by unknown exception");
        request_tree_quit(EXIT_FAILURE);
    }
}

} // namespace hengband_godot
