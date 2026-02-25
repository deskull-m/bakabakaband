#pragma once

#include "util/point-2d.h"
#include <tl/optional.hpp>

enum class DoorKind;
class CreatureEntity;
void add_door(CreatureEntity &creature, const Pos2D &pos);
void place_secret_door(CreatureEntity &creature, const Pos2D &pos, tl::optional<DoorKind> door_kind_initial = tl::nullopt);
void place_locked_door(CreatureEntity &creature, const Pos2D &pos);
void place_random_door(CreatureEntity &creature, const Pos2D &pos, bool is_room_door);
void place_closed_door(CreatureEntity &creature, const Pos2D &pos, DoorKind door_kind);
