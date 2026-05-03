#pragma once

#include "util/point-2d.h"
#include <memory>

class ItemEntity;
class CreatureEntity;
std::shared_ptr<ItemEntity> choose_warning_item(CreatureEntity &creature);
bool process_warning(CreatureEntity &creature, const Pos2D &pos);
