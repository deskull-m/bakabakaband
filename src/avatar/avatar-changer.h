#pragma once

#include "system/angband.h"

class CreatureEntity;
class AvatarChanger {
public:
    AvatarChanger(CreatureEntity &creature, const CreatureEntity &target);
    AvatarChanger &operator=(const AvatarChanger &) = delete;
    AvatarChanger &operator=(AvatarChanger &&) = delete;
    virtual ~AvatarChanger() = default;
    void change_virtue();

private:
    CreatureEntity &creature;
    const CreatureEntity *m_ptr;
    void change_virtue_non_beginner();
    void change_virtue_unique();
    void change_virtue_good_evil();
    void change_virtue_revenge();
    void change_virtue_good_animal();
    void change_virtue_wild_thief();
};
