#pragma once

class CreatureEntity;
class ItemEntity;
class PlayerType;
bool activate_dragon_breath(CreatureEntity &creature, const ItemEntity &item);
bool activate_breath_fire(CreatureEntity &creature, const ItemEntity &item);
bool activate_breath_cold(CreatureEntity &creature, const ItemEntity &item);
