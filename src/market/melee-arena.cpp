/*!
 * @brief モンスター闘技場関連処理
 * @author Hourier
 * @date 2024/06/22
 */

#include "market/melee-arena.h"
#include "core/asking-player.h"
#include "floor/floor-mode-changer.h"
#include "io/input-key-acceptor.h"
#include "main/sound-of-music.h"
#include "market/building-util.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race-hook.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "status/buff-setter.h"
#include "system/angband-system.h"
#include "system/building-type-definition.h"
#include "system/dungeon-info.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <algorithm>
#include <numeric>

/*!
 * @brief モンスター闘技場に参加するモンスターを更新する。
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void update_melee_gladiators(PlayerType *player_ptr)
{
    auto max_dl = 0;
    for (const auto &dungeon : dungeons_info) {
        if (max_dl < max_dlv[dungeon.idx]) {
            max_dl = max_dlv[dungeon.idx];
        }
    }

    auto mon_level = randint1(std::min(max_dl, 122)) + 5;
    if (evaluate_percent(60)) {
        auto i = randint1(std::min(max_dl, 122)) + 5;
        mon_level = std::max(i, mon_level);
    }

    if (evaluate_percent(30)) {
        auto i = randint1(std::min(max_dl, 122)) + 5;
        mon_level = std::max(i, mon_level);
    }

    const auto &monraces = MonraceList::get_instance();
    while (true) {
        auto total = 0;
        auto is_applicable = false;
        for (auto i = 0; i < NUM_GLADIATORS; i++) {
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
                is_applicable = true;
            }
        }

        std::array<int, NUM_GLADIATORS> power;
        std::transform(std::begin(battle_mon_list), std::end(battle_mon_list), std::begin(power),
            [&monraces](auto monrace_id) { return monraces.get_monrace(monrace_id).calc_power(); });
        total += std::reduce(std::begin(power), std::end(power));
        int i;
        for (i = 0; i < NUM_GLADIATORS; i++) {
            if (power[i] <= 0) {
                break;
            }

            power[i] = total * 60 / power[i];
            if (is_applicable && ((power[i] < 160) || power[i] > 1500)) {
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

        if (i == NUM_GLADIATORS) {
            break;
        }
    }
}

/*!
 * @brief モンスター闘技場のメインルーチン
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 賭けを開始したか否か
 */
bool melee_arena_comm(PlayerType *player_ptr)
{
    auto &world = AngbandWorld::get_instance();
    if ((world.game_turn - world.arena_start_turn) > TURNS_PER_TICK * 250) {
        update_melee_gladiators(player_ptr);
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
    for (auto i = 0; i < NUM_GLADIATORS; i++) {
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
            bet_number = i - '1';
            battle_odds = mon_odds[bet_number];
            break;
        }

        else {
            bell();
        }
    }

    clear_bldg(4, 4);
    for (auto i = 0; i < NUM_GLADIATORS; i++) {
        if (i != bet_number) {
            clear_bldg(i + 5, i + 5);
        }
    }

    auto maxbet = player_ptr->lev * 200;
    maxbet = std::min(maxbet, player_ptr->au);
    constexpr auto prompt = _("賭け金？", "Your wager? ");
    const auto wager = input_integer(prompt, 1, maxbet, 1);
    if (!wager) {
        screen_load();
        return false;
    }

    if (wager > player_ptr->au) {
        msg_print(_("おい！金が足りないじゃないか！出ていけ！", "Hey! You don't have the gold - get out of here!"));
        msg_print(nullptr);
        screen_load();
        return false;
    }

    msg_print(nullptr);
    battle_odds = std::max(*wager + 1, *wager * battle_odds / 100);
    wager_melee = *wager;
    player_ptr->au -= *wager;
    reset_tim_flags(player_ptr);

    FloorChangeModesStore::get_instace()->set(FloorChangeMode::SAVE_FLOORS);
    AngbandSystem::get_instance().set_phase_out(true);
    player_ptr->leaving = true;
    screen_load();
    return true;
}
