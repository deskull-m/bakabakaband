#pragma once

class CreatureEntity;
class BodyImprovement {
public:
    BodyImprovement(CreatureEntity &creature);

    bool has_effect() const;
    void mod_protection(short v, bool is_decrease = false);
    void set_protection(short v, bool is_decrease = false);

private:
    CreatureEntity *creature_ptr;
    bool is_affected = false;
};

bool set_invuln(CreatureEntity &creature, short v, bool do_dec);
bool set_tim_regen(CreatureEntity &creature, short v, bool do_dec);
bool set_tim_reflect(CreatureEntity &creature, short v, bool do_dec);
bool set_pass_wall(CreatureEntity &creature, short v, bool do_dec);
bool set_tim_emission(CreatureEntity &creature, short v, bool do_dec);
bool set_tim_exorcism(CreatureEntity &creature, short v, bool do_dec);
