#pragma once

#include "target/target.h"
#include <cstdint>

enum target_type : uint32_t;
class CreatureEntity;
Target target_set(CreatureEntity &creature, target_type mode);
