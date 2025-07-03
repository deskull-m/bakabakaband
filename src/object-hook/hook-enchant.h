#pragma once

typedef struct object_type object_type;
bool object_is_fixed_artifact(const object_type *o_ptr);
bool object_is_ego(const object_type *o_ptr);
bool object_is_rare(const object_type *o_ptr);
bool object_is_artifact(const object_type *o_ptr);
bool object_is_random_artifact(const object_type *o_ptr);
bool object_is_nameless(const object_type *o_ptr);
