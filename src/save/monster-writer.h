#pragma once

#include <cstdint>

class CreatureEntity;
class MonsterWriter {
public:
    MonsterWriter(const CreatureEntity &monster);
    MonsterWriter(MonsterWriter &) = delete;
    MonsterWriter &operator=(const MonsterWriter &) = delete;
    MonsterWriter &operator=(const MonsterWriter &&) = delete;
    void write_to_savedata() const;

private:
    const CreatureEntity &monster;
};
