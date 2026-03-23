#pragma once

class CreatureEntity;
bool psychometry(CreatureEntity &creature);

enum class MindMindcrafterType : int;
bool cast_mindcrafter_spell(CreatureEntity &creature, MindMindcrafterType spell);
