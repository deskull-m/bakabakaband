#pragma once

#include <string>

class CreatureEntity;
class ItemUseChecker {
public:
    ItemUseChecker(CreatureEntity &creature);
    ItemUseChecker(const ItemUseChecker &) = default;
    ItemUseChecker(ItemUseChecker &&) = default;
    ItemUseChecker &operator=(const ItemUseChecker &) = delete;
    ItemUseChecker &operator=(ItemUseChecker &&) = delete;
    virtual ~ItemUseChecker() = default;

    bool check_stun(std::string_view mes) const;

private:
    CreatureEntity &creature;
};
