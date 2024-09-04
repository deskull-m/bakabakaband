#include "market/arena.h"
#include "core/asking-player.h"
#include "core/show-file.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "floor/floor-mode-changer.h"
#include "io/input-key-acceptor.h"
#include "main/sound-of-music.h"
#include "market/arena-entry.h"
#include "market/building-actions-table.h"
#include "market/building-util.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/race-flags-resistance.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "player-base/player-class.h"
#include "status/buff-setter.h"
#include "system/angband-exceptions.h"
#include "system/angband-system.h"
#include "system/building-type-definition.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "tracking/lore-tracker.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <algorithm>
#include <numeric>
#include <optional>

/*!
 * @brief 優勝時のメッセージを表示し、賞金を与える
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return まだ優勝していないか、挑戦者モンスターとの戦いではFALSE
 */
static std::optional<int> process_ostensible_arena_victory()
{
    auto &entries = ArenaEntryList::get_instance();
    if (!entries.is_player_victor()) {
        return std::nullopt;
    }

    clear_bldg(5, 19);
    prt(_("アリーナの優勝者！", "               Arena Victor!"), 5, 0);
    prt(_("おめでとう！あなたは全ての敵を倒しました。", "Congratulations!  You have defeated all before you."), 7, 0);
    prt(_("賞金として $1,000,000 が与えられます。", "For that, receive the prize: 1,000,000 gold pieces"), 8, 0);

    prt("", 10, 0);
    prt("", 11, 0);
    msg_print(_("スペースキーで続行", "Press the space bar to continue"));
    msg_print(nullptr);
    entries.increment_entry();
    return 1000000;
}

static bool check_battle_metal_babble(PlayerType *player_ptr)
{
    msg_print(_("最強の挑戦者が君に決闘を申し込んできた。", "The strongest challenger throws down the gauntlet to your feet."));
    msg_print(nullptr);
    if (!input_check(_("受けて立つかね？", "Do you take up the gauntlet? "))) {
        msg_print(_("失望したよ。", "We are disappointed."));
        return false;
    }

    msg_print(_("挑戦者「死ぬがよい。」", "The challenger says, 'Die, maggots.'"));
    msg_print(nullptr);

    AngbandWorld::get_instance().set_arena(false);
    reset_tim_flags(player_ptr);
    FloorChangeModesStore::get_instace()->set(FloorChangeMode::SAVE_FLOORS);
    player_ptr->current_floor_ptr->inside_arena = true;
    player_ptr->leaving = true;
    return true;
}

/*!
 * @brief アリーナへの入場処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return アリーナへ入場するか否か
 */
static bool go_to_arena(PlayerType *player_ptr)
{
    const auto prize_money = process_ostensible_arena_victory();
    if (prize_money) {
        player_ptr->au += *prize_money;
        return false;
    }

    const auto arena_record = ArenaEntryList::get_instance().check_arena_record();
    if (arena_record == ArenaRecord::METAL_BABBLE) {
        msg_print(_("あなたはアリーナに入り、しばらくの間栄光にひたった。", "You enter the arena briefly and bask in your glory."));
        msg_print(nullptr);
        return false;
    }

    if ((arena_record == ArenaRecord::POWER_WYRM) && !check_battle_metal_babble(player_ptr)) {
        return false;
    }

    if (player_ptr->riding && !PlayerClass(player_ptr).is_tamer()) {
        msg_print(_("ペットに乗ったままではアリーナへ入れさせてもらえなかった。", "You don't have permission to enter with pet."));
        msg_print(nullptr);
        return false;
    }

    AngbandWorld::get_instance().set_arena(false);
    reset_tim_flags(player_ptr);
    FloorChangeModesStore::get_instace()->set(FloorChangeMode::SAVE_FLOORS);
    player_ptr->current_floor_ptr->inside_arena = true;
    player_ptr->leaving = true;
    return true;
}

/*!
 * @brief 闘技場に入るコマンドの処理 / on_defeat_arena_monster commands
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param cmd 闘技場処理のID
 */
bool arena_comm(PlayerType *player_ptr, int cmd)
{
    switch (cmd) {
    case BACT_ARENA:
        return go_to_arena(player_ptr);
    case BACT_POSTER: {
        const auto &entries = ArenaEntryList::get_instance();
        msg_print(entries.get_poster_message());
        if (entries.is_player_victor() || entries.is_player_true_victor()) {
            return false;
        }

        const auto &monrace = entries.get_monrace();
        LoreTracker::get_instance().set_trackee(monrace.idx);
        handle_stuff(player_ptr);
        return false;
    }
    case BACT_ARENA_RULES:
        screen_save();
        FileDisplayer(player_ptr->name).display(true, _("arena_j.txt", "arena.txt"), 0, 0);
        screen_load();
        return false;
    default:
        THROW_EXCEPTION(std::logic_error, "Invalid building action is specified!");
    }
}

