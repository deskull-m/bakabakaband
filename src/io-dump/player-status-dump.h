#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

class CreatureEntity;

/*!
 * @brief ステータス画面のダンプの 1 行
 * @details text はダンプに書く文字列そのもの (内部の文字コード)。colors は text の各バイトの端末の色
 */
struct PlayerStatusDumpLine {
    std::string text;
    std::vector<uint8_t> colors;

    bool operator==(const PlayerStatusDumpLine &other) const = default;
};

void dump_aux_player_status(CreatureEntity &creature, FILE *fff);

/// ステータス画面のダンプ (キャラクタダンプの先頭と同じ内容) を行ごとに作る。いまの端末に描いて読み取る
std::vector<PlayerStatusDumpLine> collect_player_status_dump(CreatureEntity &creature);

/// collect_player_status_dump() を画面外の端末 (80 桁) で行う。いまの端末の表示は変えない
std::vector<PlayerStatusDumpLine> collect_player_status_dump_offscreen(CreatureEntity &creature);
