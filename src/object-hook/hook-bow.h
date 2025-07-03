#pragma once

typedef struct object_type object_type;
class PlayerType;
bool object_is_convertible(const object_type *o_ptr);
bool object_is_ammo(const object_type *o_ptr);
