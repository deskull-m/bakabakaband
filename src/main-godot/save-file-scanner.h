#pragma once
/*!
 * @file save-file-scanner.h
 * @brief lib/save/ 内のセーブファイルを解析してキャラクタ情報を返す独立スキャナ
 *
 * Hengband のロード基盤を使わず、バイナリを直接解析する。
 * Godot タイトル画面からゲーム起動前に呼び出すことを想定。
 * 読む順序は src/load/ のローダ (load.cpp の rd_savefile_new_aux() 以降) と完全に揃えること。
 */

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace hengband_godot {

/*!
 * @brief セーブファイルから取り出したキャラクタ情報
 */
struct CharacterSummary {
    std::filesystem::path path; //!< セーブファイルの絶対パス
    std::string char_name; //!< キャラクタ名 (セーブデータの文字コードのまま。空ならファイル名を使う)
    uint8_t character_encoding = 0; //!< セーブデータの文字コード (CharacterEncoding)
    uint32_t savefile_version = 0; //!< セーブデータのフォーマット版 (SAVEFILE_VERSION)
    int level = -1; //!< レベル (-1 = 読めなかった)
    int prace = -1; //!< 種族番号 (PlayerRaceType, -1 = 読めなかった)
    int pclass = -1; //!< 職業番号 (PlayerClassType, -1 = 読めなかった)
    int ppersonality = -1; //!< 性格番号 (player_personality_type, -1 = 読めなかった)
    int64_t play_time_sec = -1; //!< そのキャラクターのプレイ時間 [秒] (-1 = 記録の無い旧版のセーブデータ)
    bool valid = false; //!< 馬鹿馬鹿蛮怒のセーブデータとして読めたか (ヘッダが正しいか)
};

/*!
 * @brief 1 つのセーブデータのバイト列を解析する
 * @param data セーブファイルの中身
 * @return 解析結果。ヘッダが正しければ valid が真で、途中で読めなくなった項目は -1 のまま
 */
CharacterSummary scan_save_data(const std::vector<uint8_t> &data);

/*!
 * @brief save_dir 内の全セーブファイルをスキャンしてキャラクタ情報を返す
 * @param save_dir lib/save/ ディレクトリのパス
 * @return ヘッダが正しかったファイルの CharacterSummary 一覧 (順序は不定)
 */
std::vector<CharacterSummary> scan_save_files(const std::filesystem::path &save_dir);

} // namespace hengband_godot
