#pragma once

#include <string>

extern std::string screen_dump;

#ifdef WORLD_SCORE

class CreatureEntity;
bool report_score(CreatureEntity &creature);
std::string make_screen_dump(CreatureEntity &creature);
#endif
