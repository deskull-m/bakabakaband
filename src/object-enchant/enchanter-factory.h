#pragma once

#include <memory>

class CreatureEntity;
class EnchanterBase;
class ItemEntity;
class EnchanterFactory {
public:
    static std::unique_ptr<EnchanterBase> create_enchanter(CreatureEntity &creature, ItemEntity *o_ptr, int lev, int power);

private:
    EnchanterFactory() = delete;
};
