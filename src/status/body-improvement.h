#pragma once

class PlayerType;
class CreatureEntity;
class BodyImprovement {
public:
    BodyImprovement(CreatureEntity &player);

    bool has_effect() const;
    void mod_protection(short v, bool is_decrease = false);
    void set_protection(short v, bool is_decrease = false);

private:
    CreatureEntity *player_ptr;
    bool is_affected = false;
};

bool set_invuln(PlayerType *player_ptr, short v, bool do_dec);
bool set_tim_regen(PlayerType *player_ptr, short v, bool do_dec);
bool set_tim_reflect(PlayerType *player_ptr, short v, bool do_dec);
bool set_pass_wall(PlayerType *player_ptr, short v, bool do_dec);
bool set_tim_emission(PlayerType *player_ptr, short v, bool do_dec);
bool set_tim_exorcism(PlayerType *player_ptr, short v, bool do_dec);
