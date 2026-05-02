#pragma once

#include <string>

class CreatureEntity;
class ItemUseChecker {
public:
    ItemUseChecker(CreatureEntity &creature);
    ItemUseChecker(const ItemUseChecker &) = delete;
    ItemUseChecker(ItemUseChecker &&) = delete;
    ItemUseChecker &operator=(const ItemUseChecker &) = delete;
    ItemUseChecker &operator=(ItemUseChecker &&) = delete;
    virtual ~ItemUseChecker() = default;

    bool check_stun(std::string_view mes) const;

private:
    CreatureEntity &creature;
};
