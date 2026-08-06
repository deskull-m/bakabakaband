#pragma once
/*!
 * @file godot-player-status.h
 * @brief プレイヤーステータスのGodot側へのブリッジ
 *
 * ゲームスレッドから player_status_push() を呼び出して
 * GodotPlayerStatusSnapshot を更新し、Godotメインスレッドから
 * player_status_get_snapshot() でコピーを取得する。
 *
 * スレッド安全性:
 *   内部mutex で保護しているため、ゲームスレッド / Godotメインスレッド
 *   双方から安全に呼び出せる。
 */

#include <array>
#include <cstdint>
#include <string>

class PlayerType;

/// Godot UI に渡すプレイヤーステータスのスナップショット
struct GodotPlayerStatusSnapshot {
    bool is_valid{ false }; ///< 初回更新前は false
    uint64_t version{ 0 }; ///< push のたびにインクリメント（変化検出用）

    // --- 基本情報 ---
    std::string name;
    std::string race_name;
    std::string class_name;
    int level{ 0 };
    int dun_level{ 0 }; ///< 現在のダンジョン階

    // --- HP / SP ---
    int chp{ 0 };
    int mhp{ 0 };
    int current_mp{ 0 };
    int max_mp{ 0 };

    // --- 所持金・経験値 ---
    long gold{ 0 };
    long exp{ 0 };
    long max_exp{ 0 };

    // --- 戦闘パラメータ ---
    int speed{ 0 }; ///< pspeed - STANDARD_SPEED (正 = 加速)
    int display_ac{ 0 }; ///< dis_ac + dis_to_a (表示上の合計AC)

    // --- 能力値 (A_STR=0 ~ A_CHR=5) ---
    std::array<int, 6> stat_use{}; ///< 補正済み実効値
    std::array<int, 6> stat_top{}; ///< 最大補正値
};

/// ゲームスレッドから呼ぶ: PlayerType の現在値をスナップショットに反映する
void player_status_push(const PlayerType *player_ptr);

/// Godotメインスレッドから呼ぶ: スナップショットのコピーを返す
GodotPlayerStatusSnapshot player_status_get_snapshot();
