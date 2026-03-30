#pragma once

#include "load/monster/monster-loader-base.h"

class MonsterEntity;
class MonsterLoader50 : public MonsterLoaderBase {
public:
    void rd_monster(MonsterEntity &monster) override;
};
