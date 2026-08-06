#pragma once

class CreatureEntity;
class StoneOfLore {
public:
    StoneOfLore(CreatureEntity &creature);
    virtual ~StoneOfLore() = default;
    bool perilous_secrets();

private:
    CreatureEntity *creature_ptr;

    void consume_mp();
};
