#pragma once

#include "util/point-2d.h"
#include <cstdint>
#include <vector>

class PlayerType;
class ProjectionPath {
public:
    using pp_const_iterator = std::vector<Pos2D>::const_iterator;

    ProjectionPath(PlayerType *player_ptr, int range, const Pos2D &pos_src, const Pos2D &pos_dst, uint32_t flag);
    pp_const_iterator begin() const;
    pp_const_iterator end() const;
    const Pos2D &front() const;
    const Pos2D &back() const;
    const Pos2D &operator[](int num) const;
    int path_num() const;

private:
    std::vector<Pos2D> position;
};

bool projectable(PlayerType *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2);
