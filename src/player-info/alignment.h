#pragma once

#include "system/angband.h"
#include <string>

class CreatureEntity;
class PlayerAlignment {
public:
    PlayerAlignment(CreatureEntity &creature);
    virtual ~PlayerAlignment() = default;
    std::string get_alignment_description(bool with_value = false);
    void update_alignment();

private:
    CreatureEntity *creature_ptr;
    std::string alignment_label() const;
    void bias_good_alignment(int value);
    void bias_evil_alignment(int value);
    void reset_alignment();
};
