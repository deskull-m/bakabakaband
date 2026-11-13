/*!
 * @brief ウィザードモードの突然変異コマンド
 * @date 2025/12/01
 * @author AI Assistant
 */

#include "wizard/wizard-mutation.h"
#include "core/asking-player.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "mutation/mutation-data.h"
#include "mutation/mutation-flag-types.h"
#include "mutation/mutation-investor-remover.h"
#include "system/creature-entity.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/enum-converter.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <algorithm>
#include <fmt/format.h>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

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

static void display_mutations_for_selection(CreatureEntity &creature)
{
    // Clear the entire screen area properly
    for (auto i = 0; i <= 23; i++) {
        term_erase(0, i, 80);
    }

    prt("突然変異一覧 (白:所持 灰:未所持)", 0, 0);

    int display_row = 1;
    int mutations_displayed = 0;
    const int mutations_per_row = 4; // 4 columns for more compact display

    // Collect all mutations in a single list for compact display using actual enum values
    std::vector<std::tuple<int, std::string, bool>> all_mutations;

    for (auto id = 0; id < enum2i(PlayerMutationType::MAX); id++) {
        const auto muta_type = i2enum<PlayerMutationType>(id);
        const auto &name = MutationDataManager::get_mutation_name(muta_type);
        const bool present = creature.has_mutation(muta_type);
        all_mutations.emplace_back(id, name, present);
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
            prt(fmt::format("突然変異一覧 (白:所持 灰:未所持) - ページ {}", page + 1), 0, 0);
        }

        display_row = 1;
        const int page_start = page * mutations_per_screen;
        const int page_end = std::min(page_start + mutations_per_screen, total_mutations);

        for (int i = page_start; i < page_end; i++) {
            const auto &[id, name, has_mutation] = all_mutations[i];

            // Very compact truncation - 15 chars max
            auto truncated_name = name.length() > 15 ? name.substr(0, 15) : name;
            const auto display_text = fmt::format("{:02}:{:<15}", id, fmt::bytes(truncated_name));

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
    prt(fmt::format("全表示: {}/{} 個の突然変異", mutations_displayed, total_mutations), 23, 0);
}

void wiz_mutation_menu(CreatureEntity &creature)
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
            display_mutations_for_selection(creature);
            const auto mutation_id = input_numerics("獲得する突然変異ID", 0, enum2i(PlayerMutationType::MAX) - 1, 0);
            screen_load();
            if (mutation_id.has_value()) {
                gain_mutation(creature, i2enum<PlayerMutationType>(*mutation_id));
                msg_print("突然変異を獲得しました。");
            }
            break;
        }

        case 'l':
        case 'L': {
            screen_save();
            display_mutations_for_selection(creature);
            const auto mutation_id = input_numerics("喪失する突然変異ID", 0, enum2i(PlayerMutationType::MAX) - 1, 0);
            screen_load();
            if (mutation_id.has_value()) {
                lose_mutation(creature, i2enum<PlayerMutationType>(*mutation_id));
                msg_print("突然変異を喪失しました。");
            }
            break;
        }

        case 'a':
        case 'A': {
            for (auto i = 0; i < enum2i(PlayerMutationType::MAX); i++) {
                gain_mutation(creature, i2enum<PlayerMutationType>(i));
            }
            msg_print("全ての突然変異を獲得しました。");
            break;
        }

        case 'r':
        case 'R': {
            lose_all_mutations(creature);
            msg_print("全ての突然変異を削除しました。");
            break;
        }

        default:
            msg_print("That is not a valid debug command.");
            break;
        }
    }
}