#pragma once

#include "room/room-types.h"
#include <array>
#include <cstdint>
#include <map>

/* Room type information */
struct room_info_type {
    std::array<short, ROOM_TYPE_MAX> prob; /* Allocation information. */
    uint8_t min_level; /* Minimum level on which room can appear. */
};

extern const std::map<RoomType, room_info_type> room_info_normal;
extern const std::array<RoomType, ROOM_TYPE_MAX> room_build_order;