/*!
 * @brief モンスター闘技場に参加するモンスターを更新する。
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void update_gambling_monsters(PlayerType *player_ptr)
{
    int total, i;
    int max_dl = 0;
    int mon_level;
    int power[4];
    bool tekitou;

    for (const auto &d_ref : dungeons_info) {
        if (max_dl < max_dlv[d_ref.idx]) {
            max_dl = max_dlv[d_ref.idx];
        }
    }

    mon_level = randint1(std::min(max_dl, 122)) + 5;
    if (randint0(100) < 60) {
        i = randint1(std::min(max_dl, 122)) + 5;
        mon_level = std::max(i, mon_level);
    }

    if (randint0(100) < 30) {
        i = randint1(std::min(max_dl, 122)) + 5;
        mon_level = std::max(i, mon_level);
    }

    const auto &monraces = MonraceList::get_instance();
    while (true) {
        total = 0;
        tekitou = false;
        for (i = 0; i < 4; i++) {
            MonsterRaceId monrace_id;
            int j;
            while (true) {
                get_mon_num_prep(player_ptr, monster_can_entry_arena, nullptr);
                monrace_id = get_mon_num(player_ptr, 0, mon_level, PM_ARENA);
                if (!MonraceList::is_valid(monrace_id)) {
                    continue;
                }

                const auto &monrace = monraces.get_monrace(monrace_id);
                if (monrace.kind_flags.has(MonsterKindType::UNIQUE) || monrace.population_flags.has(MonsterPopulationType::ONLY_ONE)) {
                    if ((monrace.level + 10) > mon_level) {
                        continue;
                    }
                }

                for (j = 0; j < i; j++) {
                    if (monrace_id == battle_mon_list[j]) {
                        break;
                    }
                }
                if (j < i) {
                    continue;
                }

                break;
            }

            battle_mon_list[i] = monrace_id;
            if (monraces.get_monrace(monrace_id).level < 45) {
                tekitou = true;
            }
        }

        std::transform(std::begin(battle_mon_list), std::end(battle_mon_list), std::begin(power),
            [&monraces](auto monrace_id) { return monraces.get_monrace(monrace_id).calc_power(); });
        total += std::reduce(std::begin(power), std::end(power));

        for (i = 0; i < 4; i++) {
            if (power[i] <= 0) {
                break;
            }
            power[i] = total * 60 / power[i];
            if (tekitou && ((power[i] < 160) || power[i] > 1500)) {
                break;
            }
            if ((power[i] < 160) && randint0(20)) {
                break;
            }
            if (power[i] < 101) {
                power[i] = 100 + randint1(5);
            }
            mon_odds[i] = power[i];
        }

        if (i == 4) {
            break;
        }
    }
}

/*!
 * @brief モンスター闘技場のメインルーチン
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 賭けを開始したか否か
 */
bool monster_arena_comm(PlayerType *player_ptr)
{
    auto &world = AngbandWorld::get_instance();
    if ((world.game_turn - world.arena_start_turn) > TURNS_PER_TICK * 250) {
        update_gambling_monsters(player_ptr);
        world.arena_start_turn = world.game_turn;
    }

    screen_save();

    /* No money */
    if (player_ptr->au <= 1) {
        msg_print(_("おい！おまえ一文なしじゃないか！こっから出ていけ！", "Hey! You don't have gold - get out of here!"));
        msg_print(nullptr);
        screen_load();
        return false;
    }

    clear_bldg(4, 10);

    prt(_("モンスター                                                     倍率", "Monsters                                                       Odds"), 4, 4);
    for (auto i = 0; i < 4; i++) {
        const auto &monrace = monraces_info[battle_mon_list[i]];
        std::string name;
        if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
            name = _(monrace.name, "Fake ");
            name.append(_("もどき", monrace.name));
        } else {
            name = monrace.name;
            name.append(_("      ", ""));
        }

        constexpr auto fmt = _("%d) %-58s  %4ld.%02ld倍", "%d) %-58s  %4ld.%02ld");
        prt(format(fmt, i + 1, name.data(), (long int)mon_odds[i] / 100, (long int)mon_odds[i] % 100), 5 + i, 1);
    }

    prt(_("どれに賭けますか:", "Which monster: "), 0, 0);
    while (true) {
        int i = inkey();

        if (i == ESCAPE) {
            screen_load();
            return false;
        }

        if (i >= '1' && i <= '4') {
            sel_monster = i - '1';
            battle_odds = mon_odds[sel_monster];
            break;
        }

        else {
            bell();
        }
    }

    clear_bldg(4, 4);
    for (int i = 0; i < 4; i++) {
        if (i != sel_monster) {
            clear_bldg(i + 5, i + 5);
        }
    }

    auto maxbet = player_ptr->lev * 200;
    maxbet = std::min(maxbet, player_ptr->au);
    constexpr auto prompt = _("賭け金？", "Your wager? ");
    const auto wager_opt = input_integer(prompt, 1, maxbet, 1);
    if (!wager_opt.has_value()) {
        screen_load();
        return false;
    }

    auto wager = wager_opt.value();
    if (wager > player_ptr->au) {
        msg_print(_("おい！金が足りないじゃないか！出ていけ！", "Hey! You don't have the gold - get out of here!"));
        msg_print(nullptr);
        screen_load();
        return false;
    }

    msg_print(nullptr);
    battle_odds = std::max(wager + 1, wager * battle_odds / 100);
    wager_melee = wager;
    player_ptr->au -= wager;
    reset_tim_flags(player_ptr);

    FloorChangeModesStore::get_instace()->set(FloorChangeMode::SAVE_FLOORS);
    AngbandSystem::get_instance().set_phase_out(true);
    player_ptr->leaving = true;
    screen_load();
    return true;
}
