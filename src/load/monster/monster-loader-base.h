#pragma once

class CreatureEntity;
class MonsterLoaderBase {
public:
    virtual ~MonsterLoaderBase() = default;
    virtual void rd_monster(CreatureEntity &monster) = 0;

protected:
    MonsterLoaderBase() = default;
};
