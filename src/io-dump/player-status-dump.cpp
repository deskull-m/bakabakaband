#include "io-dump/player-status-dump.h"
#include "system/creature-entity.h"
#include "term/z-term.h"
#include "view/display-player.h"
#include "view/display-symbol.h"
#include <fmt/format.h>

namespace {
/// ダンプの幅 (桁)
constexpr TERM_LEN DUMP_WIDTH = 79;

/// 画面外の端末の大きさ。ステータス画面はモンスター名の長さで行数が伸びるので高さに余裕を持たせる
constexpr TERM_LEN OFFSCREEN_WIDTH = 80;
constexpr TERM_LEN OFFSCREEN_HEIGHT = 48;

/*!
 * @brief 画面番号を指定してステータス画面を描き、行ごとに読み取る
 * @param creature クリーチャーへの参照
 * @param lines 読み取った行の追加先
 * @param mode 表示モード
 * @param start_y ダンプの開始行数
 * @param tmp_end_y ダンプの終了行数 (補正前)
 * @param change_color バッファへ詰める文字の変更有無
 * @details モンスター名の長さによっては固定長だとはみ出す可能性があるので、適切に終了行数を調整する.
 */
void collect_status_screen(CreatureEntity &creature, std::vector<PlayerStatusDumpLine> &lines,
    const int mode, const TERM_LEN start_y, const TERM_LEN tmp_end_y, const bool change_color)
{
    auto num_lines = display_player(creature, mode);
    auto end_y = tmp_end_y + num_lines.value_or(0);
    for (auto y = start_y; y < end_y; y++) {
        PlayerStatusDumpLine line;
        for (TERM_LEN x = 0; x < DUMP_WIDTH; x++) {
            DisplaySymbol ds;
            ds = term_what(x, y, ds);
            const auto is_text = !change_color || (ds.color < 128);
            line.text.push_back(is_text ? ds.character : ' ');
            line.colors.push_back(is_text ? ds.color : 0);
        }

        // 従来どおり C 文字列として書くので、NUL があればそこで終わる
        if (const auto nul = line.text.find('\0'); nul != std::string::npos) {
            line.text.resize(nul);
            line.colors.resize(nul);
        }

        while (!line.text.empty() && (line.text.back() == ' ')) {
            line.text.pop_back();
            line.colors.pop_back();
        }

        lines.push_back(std::move(line));
    }
}

}

/*!
 * @brief クリーチャー（プレイヤー・モンスター）のステータス表示を行ごとに作る
 * @details モンスターの場合は mode 0 の共通フォーマット簡易ステータス画面のみを作る。
 * 画面の間には空行を入れる (ダンプのファイルと同じ)
 */
std::vector<PlayerStatusDumpLine> collect_player_status_dump(CreatureEntity &creature)
{
    std::vector<PlayerStatusDumpLine> lines;
    collect_status_screen(creature, lines, 0, 1, 22, false);

    // モンスターは mode 0 で共通フォーマットの簡易ステータス画面のみを描画するため、追加ページは出力しない
    if (!creature.is_player()) {
        lines.emplace_back();
        return lines;
    }

    collect_status_screen(creature, lines, 1, 10, 17, false);
    lines.emplace_back();
    collect_status_screen(creature, lines, 2, 2, 22, true);
    lines.emplace_back();
    collect_status_screen(creature, lines, 3, 1, 18, true);
    lines.emplace_back();
    collect_status_screen(creature, lines, 4, 1, 19, true);
    lines.emplace_back();
    return lines;
}

/*!
 * @brief 画面外の端末に描いてステータス表示を行ごとに作る
 * @details 画面に出していない端末 (term_init() の既定のフックだけを持つ) に切り替えて描き、元の端末に戻す。
 * いまの端末の内容・カーソルには触れないので、ゲームの画面の更新中 (TERM_XTRA_FRESH) からも呼べる
 */
std::vector<PlayerStatusDumpLine> collect_player_status_dump_offscreen(CreatureEntity &creature)
{
    static term_type offscreen{};
    static auto is_initialized = false;
    if (!is_initialized) {
        term_init(&offscreen, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT, 1);
        is_initialized = true;
    }

    auto *const old_term = game_term;
    term_activate(&offscreen);
    std::vector<PlayerStatusDumpLine> lines;
    {
        TermCenteredOffsetSetter tos(OFFSCREEN_WIDTH, tl::nullopt);
        lines = collect_player_status_dump(creature);
    }
    term_activate(old_term);
    return lines;
}

/*!
 * @brief クリーチャー（プレイヤー・モンスター）のステータス表示をファイルにダンプする
 * @param creature クリーチャーへの参照
 * @param fff ファイルポインタ
 */
void dump_aux_player_status(CreatureEntity &creature, FILE *fff)
{
    for (const auto &line : collect_player_status_dump(creature)) {
        fmt::println(fff, "{}", line.text);
    }
}
