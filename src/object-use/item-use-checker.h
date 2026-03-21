#pragma once

#include <string>

class CreatureEntity;
class ItemUseChecker {
public:
    ItemUseChecker(CreatureEntity &creature);
    virtual ~ItemUseChecker() = default;

    bool check_stun(std::string_view mes) const;

private:
    CreatureEntity &creature;
};
