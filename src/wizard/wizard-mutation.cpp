/*!
 * @brief ウィザードモードの突然変異コマンド
 * @date 2025/12/01
 * @author AI Assistant
 */

#include "wizard/wizard-mutation.h"
#include "core/asking-player.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "mutation/mutation-flag-types.h"
#include "mutation/mutation-investor-remover.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "term/term-color-types.h"
#include <algorithm>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

enum class MutationCategory {
    ACTIVE_POWERS = 0, // アクティブ能力 (0-31)
    RANDOM_EFFECTS = 1, // ランダム効果 (32-63)
    PASSIVE_TRAITS = 2, // パッシブ特性 (64-95)
    SPECIAL_TRAITS = 3, // 特殊特性 (96+)
    MAX
};

static const std::vector<std::vector<std::pair<int, std::string>>> mutation_categories = {
    // アクティブ能力 (0-31)
    {
        { 0, "酸の唾" }, { 1, "炎のブレス" }, { 2, "催眠睨み" }, { 3, "念動力" },
        { 4, "テレポート" }, { 5, "精神攻撃" }, { 6, "放射能" }, { 7, "吸血" },
        { 8, "金属嗅覚" }, { 9, "敵臭嗅覚" }, { 10, "ショート・テレポート" }, { 11, "岩喰い" },
        { 12, "位置交換" }, { 13, "叫び" }, { 14, "照明" }, { 15, "呪い感知" },
        { 16, "狂戦士化" }, { 17, "変身" }, { 18, "ミダスの手" }, { 19, "カビ発生" },
        { 20, "エレメント耐性" }, { 21, "地震" }, { 22, "魔力喰い" }, { 23, "魔力感知" },
        { 24, "増殖阻止" }, { 25, "ヒットアンドアウェイ" }, { 26, "眩惑" }, { 27, "レーザー・アイ" },
        { 28, "帰還" }, { 29, "邪悪消滅" }, { 30, "凍結の手" }, { 31, "アイテム投げ" } },
    // ランダム効果 (32-63)
    {
        { 32, "狂戦士化の発作" }, { 33, "臆病" }, { 34, "ランダムテレポート" }, { 35, "アルコール分泌" },
        { 36, "幻覚" }, { 37, "猛烈な屁" }, { 38, "サソリの尻尾" }, { 39, "ツノ" },
        { 40, "クチバシ" }, { 41, "デーモンを引き付ける" }, { 42, "制御できない魔力" }, { 43, "ランダムな加減速" },
        { 44, "ランダムなモンスター消滅" }, { 45, "光源喰い" }, { 46, "象の鼻" }, { 47, "動物を引き寄せる" },
        { 48, "邪悪な触手" }, { 49, "純カオス" }, { 50, "ランダムな変異の消滅" }, { 51, "ランダムな幽体化" },
        { 52, "ランダムな傷の変化" }, { 53, "衰弱" }, { 54, "ドラゴンを引き寄せる" }, { 55, "ランダムなテレパシー" },
        { 56, "落ち着きの無い胃" }, { 57, "パトロン" }, { 58, "ランダムな現実変容" }, { 59, "警告" },
        { 60, "ランダムな無敵化" }, { 61, "ランダムなMPからHPへの変換" }, { 62, "ランダムなHPからMPへの変換" }, { 63, "ランダムな武器落とし" } },
    // パッシブ特性 (64-95)
    {
        { 64, "超人的な力" }, { 65, "虚弱" }, { 66, "生体コンピュータ" }, { 67, "精神薄弱" },
        { 68, "弾力のある体" }, { 69, "異常な肥満" }, { 70, "アルビノ" }, { 71, "腐敗した肉体" },
        { 72, "間抜けなキーキー声" }, { 73, "のっぺらぼう" }, { 74, "幻影に覆われた体" }, { 75, "第三の目" },
        { 76, "魔法防御" }, { 77, "騒音" }, { 78, "赤外線視力" }, { 79, "追加の脚" },
        { 80, "短い脚" }, { 81, "電撃オーラ" }, { 82, "火炎オーラ" }, { 83, "イボ肌" },
        { 84, "鱗肌" }, { 85, "鉄の肌" }, { 86, "翼" }, { 87, "恐れ知らず" },
        { 88, "急回復" }, { 89, "テレパシー" }, { 90, "しなやかな肉体" }, { 91, "関節の痛み" },
        { 92, "黒いオーラ(不運)" }, { 93, "元素攻撃弱点" }, { 94, "正確で力強い動作" }, { 95, "白いオーラ(幸運)" } },
    // 特殊特性 (96+)
    {
        { 96, "脱糞癖" }, { 97, "ゼEROウイルス感染" }, { 98, "同性愛" }, { 99, "両性愛" },
        { 100, "貧弱な下半身" }, { 101, "イキすぎ" }, { 102, "クッソ汚い輩を引きつける" }, { 103, "変質者を引きつける" },
        { 104, "肛門を完全破壊された" }, { 105, "頭部を失った" } }
};

static const std::vector<std::string> category_names = {
    "アクティブ能力",
    "ランダム効果",
    "パッシブ特性",
    "特殊特性"
};

