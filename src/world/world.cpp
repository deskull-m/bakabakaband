#include "world/world.h"
#include "core/asking-player.h"
#include "market/arena-entry.h"
#include "player-info/race-types.h"
#include "system/inner-game-data.h"
#include "system/monrace/monrace-list.h"
#include "term/term-color-types.h"
#include "term/z-util.h"
#include "util/bit-flags-calculator.h"
#include <algorithm>
#include <ctime>
#include <fmt/format.h>

AngbandWorld AngbandWorld::instance{};

AngbandWorld &AngbandWorld::get_instance()
{
    return instance;
}

bool AngbandWorld::is_wild_mode() const
{
    return this->wild_mode;
}

void AngbandWorld::set_wild_mode(bool new_wild_mode)
{
    this->wild_mode = new_wild_mode;
}

/*!
 * @brief アリーナへの入場/退出状態を更新する
 * @param 入場ならばtrue、退出ならばfalse
 */
void AngbandWorld::set_arena(const bool new_status)
{
    this->is_out_arena = new_status;
}

/*!
 * @brief アリーナへの入場/退出状態を取得する
 * @return アリーナ状態
 */
bool AngbandWorld::get_arena() const
{
    return this->is_out_arena;
}

/*!
 * @brief ゲーム時間が日中かどうかを返す
 * @return 日中ならばTRUE、夜ならばFALSE
 */
bool AngbandWorld::is_daytime() const
{
    int a_day = TURNS_PER_TICK * TOWN_DAWN;
    return (this->game_turn % a_day) < (a_day / 2);
}

/*!
 * @brief プレイ開始からの経過時間を計算する
 * @param start_race 開始時のプレイヤー種族
 * @return 日数、時間、分
 */
std::tuple<int, int, int> AngbandWorld::extract_date_time(PlayerRaceType start_race) const
{
    const auto a_day = TURNS_PER_TICK * TOWN_DAWN;
    const auto turn_in_today = (this->game_turn + a_day / 4) % a_day;
    int day;
    switch (start_race) {
    case PlayerRaceType::VAMPIRE:
    case PlayerRaceType::SKELETON:
    case PlayerRaceType::ZOMBIE:
    case PlayerRaceType::SPECTRE:
        day = (this->game_turn - a_day * 3 / 4) / a_day + 1;
        break;
    default:
        day = (this->game_turn + a_day / 4) / a_day + 1;
        break;
    }

    auto hour = (24 * turn_in_today / a_day) % 24;
    auto min = (1440 * turn_in_today / a_day) % 60;
    return { day, hour, min };
}

MonraceDefinition &AngbandWorld::get_today_bounty()
{
    return MonraceList::get_instance().get_monrace(this->today_mon);
}

const MonraceDefinition &AngbandWorld::get_today_bounty() const
{
    return MonraceList::get_instance().get_monrace(this->today_mon);
}

bool AngbandWorld::is_player_true_winner() const
{
    const auto &entries = ArenaEntryList::get_instance();
    return (this->total_winner > 0) && (entries.is_player_true_victor());
}

/*!
 * @brief 宿泊によってゲーム内ターンを経過させる
 */
void AngbandWorld::pass_game_turn_by_stay()
{
    const auto oldturn = this->game_turn;
    this->game_turn = (this->game_turn / (TURNS_PER_TICK * TOWN_DAWN / 2) + 1) * (TURNS_PER_TICK * TOWN_DAWN / 2);
    if (this->dungeon_turn >= this->dungeon_turn_limit) {
        return;
    }

    constexpr auto stay_magnificant = 10;
    this->dungeon_turn += std::min<int>((this->game_turn - oldturn), TURNS_PER_TICK * 250) * stay_magnificant;
    if (this->dungeon_turn > this->dungeon_turn_limit) {
        this->dungeon_turn = this->dungeon_turn_limit;
    }
}

/*!
 * @brief ゲーム開始時刻を、1 日の中のターンで返す
 * @param is_undead アンデッド系種族なら true
 * @return ゲーム開始時刻のターン (1 日目のもの)
 * @details アンデッド系種族は日中に地上へ出るとダメージを受けるため、夜から開始する.
 */
GAME_TURN AngbandWorld::get_start_turn_in_day(bool is_undead)
{
    return is_undead ? (TURNS_PER_TICK * 3 * TOWN_DAWN) / 4 + 1 : 1;
}

/*!
 * @brief 死亡からの復活によってゲーム内ターンを経過させる
 * @param is_undead アンデッド系種族なら true
 * @details 3 日後のゲーム開始時刻 (アンデッド系種族は夜) まで進める.
 * 復活した直後に、アンデッドが日中の地上へ放り出されてダメージを受けないようにするため.
 */
void AngbandWorld::pass_game_turn_by_death(bool is_undead)
{
    constexpr GAME_TURN a_day = TURNS_PER_TICK * TOWN_DAWN;
    constexpr auto days_to_revive = 3;
    const auto start_turn = get_start_turn_in_day(is_undead);

    // 現在以前で直近のゲーム開始時刻の日を基準にする (種族が変わって開始時刻より前のターンになっている場合にも備えて切り捨てる)
    auto elapsed_days = (this->game_turn - start_turn) / a_day;
    if (this->game_turn < start_turn) {
        elapsed_days = -1;
    }

    const auto revived_turn = start_turn + (elapsed_days + days_to_revive) * a_day;
    const auto turn_limit = InnerGameData::get_instance().get_game_turn_limit() - 1;
    this->game_turn = std::min<GAME_TURN>(revived_turn, turn_limit);
}

/*!
 * @brief 現実のプレイ時間をフォーマットして返す
 */
std::string AngbandWorld::format_real_playtime() const
{
    const auto playtime = this->play_time.elapsed_sec();
    const auto hour = playtime / (60 * 60);
    const auto min = (playtime / 60) % 60;
    const auto sec = playtime % 60;
    return fmt::format("{:02}:{:02}:{:02}", hour, min, sec);
}

/*!
 * @brief プレイ日数を直接変更する (デバッグ専用)
 */
void AngbandWorld::set_gametime()
{
    const auto game_time = input_integer("Dungeon Turn", 0, this->dungeon_turn_limit - 1);
    if (!game_time) {
        return;
    }

    this->dungeon_turn = this->game_turn = *game_time;
}
