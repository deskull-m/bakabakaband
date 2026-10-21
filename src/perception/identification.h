#pragma once

#include "system/angband.h"

constexpr auto SCROBJ_FAKE_OBJECT = 0x00000001;
constexpr auto SCROBJ_FORCE_DETAIL = 0x00000002;
class CreatureEntity;
class ItemEntity;
bool screen_object(CreatureEntity &creature, const ItemEntity &item, BIT_FLAGS mode);
