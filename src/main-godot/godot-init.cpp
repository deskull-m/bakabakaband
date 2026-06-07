/*!
 * @file godot-init.cpp
 * @brief Godot 版ゲーム初期化実装
 */

#include "godot-init.h"

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

#include <filesystem>

namespace hengband_godot {

/// quit_aux: std::exit() の代わりに Godot のシーンツリーを終了させる
static void godot_quit_aux(std::string_view /*msg*/)
{
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
    init_angband(player, false);

    // セーブファイルが指定されていなければ新規ゲーム
    const bool new_game = savefile.empty();
    play_game(player, new_game, false);

    quit(nullptr);
}

} // namespace hengband_godot
