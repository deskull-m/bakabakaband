#pragma once

#include "load/monster/monster-loader-base.h"

class MonsterLoader50 : public MonsterLoaderBase {
public:
    void rd_monster(CreatureEntity &monster) override;
};
