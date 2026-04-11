/*!
 * @brief ゲーム属性を変更するデバッグコマンド
 * @date 2021/03/07
 */

#include "wizard/wizard-game-modifier.h"
#include "core/asking-player.h"
#include "dungeon/quest.h"
#include "info-reader/fixed-map-parser.h"
#include "io/input-key-requester.h"
#include "player-info/self-info.h"
#include "system/building-type-definition.h"
#include "system/creature-entity.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/system-variables.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <array>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

void wiz_enter_quest(CreatureEntity &creature);
void wiz_complete_quest(CreatureEntity &creature);
void wiz_restore_monster_max_num(MonraceId r_idx);

/*!
 * @brief ゲーム設定コマンド一覧表
 */
constexpr std::array wizard_game_modifier_menu_table = {
    std::make_tuple('t', _("プレイ時間変更", "Modify played time")),
    std::make_tuple('q', _("現在のクエストを完了", "Complete current quest")),
    std::make_tuple('Q', _("クエストに突入", "Enter quest")),
    std::make_tuple('u', _("ユニーク/ナズグルの生存数を復元", "Restore living info of unique/nazgul")),
    std::make_tuple('g', _("モンスター闘技場出場者更新", "Update gambling monster")),
};

/*!
 * @brief ゲーム設定コマンドの一覧を表示する
 */
static void display_wizard_game_modifier_menu()
{
    for (auto y = 1U; y <= wizard_game_modifier_menu_table.size(); y++) {
        term_erase(14, y, 64);
    }

    int r = 1;
    int c = 15;
    for (const auto &[symbol, desc] : wizard_game_modifier_menu_table) {
        std::stringstream ss;
        ss << symbol << ") " << desc;
        put_str(ss.str(), r++, c);
    }
}

/*!
 * @brief ゲーム設定コマンドの入力を受け付ける
 * @param creature クリーチャーへの参照
 */
void wizard_game_modifier(CreatureEntity &creature)
{
    screen_save();
    display_wizard_game_modifier_menu();

    const auto command = input_command("Player Command: ");
    const auto cmd = command.value_or(ESCAPE);
    screen_load();

    switch (cmd) {
    case ESCAPE:
    case ' ':
    case '\n':
    case '\r':
        break;
    case 'g': {
        auto &melee_arena = MeleeArena::get_instance();
        melee_arena.update_gladiators(creature);
        break;
    }
    case 'q':
        wiz_complete_quest(creature);
        break;
    case 'Q':
        wiz_enter_quest(creature);
        break;
    case 'u':
        wiz_restore_monster_max_num(i2enum<MonraceId>(command_arg));
        break;
    case 't':
        AngbandWorld::get_instance().set_gametime();
        break;
    }
}

/*!
 * @brief 指定したクエストに突入する
 * @param creature クリーチャーへの参照
 */
void wiz_enter_quest(CreatureEntity &creature)
{
    auto &quests = QuestList::get_instance();
    const auto quest_max = enum2i(quests.rbegin()->first);
    const auto quest_id = input_numerics("QuestID", 0, quest_max - 1, QuestId::NONE);
    if (!quest_id) {
        return;
    }

    init_flags = i2enum<init_flags_type>(INIT_SHOW_TEXT | INIT_ASSIGN);
    creature.get_floor()->quest_number = *quest_id;
    parse_fixed_map(creature, QUEST_DEFINITION_LIST, 0, 0, 0, 0);
    auto &quest = quests.get_quest(*quest_id);
    quest.status = QuestStatusType::TAKEN;
    if (quest.dungeon == DungeonId::WILDERNESS) {
        exe_enter_quest(creature, *quest_id);
    }
}

/*!
 * @brief 指定したクエストを完了させる
 * @param creature クリーチャーへの参照
 */
void wiz_complete_quest(CreatureEntity &creature)
{
    const auto &floor = *creature.get_floor();
    if (!floor.is_in_quest()) {
        msg_print("No current quest");
        msg_erase();
        return;
    }

    const auto &quests = QuestList::get_instance();
    if (quests.get_quest(floor.quest_number).status == QuestStatusType::TAKEN) {
        complete_quest(creature, floor.quest_number);
    }
}

void wiz_restore_monster_max_num(MonraceId monrace_id)
{
    if (!MonraceList::is_valid(monrace_id)) {
        const auto restore_monrace_id = input_numerics("MonsterID", 1, MonraceList::get_instance().size() - 1, MonraceId::FILTHY_URCHIN);
        if (!restore_monrace_id.has_value()) {
            return;
        }

        monrace_id = *restore_monrace_id;
    }
    auto &monraces = MonraceList::get_instance();
    auto &monrace = monraces.get_monrace(monrace_id);
    tl::optional<int> max_num;
    if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
        max_num = MAX_UNIQUE_NUM;
    } else if (monrace.population_flags.has(MonsterPopulationType::NAZGUL)) {
        max_num = MAX_NAZGUL_NUM;
    }

    if (monrace.max_num == 0) {
        msg_print(_("出現数に制限がないモンスターです。", "This monster can appear any time."));
        msg_erase();
        return;
    }

    monrace.mob_num = monrace.max_num;

    std::stringstream ss;
    ss << monrace.name << _("の出現数を復元しました。", " can appear again now.");
    msg_print(ss.str());
    msg_erase();
}
