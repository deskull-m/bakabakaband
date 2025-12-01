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
#include "term/term-color-types.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <algorithm>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

// Map mutation types to their Japanese names using actual enum values
static const std::vector<std::pair<PlayerMutationType, std::string>> mutation_data = {
    { PlayerMutationType::SPIT_ACID, "酸の唾" },
    { PlayerMutationType::BR_FIRE, "炎のブレス" },
    { PlayerMutationType::HYPN_GAZE, "催眠睨み" },
    { PlayerMutationType::TELEKINES, "念動力" },
    { PlayerMutationType::VTELEPORT, "テレポート" },
    { PlayerMutationType::MIND_BLST, "精神攻撃" },
    { PlayerMutationType::RADIATION, "放射能" },
    { PlayerMutationType::VAMPIRISM, "吸血" },
    { PlayerMutationType::SMELL_MET, "金属嗅覚" },
    { PlayerMutationType::SMELL_MON, "敵臭嗅覚" },
    { PlayerMutationType::BLINK, "ショート・テレポート" },
    { PlayerMutationType::EAT_ROCK, "岩喰い" },
    { PlayerMutationType::SWAP_POS, "位置交換" },
    { PlayerMutationType::SHRIEK, "叫び" },
    { PlayerMutationType::ILLUMINE, "照明" },
    { PlayerMutationType::DET_CURSE, "呪い感知" },
    { PlayerMutationType::BERSERK, "狂戦士化" },
    { PlayerMutationType::POLYMORPH, "変身" },
    { PlayerMutationType::MIDAS_TCH, "ミダスの手" },
    { PlayerMutationType::GROW_MOLD, "カビ発生" },
    { PlayerMutationType::RESIST, "エレメント耐性" },
    { PlayerMutationType::EARTHQUAKE, "地震" },
    { PlayerMutationType::EAT_MAGIC, "魔力喰い" },
    { PlayerMutationType::WEIGH_MAG, "魔力感知" },
    { PlayerMutationType::STERILITY, "増殖阻止" },
    { PlayerMutationType::HIT_AND_AWAY, "ヒットアンドアウェイ" },
    { PlayerMutationType::DAZZLE, "眩惑" },
    { PlayerMutationType::LASER_EYE, "レーザー・アイ" },
    { PlayerMutationType::RECALL, "帰還" },
    { PlayerMutationType::BANISH, "邪悪消滅" },
    { PlayerMutationType::COLD_TOUCH, "凍結の手" },
    { PlayerMutationType::LAUNCHER, "アイテム投げ" },
    { PlayerMutationType::BERS_RAGE, "狂戦士化の発作" },
    { PlayerMutationType::COWARDICE, "臆病" },
    { PlayerMutationType::RTELEPORT, "ランダムテレポート" },
    { PlayerMutationType::ALCOHOL, "アルコール分泌" },
    { PlayerMutationType::HALLU, "幻覚" },
    { PlayerMutationType::FLATULENT, "猛烈な屁" },
    { PlayerMutationType::SCOR_TAIL, "サソリの尻尾" },
    { PlayerMutationType::HORNS, "ツノ" },
    { PlayerMutationType::BEAK, "クチバシ" },
    { PlayerMutationType::ATT_DEMON, "デーモンを引き付ける" },
    { PlayerMutationType::PROD_MANA, "制御できない魔力" },
    { PlayerMutationType::SPEED_FLUX, "ランダムな加減速" },
    { PlayerMutationType::BANISH_ALL, "ランダムなモンスター消滅" },
    { PlayerMutationType::EAT_LIGHT, "光源喰い" },
    { PlayerMutationType::TRUNK, "象の鼻" },
    { PlayerMutationType::ATT_ANIMAL, "動物を引き寄せる" },
    { PlayerMutationType::TENTACLES, "邪悪な触手" },
    { PlayerMutationType::RAW_CHAOS, "純カオス" },
    { PlayerMutationType::NORMALITY, "ランダムな変異の消滅" },
    { PlayerMutationType::WRAITH, "ランダムな幽体化" },
    { PlayerMutationType::POLY_WOUND, "ランダムな傷の変化" },
    { PlayerMutationType::WASTING, "衰弱" },
    { PlayerMutationType::ATT_DRAGON, "ドラゴンを引き寄せる" },
    { PlayerMutationType::WEIRD_MIND, "ランダムなテレパシー" },
    { PlayerMutationType::NAUSEA, "落ち着きの無い胃" },
    { PlayerMutationType::CHAOS_GIFT, "パトロン" },
    { PlayerMutationType::WALK_SHAD, "ランダムな現実変容" },
    { PlayerMutationType::WARNING, "警告" },
    { PlayerMutationType::INVULN, "ランダムな無敵化" },
    { PlayerMutationType::SP_TO_HP, "ランダムなMPからHPへの変換" },
    { PlayerMutationType::HP_TO_SP, "ランダムなHPからMPへの変換" },
    { PlayerMutationType::DISARM, "ランダムな武器落とし" },
    { PlayerMutationType::HYPER_STR, "超人的な力" },
    { PlayerMutationType::PUNY, "虚弱" },
    { PlayerMutationType::HYPER_INT, "生体コンピュータ" },
    { PlayerMutationType::MORONIC, "精神薄弱" },
    { PlayerMutationType::RESILIENT, "弾力のある体" },
    { PlayerMutationType::XTRA_FAT, "異常な肥満" },
    { PlayerMutationType::ALBINO, "アルビノ" },
    { PlayerMutationType::FLESH_ROT, "腐敗した肉体" },
    { PlayerMutationType::SILLY_VOI, "間抜けなキーキー声" },
    { PlayerMutationType::BLANK_FAC, "のっぺらぼう" },
    { PlayerMutationType::ILL_NORM, "幻影に覆われた体" },
    { PlayerMutationType::XTRA_EYES, "第三の目" },
    { PlayerMutationType::MAGIC_RES, "魔法防御" },
    { PlayerMutationType::XTRA_NOIS, "騒音" },
    { PlayerMutationType::INFRAVIS, "赤外線視力" },
    { PlayerMutationType::XTRA_LEGS, "追加の脚" },
    { PlayerMutationType::SHORT_LEG, "短い脚" },
    { PlayerMutationType::ELEC_TOUC, "電撃オーラ" },
    { PlayerMutationType::FIRE_BODY, "火炎オーラ" },
    { PlayerMutationType::WART_SKIN, "イボ肌" },
    { PlayerMutationType::SCALES, "鱗肌" },
    { PlayerMutationType::IRON_SKIN, "鉄の肌" },
    { PlayerMutationType::WINGS, "翼" },
    { PlayerMutationType::FEARLESS, "恐れ知らず" },
    { PlayerMutationType::REGEN, "急回復" },
    { PlayerMutationType::ESP, "テレパシー" },
    { PlayerMutationType::LIMBER, "しなやかな肉体" },
    { PlayerMutationType::ARTHRITIS, "関節の痛み" },
    { PlayerMutationType::BAD_LUCK, "黒いオーラ(不運)" },
    { PlayerMutationType::VULN_ELEM, "元素攻撃弱点" },
    { PlayerMutationType::MOTION, "正確で力強い動作" },
    { PlayerMutationType::GOOD_LUCK, "白いオーラ(幸運)" },
    { PlayerMutationType::DEFECATION, "脱糞癖" },
    { PlayerMutationType::ZEERO_VIRUS, "ゼEROウイルス感染" },
    { PlayerMutationType::HOMO_SEXUAL, "同性愛" },
    { PlayerMutationType::BI_SEXUAL, "両性愛" },
    { PlayerMutationType::WEAK_LOWER_BODY, "貧弱な下半身" },
    { PlayerMutationType::IKISUGI, "イキすぎ" },
    { PlayerMutationType::ATT_NASTY, "クッソ汚い輩を引きつける" },
    { PlayerMutationType::ATT_PERVERT, "変質者を引きつける" },
    { PlayerMutationType::DESTROYED_ASSHOLE, "肛門を完全破壊された" },
    { PlayerMutationType::LOST_HEAD, "頭部を失った" }
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

    // Collect all mutations in a single list for compact display using actual enum values
    std::vector<std::tuple<int, std::string, bool>> all_mutations;

    for (const auto &[muta_type, name] : mutation_data) {
        const int id = static_cast<int>(muta_type);
        const bool has_mutation = player_ptr->muta.has(muta_type);
        all_mutations.emplace_back(id, name, has_mutation);
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