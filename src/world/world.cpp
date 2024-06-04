#include "world/world.h"
#include "player-info/race-types.h"
#include "system/player-type-definition.h"
#include "term/z-util.h"
#include "util/bit-flags-calculator.h"
#include <ctime>

AngbandWorld world;
AngbandWorld *w_ptr = &world;

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
 * @brief ゲーム時間が日中かどうかを返す /
 * Whether daytime or not
 * @return 日中ならばTRUE、夜ならばFALSE
 */
bool is_daytime(void)
{
    int32_t len = TURNS_PER_TICK * TOWN_DAWN;
    if ((w_ptr->game_turn % len) < (len / 2)) {
        return true;
    } else {
        return false;
    }
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

/*!
 * @brief 実ゲームプレイ時間を更新する
 */
void update_playtime(void)
{
    if (w_ptr->start_time != 0) {
        uint32_t tmp = (uint32_t)time(nullptr);
        w_ptr->play_time += (tmp - w_ptr->start_time);
        w_ptr->start_time = tmp;
    }
}

/*!
 * @brief 勝利したクラスを追加する
 */
void add_winner_class(PlayerClassType c)
{
    if (!w_ptr->noscore) {
        w_ptr->sf_winner.set(c);
    }
}

/*!
 * @brief 引退したクラスを追加する
 */
void add_retired_class(PlayerClassType c)
{
    if (!w_ptr->noscore) {
        w_ptr->sf_retired.set(c);
    }
}

/*!
 * @brief 勝利したクラスか判定する
 */
bool is_winner_class(PlayerClassType c)
{
    if (c == PlayerClassType::MAX) {
        return false;
    }
    return w_ptr->sf_winner.has(c);
}

/*!
 * @brief 引退したクラスか判定する
 */
bool is_retired_class(PlayerClassType c)
{
    if (c == PlayerClassType::MAX) {
        return false;
    }
    return w_ptr->sf_retired.has(c);
}
