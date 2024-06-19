#pragma once

#include "load/monster/monster-loader-base.h"

class MonsterEntity;
class PlayerType;
class MonsterLoader50 : public MonsterLoaderBase {
public:
    MonsterLoader50();
    void rd_monster(MonsterEntity *m_ptr) override;

private:
    MonsterEntity *m_ptr = nullptr;
};
