#pragma once

#include "spell/spells-util.h"
#include "system/angband.h"
#include <memory>

class Direction;
class ItemEntity;
class CreatureEntity;
bool heal_monster(CreatureEntity &creature, const Direction &dir, int dam);
bool speed_monster(CreatureEntity &creature, const Direction &dir, int power);
bool slow_monster(CreatureEntity &creature, const Direction &dir, int power);
bool sleep_monster(CreatureEntity &creature, const Direction &dir, int power);
bool stasis_monster(CreatureEntity &creature, const Direction &dir); /* Like sleep, affects undead as well */
bool stasis_evil(CreatureEntity &creature, const Direction &dir); /* Like sleep, affects undead as well */
bool confuse_monster(CreatureEntity &creature, const Direction &dir, PLAYER_LEVEL plev);
bool stun_monster(CreatureEntity &creature, const Direction &dir, PLAYER_LEVEL plev);
bool fear_monster(CreatureEntity &creature, const Direction &dir, PLAYER_LEVEL plev);
bool poly_monster(CreatureEntity &creature, const Direction &dir, int power);
bool clone_monster(CreatureEntity &creature, const Direction &dir);
bool time_walk(CreatureEntity &creature);
void roll_hitdice(CreatureEntity &creature, spell_operation options);
bool life_stream(CreatureEntity &creature, bool message, bool virtue_change);
bool heroism(CreatureEntity &creature, int base);
bool berserk(CreatureEntity &creature, int base);
bool cure_light_wounds(CreatureEntity &creature, int pow);
bool cure_serious_wounds(CreatureEntity &creature, int pow);
bool cure_critical_wounds(CreatureEntity &creature, int pow);
bool true_healing(CreatureEntity &creature, int pow);
bool restore_mana(CreatureEntity &creature, bool magic_eater);
bool restore_all_status(CreatureEntity &creature);

bool fishing(CreatureEntity &creature);
std::shared_ptr<ItemEntity> cosmic_cast_off(CreatureEntity &creature, const ItemEntity &item);
void apply_nexus(const CreatureEntity &attacker, CreatureEntity &creature);
void status_shuffle(CreatureEntity &creature);
