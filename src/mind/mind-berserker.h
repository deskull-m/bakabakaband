#pragma once

enum class MindBerserkerType : int;
class CreatureEntity;
bool cast_berserk_spell(CreatureEntity &creature, MindBerserkerType spell);
