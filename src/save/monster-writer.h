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
    uint32_t write_monster_flags() const;
    void write_monster_info(uint32_t flags) const;
    const CreatureEntity &monster;
};
