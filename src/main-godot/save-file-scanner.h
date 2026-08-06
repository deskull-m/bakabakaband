#pragma once
/*!
 * @file save-file-scanner.h
 * @brief lib/save/ 内のセーブファイルを解析してキャラクタ情報を返す独立スキャナ
 *
 * Hengband のロード基盤を使わず、バイナリを直接解析する。
 * Godot タイトル画面からゲーム起動前に呼び出すことを想定。
 */

#include <filesystem>
#include <string>
#include <vector>

namespace hengband_godot {

/*!
 * @brief セーブファイルから取り出したキャラクタ情報
 */
struct CharacterSummary {
    std::filesystem::path path; //!< セーブファイルの絶対パス
    std::string char_name; //!< キャラクタ名 (UTF-8)
    int level = 0; //!< レベル
    int prace = 0; //!< 種族番号 (PlayerRaceType)
    int pclass = 0; //!< 職業番号 (PlayerClassType)
    int ppersonality = 0; //!< 性格番号 (player_personality_type)
    bool valid = false; //!< 解析成功フラグ
};

/*!
 * @brief save_dir 内の全セーブファイルをスキャンしてキャラクタ情報を返す
 * @param save_dir lib/save/ ディレクトリのパス
 * @return 解析できたファイルの CharacterSummary 一覧
 */
std::vector<CharacterSummary> scan_save_files(const std::filesystem::path &save_dir);

} // namespace hengband_godot
