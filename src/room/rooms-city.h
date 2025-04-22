#pragma once

#include "util/point-2d.h"
#include <vector>

/* Minimum & maximum town size */
constexpr int MIN_TOWN_WID = 15;
constexpr int MIN_TOWN_HGT = 15;
constexpr int MAX_TOWN_WID = 32;
constexpr int MAX_TOWN_HGT = 32;

class UndergroundBuilding {
public:
    UndergroundBuilding();
    Pos2DVec pick_door_direction() const;
    void set_area(int height, int width, int max_height, int max_width);
    bool is_area_used(const std::vector<std::vector<bool>> &ugarcade_used) const;
    void reserve_area(std::vector<std::vector<bool>> &ugarcade_used) const;
    Rect2D get_outer_room(const Pos2D &pos_ug) const;
    Rect2D get_inner_room(const Pos2D &pos_ug) const;

private:
    Rect2D rectangle; // 地下店舗の領域.
};

class DungeonData;
class PlayerType;
bool build_type16(PlayerType *player_ptr, DungeonData *dd_ptr);
