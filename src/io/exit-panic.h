#pragma once
/*
 * Windowsのコードからは呼ばれない。よってVSからは見えない
 */

#include "system/angband.h"

class CreatureEntity;
void exit_game_panic(CreatureEntity &creature);
