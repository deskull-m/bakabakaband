#pragma once

#include "effect/attribute-types.h"
#include "system/angband.h"
#include <tl/optional.hpp>

class CapturedMonsterType;
class Dice;
class Direction;
class PlayerType;
class CreatureEntity;
bool fire_ball(CreatureEntity &creature, AttributeType typ, const Direction &dir, int dam, POSITION rad, tl::optional<CapturedMonsterType *> cap_mon_ptr = tl::nullopt);
bool fire_breath(CreatureEntity &creature, AttributeType typ, const Direction &dir, int dam, POSITION rad);
bool fire_rocket(CreatureEntity &creature, AttributeType typ, const Direction &dir, int dam, POSITION rad);
bool fire_ball_hide(CreatureEntity &creature, AttributeType typ, const Direction &dir, int dam, POSITION rad);
bool fire_meteor(CreatureEntity &creature, MONSTER_IDX src_idx, AttributeType typ, POSITION x, POSITION y, int dam, POSITION rad);
bool fire_bolt(CreatureEntity &creature, AttributeType typ, const Direction &dir, int dam);
bool fire_blast(CreatureEntity &creature, AttributeType typ, const Direction &dir, const Dice &dice, int num, int dev);
bool fire_beam(CreatureEntity &creature, AttributeType typ, const Direction &dir, int dam);
bool fire_bolt_or_beam(CreatureEntity &creature, PERCENTAGE prob, AttributeType typ, const Direction &dir, int dam);
bool project_hook(CreatureEntity &creature, AttributeType typ, const Direction &dir, int dam, BIT_FLAGS flg);
