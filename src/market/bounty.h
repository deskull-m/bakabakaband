#pragma once

enum class MonraceId : short;
class CreatureEntity;
bool exchange_cash(CreatureEntity &creature);
void today_target();
void tsuchinoko();
void show_bounty();
void determine_daily_bounty(CreatureEntity &creature);
void determine_bounty_uniques(CreatureEntity &creature);
