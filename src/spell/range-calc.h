#pragma once

#include "util/point-2d.h"
#include <span>
#include <utility>
#include <vector>

class CreatureEntity;
class FloorType;
class ProjectionPath;
enum class AttributeType;
bool in_disintegration_range(const FloorType &floor, const Pos2D &pos1, const Pos2D &pos2);
std::vector<std::pair<int, Pos2D>> breath_shape(CreatureEntity &creature, const ProjectionPath &path, int dist, int rad, const Pos2D &pos_source, const Pos2D &pos_target, AttributeType typ);
std::vector<std::pair<int, Pos2D>> ball_shape(CreatureEntity &creature, const Pos2D &center, int rad, AttributeType typ);
int dist_to_line(const Pos2D &pos, const Pos2D &pos_line1, const Pos2D &pos_line2);
