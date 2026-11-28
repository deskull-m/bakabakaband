#pragma once

#include <cstdint>

/*!
 * @file save-header-reserved.h
 * @brief セーブデータのヘッダの予約領域 (旧 sf_system / sf_when / sf_lives / sf_saves) の使い道
 * @details
 * セーブデータのバージョンの直前の 12 バイトは上流 hengband#5402 で廃止された領域で、
 * ローダ (rd_version_info()) は中身を見ずに読み飛ばす。ここにゲームを始めずに読める情報を置き、
 * Godot 版のタイトル画面のロード一覧 (src/main-godot/save-file-scanner.cpp) が使う。
 * ローダが読み飛ばすので、書く内容を変えてもセーブデータのバージョンの更新は要らない。
 *
 * | オフセット | 型 | 内容 |
 * |---|---|---|
 * | 0 | u32 | そのキャラクターのプレイ時間 [秒] (旧 sf_system。旧版は 0) |
 * | 4 | u32 | 保存した時刻 (time_t。旧 sf_when) |
 * | 8 | u16 | 下の SaveHeaderReservedFlag の論理和。どの欄が有効か (旧 sf_lives。旧版は 0) |
 * | 10 | u16 | 未使用 (旧 sf_saves) |
 */
enum SaveHeaderReservedFlag : uint16_t {
    SAVE_HEADER_HAS_PLAY_TIME = 0x0001, //!< オフセット 0 にプレイ時間を書いている
};
