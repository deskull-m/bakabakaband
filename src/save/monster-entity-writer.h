#pragma once

#include <cstdint>

class CreatureEntity;
class MonsterEntityWriter {
public:
    MonsterEntityWriter(const CreatureEntity &monster);
    MonsterEntityWriter(MonsterEntityWriter &) = delete;
    MonsterEntityWriter &operator=(const MonsterEntityWriter &) = delete;
    MonsterEntityWriter &operator=(const MonsterEntityWriter &&) = delete;
    void write_to_savedata() const;

private:
    uint32_t write_monster_flags() const;
    void write_monster_info(uint32_t flags) const;
    const CreatureEntity &monster;
};
