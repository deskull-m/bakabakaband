#pragma once

struct building_type;
class CreatureEntity;
bool is_owner(CreatureEntity &creature, const building_type &bldg);
bool is_member(CreatureEntity &creature, const building_type &bldg);
void display_building_service(CreatureEntity &creature, const building_type &bldg);
