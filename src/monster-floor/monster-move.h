#pragma once

#include "system/angband.h"
#include "util/point-2d.h"
#include <string_view>

constexpr auto BREAK_RUNE_PROTECTION = 550; /*!< 守りのルーンの強靭度 / Rune of protection resistance */
constexpr auto BREAK_RUNE_EXPLOSION = 299; /*!< 爆発のルーンの発動しやすさ / For explosive runes */

class CreatureEntity;
class Direction;
class MonsterMovementDirectionList;
class MonraceDefinition;
class PlayerType;
struct turn_flags;
void activate_explosive_rune(CreatureEntity &creature, const Pos2D &pos, const MonraceDefinition &monrace);
bool process_monster_movement(CreatureEntity &creature, turn_flags *turn_flags_ptr, const MonsterMovementDirectionList &mmdl, const Pos2D &pos, int *count);
void show_sound_message(CreatureEntity &creature, std::string_view message);
void process_sound(CreatureEntity &creature, MONSTER_IDX m_idx);
void process_speak(CreatureEntity &creature, MONSTER_IDX m_idx, POSITION oy, POSITION ox, bool aware);
