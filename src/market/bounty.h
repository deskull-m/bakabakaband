#pragma once

enum class MonsterRaceId : short;
class PlayerType;
bool exchange_cash(PlayerType *player_ptr);
void today_target();
void tsuchinoko();
void show_bounty();
void determine_daily_bounty(PlayerType *player_ptr, bool conv_old);
void determine_bounty_uniques(PlayerType *player_ptr);
