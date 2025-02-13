#pragma once

#include "floor/floor-base-definitions.h"
#include "util/point-2d.h"
#include <optional>
#include <string>
#include <vector>

/*
 * Bounds on some arrays used in the DungeonData.
 * These bounds are checked, though usually this is a formality.
 */
#define CENT_MAX 300 //!< 「部屋の中心」最大数/巨大マップを作るバリアントにするほど大きめにとっておかないといけないと思われる。
#define DOOR_MAX 200
#define WALL_MAX 500
#define TUNN_MAX 900

/*
 * Maximum numbers of rooms along each axis (currently 6x6)
 */
constexpr int MAX_ROOMS_ROW = (MAX_HGT / BLOCK_HGT);
constexpr int MAX_ROOMS_COL = (MAX_WID / BLOCK_WID);

/*
 * Structure to hold all "dungeon generation" data
 */
class DungeonData {
public:
    DungeonData();

    int cent_n = 0;
    std::vector<Pos2D> centers;

    int door_n = 0;
    std::vector<Pos2D> doors;

    int wall_n = 0;
    std::vector<Pos2D> walls;

    int tunn_n = 0;
    std::vector<Pos2D> tunnels;

    /* Number of blocks along each axis */
    int row_rooms = 0;
    int col_rooms = 0;

    /* Array of which blocks are used */
    std::vector<std::vector<bool>> room_map;

    /* Various type of dungeon floors */
    bool destroyed = false;
    bool empty_level = false;
    bool cavern = false;
    int laketype = 0;
    int tunnel_fail_count = 0;

    Pos2D tunnel_pos;

    int alloc_object_num = 0;
    int alloc_monster_num = 0;

    std::optional<std::string> why;
};
