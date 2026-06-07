#pragma once
/*!
 * @file godot-init.h
 * @brief Godot 版ゲーム初期化 (WinMain 相当)
 *
 * main-win.cpp の init_stuff() / WinMain() 相当を Godot API で実装する。
 * ANGBAND_DIR 等のパスを実行ファイルパスから設定し、
 * init_file_paths() と init_angband() を呼び出す。
 */

#include <filesystem>

namespace hengband_godot {

/*!
 * @brief ゲームパスを初期化する
 * @param exe_path 実行ファイルのパス (OS::get_executable_path() の値)
 *
 * - ANGBAND_DIR     = exe_path/../lib/
 * - ANGBAND_SYS     = "godot"
 * - ANGBAND_KEYBOARD = "JAPAN"
 * - special_key[] / ignore_key[] を設定
 * - init_file_paths(ANGBAND_DIR) を呼ぶ
 */
void init_godot_game_paths(const std::filesystem::path &exe_path);

/// ゲームスレッドのエントリポイント
/// @param lib_path  lib/ ディレクトリの絶対パス
/// @param save_path セーブファイルのパス（空文字列 = 新規ゲーム）
void run_game_thread(const std::filesystem::path &lib_path,
    const std::filesystem::path &save_path = {});

} // namespace hengband_godot
