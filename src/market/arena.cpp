/*!
 * @brief アリーナのUI処理
 * @author Hourier
 * @date 2024/06/22
 */

#include "market/arena.h"
#include "core/asking-player.h"
#include "core/show-file.h"
#include "core/stuff-handler.h"
#include "floor/floor-mode-changer.h"
#include "market/arena-entry.h"
#include "market/building-actions-table.h"
#include "market/building-util.h"
#include "player-base/player-class.h"
#include "status/buff-setter.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "tracking/lore-tracker.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <tl/optional.hpp>

/*!
 * @brief 優勝時のメッセージを表示し、賞金を与える
 * @param creature クリーチャーへの参照
 * @return まだ優勝していないか、挑戦者モンスターとの戦いではFALSE
 */
static tl::optional<int> process_ostensible_arena_victory()
{
    auto &entries = ArenaEntryList::get_instance();
    if (!entries.is_player_victor()) {
        return tl::nullopt;
    }

    clear_bldg(5, 19);
    prt(_("アリーナの優勝者！", "               Arena Victor!"), 5, 0);
    prt(_("おめでとう！あなたは全ての敵を倒しました。", "Congratulations!  You have defeated all before you."), 7, 0);
    prt(_("賞金として $1,000,000 が与えられます。", "For that, receive the prize: 1,000,000 gold pieces"), 8, 0);

    prt("", 10, 0);
    prt("", 11, 0);
    msg_print(_("スペースキーで続行", "Press the space bar to continue"));
    msg_erase();
    entries.increment_entry();
    return 1000000;
}

static bool check_battle_metal_babble(CreatureEntity &creature)
{
    msg_print(_("最強の挑戦者が君に決闘を申し込んできた。", "The strongest challenger throws down the gauntlet to your feet."));
    msg_erase();
    if (!input_check(_("受けて立つかね？", "Do you take up the gauntlet? "))) {
        msg_print(_("失望したよ。", "We are disappointed."));
        return false;
    }

    msg_print(_("挑戦者「死ぬがよい。」", "The challenger says, 'Die, maggots.'"));
    msg_erase();

    AngbandWorld::get_instance().set_arena(false);
    reset_tim_flags(creature);
    FloorChangeModesStore::get_instace()->set(FloorChangeMode::SAVE_FLOORS);
    creature.get_floor()->inside_arena = true;
    creature.leaving = true;
    return true;
}

/*!
 * @brief アリーナへの入場処理
 * @param creature クリーチャーへの参照
 * @return アリーナへ入場するか否か
 */
static bool go_to_arena(CreatureEntity &creature)
{
    const auto prize_money = process_ostensible_arena_victory();
    if (prize_money) {
        creature.add_au(*prize_money);
        return false;
    }

    const auto arena_record = ArenaEntryList::get_instance().check_arena_record();
    if (arena_record == ArenaRecord::METAL_BABBLE) {
        msg_print(_("あなたはアリーナに入り、しばらくの間栄光にひたった。", "You enter the arena briefly and bask in your glory."));
        msg_erase();
        return false;
    }

    if ((arena_record == ArenaRecord::POWER_WYRM) && !check_battle_metal_babble(creature)) {
        return false;
    }

    if (creature.get_riding() && !CreatureClass(creature).is_tamer()) {
        msg_print(_("ペットに乗ったままではアリーナへ入れさせてもらえなかった。", "You don't have permission to enter with pet."));
        msg_erase();
        return false;
    }

    AngbandWorld::get_instance().set_arena(false);
    reset_tim_flags(creature);
    FloorChangeModesStore::get_instace()->set(FloorChangeMode::SAVE_FLOORS);
    creature.get_floor()->inside_arena = true;
    creature.leaving = true;
    return true;
}

/*!
 * @brief アリーナ受付のコマンド処理
 * @param creature クリーチャーへの参照
 * @param cmd アリーナ処理のID
 */
bool arena_comm(CreatureEntity &creature, int cmd)
{
    switch (cmd) {
    case BACT_ARENA:
        return go_to_arena(creature);
    case BACT_POSTER: {
        const auto &entries = ArenaEntryList::get_instance();
        msg_print(entries.get_poster_message());
        if (entries.is_player_victor() || entries.is_player_true_victor()) {
            return false;
        }

        const auto &monrace = entries.get_monrace();
        LoreTracker::get_instance().set_trackee(monrace.idx);
        handle_stuff(creature);
        return false;
    }
    case BACT_ARENA_RULES:
        screen_save();
        FileDisplayer(creature.name).display(true, _("arena_j.txt", "arena.txt"), 0, 0);
        screen_load();
        return false;
    default:
        THROW_EXCEPTION(std::logic_error, "Invalid building action is specified!");
    }
}
