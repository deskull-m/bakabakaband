#pragma once

#include <string>

class CreatureEntity;
enum class MindKindType;

std::string mindcraft_info(CreatureEntity &creature, MindKindType use_mind, int power);
