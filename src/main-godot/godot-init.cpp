/*!
 * @file godot-init.cpp
 * @brief Godot 版ゲーム初期化実装
 */

#include "main-godot/godot-init.h"

#include "core/game-play.h"
#include "io/files-util.h"
#include "main/angband-initializer.h"
#include "system/player-type-definition.h"
#include "system/system-variables.h"
#include "term/gameterm.h"
#include "term/z-term.h"
#include "term/z-util.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <exception>
#include <filesystem>
#include <string>

namespace hengband_godot {

/// quit_aux: std::exit() の代わりに Godot のシーンツリーを終了させる
static void godot_quit_aux(std::string_view msg)
{
    // quit("メッセージ") で渡された終了理由をコンソールに出力する。
    // これが無いと init_angband / play_game がエラーで quit した際に、
    // 理由が分からないままウィンドウだけ閉じてしまう。
    if (!msg.empty()) {
        godot::UtilityFunctions::printerr(
            godot::String("Hengband quit: ") + godot::String(std::string(msg).c_str()));
    }

    // メインループに quit を要求する (ゲームスレッドから呼ばれる)
    auto *engine = godot::Engine::get_singleton();
    if (engine) {
        auto *main_loop = engine->get_main_loop();
        if (main_loop) {
            auto *tree = godot::Object::cast_to<godot::SceneTree>(main_loop);
            if (tree) {
                tree->quit();
            }
        }
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

    term_activate(term_screen);

    auto &player = PlayerType::get_instance();

    // ゲームスレッドで未捕捉の C++ 例外が発生すると std::terminate で
    // プロセスごとサイレントに落ちてしまう (Godot 側にもログが出ない)。
    // ここで捕捉してコンソールに出力し、Godot を正常終了させる。
    try {
        init_angband(player, false);

        // セーブファイルが指定されていなければ新規ゲーム
        const bool new_game = savefile.empty();
        play_game(player, new_game, false);

        quit(nullptr);
    } catch (const std::exception &e) {
        godot::UtilityFunctions::printerr(
            godot::String("Hengband game thread exception: ") + godot::String(e.what()));
        godot_quit_aux({});
    } catch (...) {
        godot::UtilityFunctions::printerr(
            "Hengband game thread terminated by unknown exception");
        godot_quit_aux({});
    }
}

} // namespace hengband_godot
