#pragma once

enum class TerrainCharacteristics;
struct msa_type;
class CreatureEntity;
void decide_lite_range(CreatureEntity &creature, msa_type *msa_ptr);
bool decide_lite_projection(CreatureEntity &creature, msa_type *msa_ptr);
void decide_lite_area(CreatureEntity &creature, msa_type *msa_ptr);
