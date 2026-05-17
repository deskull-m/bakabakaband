#include "racial/racial-util.h"
#include "io/input-key-requester.h"
#include "player-base/player-class.h"
#include "system/creature-entity.h"
#include "util/enum-converter.h"

rc_type::rc_type(CreatureEntity &creature)
{
    this->ask = true;
    this->lvl = creature.get_level();
    CreatureClass pc(creature);
    this->is_warrior = pc.equals(PlayerClassType::WARRIOR) || pc.equals(PlayerClassType::BERSERKER);
}

void rc_type::add_power(rpi_type &rpi, int number)
{
    rpi.number = number;
    this->power_desc.push_back(std::move(rpi));
}

void rc_type::add_power(rpi_type &rpi, PlayerMutationType flag)
{
    add_power(rpi, enum2i(flag));
}

COMMAND_CODE rc_type::power_count()
{
    return (COMMAND_CODE)this->power_desc.size();
}
