/*!
 * @file birth.c
 * @brief プレイヤーの作成を行う / Create a creature character
 * @date 2013/12/28
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 *\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2013 Deskull Doxygen向けのコメント整理\n
 */

#include "birth/character-builder.h"
#include "birth/birth-explanations-table.h"
#include "birth/birth-wizard.h"
#include "birth/game-play-initializer.h"
#include "birth/quick-start.h"
#include "core/window-redrawer.h"
#include "dungeon/quest.h"
#include "game-option/option-flags.h"
#include "info-reader/fixed-map-parser.h"
#include "io/write-diary.h"
#include "main/music-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-elementalist.h"
#include "monster-floor/monster-remover.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/class-info.h"
#include "player-info/race-types.h"
#include "player/patron.h"
#include "player/player-personality.h"
#include "player/player-realm.h"
#include "player/player-sex.h"
#include "player/race-info-table.h"
#include "store/store-owners.h"
#include "store/store.h"
#include "system/creature-entity.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/floor/floor-info.h"
#include "system/floor/town-info.h"
#include "system/floor/town-list.h"
#include "system/floor/wilderness-grid.h"
#include "system/gamevalue.h"
#include "system/redrawing-flags-updater.h"
#include "term/gameterm.h"
#include "term/z-form.h"
#include "util/enum-converter.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief プレイヤーキャラの作成結果を日記に書く
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void write_birth_diary(CreatureEntity &creature)
{
    concptr indent = "                            ";

    message_add(" ");
    message_add("  ");
    message_add("====================");
    message_add(" ");
    message_add("  ");
    const auto &floor = *creature.get_floor();
    exe_write_diary(floor, DiaryKind::GAMESTART, 1, _("-------- 新規ゲーム開始 --------", "------- Started New Game -------"));
    exe_write_diary(floor, DiaryKind::DIALY, 0);
    const auto mes_sex = format(_("%s性別に%sを選択した。", "%schose %s gender."), indent, sex_info[creature.psex].title.data());
    exe_write_diary(floor, DiaryKind::DESCRIPTION, 1, mes_sex);
    const auto mes_race = format(_("%s種族に%sを選択した。", "%schose %s race."), indent, race_info[enum2i(creature.prace)].title.data());
    exe_write_diary(floor, DiaryKind::DESCRIPTION, 1, mes_race);
    const auto mes_class = format(_("%s職業に%sを選択した。", "%schose %s class."), indent, class_info.at(creature.pclass).title.data());
    exe_write_diary(floor, DiaryKind::DESCRIPTION, 1, mes_class);
    PlayerRealm pr(creature);
    if (pr.realm1().is_available()) {
        const auto mes_realm2 = pr.realm2().is_available() ? format(_("と%s", " and %s realms"), pr.realm2().get_name().data()) : _("", " realm");
        const auto mes_realm = format(_("%s魔法の領域に%s%sを選択した。", "%schose %s%s."), indent, pr.realm1().get_name().data(), mes_realm2.data());
        exe_write_diary(floor, DiaryKind::DESCRIPTION, 1, mes_realm);
    }

    if (creature.element_realm != ElementRealmType::NONE) {
        const auto mes_element = format(_("%s元素系統に%sを選択した。", "%schose %s system."), indent, get_element_title(creature.element_realm).data());
        exe_write_diary(floor, DiaryKind::DESCRIPTION, 1, mes_element);
    }

    const auto mes_personality = format(_("%s性格に%sを選択した。", "%schose %s personality."), indent, personality_info[creature.ppersonality].title.data());
    exe_write_diary(floor, DiaryKind::DESCRIPTION, 1, mes_personality);
    if (CreatureClass(creature).equals(PlayerClassType::CHAOS_WARRIOR)) {
        const auto fmt_patron = _("%s守護神%sと契約を交わした。", "%smade a contract with patron %s.");
        const auto mes_patron = format(fmt_patron, indent, patron_list[creature.patron].name.data());
        exe_write_diary(floor, DiaryKind::DESCRIPTION, 1, mes_patron);
    }
}

/*!
 * @brief プレイヤー作成処理のメインルーチン/ Create a new character.
 * @details
 * Note that we may be called with "junk" leftover in the various
 * fields, so we must be sure to clear them first.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param initial_quest_id ゲーム開始時に突入するクエストID（オプション）
 */
void player_birth(CreatureEntity &creature, std::optional<QuestId> initial_quest_id)
{

    TermCenteredOffsetSetter tcos(MAIN_TERM_MIN_COLS, MAIN_TERM_MIN_ROWS);

    AngbandWorld::get_instance().play_time.reset();
    wipe_monsters_list(creature);
    player_wipe_without_name(creature);
    if (!ask_quick_start(creature)) {
        play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_NEW_GAME);
        while (true) {
            if (player_birth_wizard(creature)) {
                break;
            }

            player_wipe_without_name(creature);
        }
    }

    write_birth_diary(creature);
    for (size_t i = 1; i < towns_info.size(); i++) {
        for (auto sst : STORE_SALE_TYPE_LIST) {
            store_init(i, sst);
        }
    }

    WildernessGrids::get_instance().initialize_seeds();
    if (CreatureRace(&creature).equals(PlayerRaceType::BEASTMAN)) {
        creature.hack_mutation = true;
    } else {
        creature.hack_mutation = false;
    }

    if (g_window_flags[1].none()) {
        g_window_flags[1].set(SubWindowRedrawingFlag::MESSAGE);
    }

    if (g_window_flags[2].none()) {
        g_window_flags[2].set(SubWindowRedrawingFlag::INVENTORY);
    }

    // ゲーム開始時に特定のクエストに突入する処理
    if (initial_quest_id.has_value()) {
        auto &quests = QuestList::get_instance();
        auto &quest = quests.get_quest(*initial_quest_id);

        // クエストの初期化処理（ウィザードモードのwiz_enter_quest関数を参考）
        init_flags = i2enum<init_flags_type>(INIT_SHOW_TEXT | INIT_ASSIGN);
        creature.get_floor()->quest_number = *initial_quest_id;
        parse_fixed_map(creature, QUEST_DEFINITION_LIST, 0, 0, 0, 0);
        quest.status = QuestStatusType::TAKEN;

        // クエストに突入
        if (quest.dungeon == DungeonId::WILDERNESS) {
            exe_enter_quest(creature, *initial_quest_id);
        }
    }
}