static void display_mutation_list()
{
    for (auto y = 1U; y <= 8; y++) {
        term_erase(14, y, 64);
    }

    int r = 1;
    int c = 15;
    put_str("g) 突然変異を獲得 (一覧表示)", r++, c);
    put_str("l) 突然変異を喪失 (一覧表示)", r++, c);
    put_str("a) 全ての突然変異を獲得", r++, c);
    put_str("r) 全ての突然変異を削除", r++, c);
    put_str("", r++, c);
    put_str("ESC) 終了", r++, c);
}



static void display_mutations_for_selection(PlayerType *player_ptr)
{
    // Clear the entire screen area properly
    for (auto i = 0; i <= 23; i++) {
        term_erase(0, i, 80);
    }

    prt("突然変異一覧 (白:所持 灰:未所持)", 0, 0);
    
    int display_row = 1;
    int mutations_displayed = 0;
    const int mutations_per_row = 4; // 4 columns for more compact display
    
    // Collect all mutations in a single list for compact display
    std::vector<std::tuple<int, std::string, bool>> all_mutations;
    
    for (auto cat_idx = 0; cat_idx < static_cast<int>(MutationCategory::MAX); cat_idx++) {
        const auto &category = mutation_categories[cat_idx];
        
        for (const auto &[id, name] : category) {
            const auto muta_type = i2enum<PlayerMutationType>(id);
            const bool has_mutation = player_ptr->muta.has(muta_type);
            all_mutations.emplace_back(id, name, has_mutation);
        }
    }
    
    // Display all mutations in ultra-compact 4-column format
    const int mutations_per_screen = 88; // 22 rows * 4 columns = 88 mutations per screen
    const int total_mutations = all_mutations.size();
    
    for (int page = 0; page * mutations_per_screen < total_mutations; page++) {
        if (page > 0) {
            // Clear screen for next page
            for (auto i = 1; i <= 23; i++) {
                term_erase(0, i, 80);
            }
            prt(format("突然変異一覧 (白:所持 灰:未所持) - ページ %d", page + 1), 0, 0);
        }
        
        display_row = 1;
        const int page_start = page * mutations_per_screen;
        const int page_end = std::min(page_start + mutations_per_screen, total_mutations);
        
        for (int i = page_start; i < page_end; i++) {
            const auto &[id, name, has_mutation] = all_mutations[i];
            
            // Very compact truncation - 15 chars max
            auto truncated_name = name.length() > 15 ? name.substr(0, 15) : name;
            const auto display_text = format("%02d:%-15.15s", id, truncated_name.c_str());
            
            // Use color: white for owned, dark gray for unowned
            const auto color = has_mutation ? TERM_WHITE : TERM_SLATE;
            
            // Position in 4-column layout with tight spacing
            const int local_index = i - page_start;
            const int col = (local_index % mutations_per_row) * 19; // 19 chars per column
            const int row = display_row + (local_index / mutations_per_row);
            
            if (row <= 22) {
                c_put_str(color, display_text, row, col);
                mutations_displayed++;
            }
        }
        
        // If there are more pages, wait for key press
        if (page_end < total_mutations) {
            prt("-- スペースキーで次のページ、ESCで終了 --", 23, 0);
            const auto key = inkey();
            if (key == ESCAPE) {
                break;
            }
        }
    }
    
    // Show final count
    prt(format("全表示: %d/%d 個の突然変異", mutations_displayed, total_mutations), 23, 0);
}

void wiz_mutation_menu(PlayerType *player_ptr)
{
    while (true) {
        screen_save();
        display_mutation_list();

        const auto command = input_command("Mutation Command: ");
        const auto cmd = command.value_or(ESCAPE);
        screen_load();

        switch (cmd) {
        case ESCAPE:
        case ' ':
        case '\n':
        case '\r':
            return;

        case 'g':
        case 'G': {
            screen_save();
            display_mutations_for_selection(player_ptr);
            const auto mutation_id = input_numerics("獲得する突然変異ID", 0, static_cast<int>(PlayerMutationType::MAX) - 1, 0);
            screen_load();
            if (mutation_id.has_value()) {
                gain_mutation(player_ptr, mutation_id.value());
                msg_print("突然変異を獲得しました。");
            }
            break;
        }

        case 'l':
        case 'L': {
            screen_save();
            display_mutations_for_selection(player_ptr);
            const auto mutation_id = input_numerics("喪失する突然変異ID", 0, static_cast<int>(PlayerMutationType::MAX) - 1, 0);
            screen_load();
            if (mutation_id.has_value()) {
                lose_mutation(player_ptr, mutation_id.value());
                msg_print("突然変異を喪失しました。");
            }
            break;
        }

        case 'a':
        case 'A': {
            for (int i = 0; i < static_cast<int>(PlayerMutationType::MAX); i++) {
                gain_mutation(player_ptr, i);
            }
            msg_print("全ての突然変異を獲得しました。");
            break;
        }

        case 'r':
        case 'R': {
            lose_all_mutations(player_ptr);
            msg_print("全ての突然変異を削除しました。");
            break;
        }

        default:
            msg_print("That is not a valid debug command.");
            break;
        }
    }
}