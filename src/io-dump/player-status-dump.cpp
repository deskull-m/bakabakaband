#include "io-dump/player-status-dump.h"
#include "system/creature-entity.h"
#include "view/display-player.h"
#include "view/display-symbol.h"

/*!
 * @brief 画面番号を指定してダンプする
 * @param creature クリーチャーへの参照
 * @param fff ファイルポインタ
 * @param mode 表示モード
 * @param start_y ダンプの開始行数
 * @param tmp_end_y ダンプの終了行数 (補正前)
 * @param change_color バッファへ詰める文字の変更有無
 * @details モンスター名の長さによっては固定長だとはみ出す可能性があるので、適切に終了行数を調整する.
 */
static void dump_player_status_with_screen_num(CreatureEntity &creature, FILE *fff,
    const int mode, const TERM_LEN start_y, const TERM_LEN tmp_end_y, const bool change_color)
{
    char buf[1024]{};
    auto num_lines = display_player(creature, mode);
    auto end_y = tmp_end_y + num_lines.value_or(0);
    for (auto y = start_y; y < end_y; y++) {
        TERM_LEN x;
        for (x = 0; x < 79; x++) {
            DisplaySymbol ds;
            ds = term_what(x, y, ds);
            if (!change_color) {
                buf[x] = ds.character;
                continue;
            }

            buf[x] = ds.color < 128 ? ds.character : ' ';
        }

        buf[x] = '\0';
        while ((x > 0) && (buf[x - 1] == ' ')) {
            buf[--x] = '\0';
        }

        fprintf(fff, "%s\n", buf);
    }
}

/*!
 * @brief クリーチャー（プレイヤー・モンスター）のステータス表示をファイルにダンプする
 * @param creature クリーチャーへの参照
 * @param fff ファイルポインタ
 * @details モンスターの場合は mode 0 の共通フォーマット簡易ステータス画面のみを出力する。
 */
void dump_aux_player_status(CreatureEntity &creature, FILE *fff)
{
    dump_player_status_with_screen_num(creature, fff, 0, 1, 22, false);

    // モンスターは mode 0 で共通フォーマットの簡易ステータス画面のみを描画するため、追加ページは出力しない
    if (!creature.is_player()) {
        fprintf(fff, "\n");
        return;
    }

    dump_player_status_with_screen_num(creature, fff, 1, 10, 17, false);
    fprintf(fff, "\n");
    dump_player_status_with_screen_num(creature, fff, 2, 2, 22, true);
    fprintf(fff, "\n");
    dump_player_status_with_screen_num(creature, fff, 3, 1, 18, true);
    fprintf(fff, "\n");
    dump_player_status_with_screen_num(creature, fff, 4, 1, 19, true);
    fprintf(fff, "\n");
}
