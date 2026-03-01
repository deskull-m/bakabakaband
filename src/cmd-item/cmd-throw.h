#pragma once

#include "system/angband.h"

class CreatureEntity;
class PlayerType;
class ThrowCommand {
public:
    ThrowCommand(CreatureEntity &creature);
    virtual ~ThrowCommand() = default;
    bool do_cmd_throw(int mult, bool boomerang, OBJECT_IDX shuriken);

private:
    PlayerType *player_ptr;
};
