#pragma once

class CreatureEntity;

class PlayerSpellStatus {
public:
    PlayerSpellStatus(CreatureEntity &creature);

    class Realm {
    public:
        Realm(CreatureEntity &creature, bool is_realm1);

        void initialize();
        bool is_nothing_learned() const;
        bool is_learned(int spell_id) const;
        bool is_worked(int spell_id) const;
        bool is_forgotten(int spell_id) const;
        void set_learned(int spell_id, bool value = true);
        void set_worked(int spell_id, bool value = true);
        void set_forgotten(int spell_id, bool value = true);

    private:
        CreatureEntity *creature_ptr;
        bool is_realm1;
    };

    Realm realm1() const;
    Realm realm2() const;

private:
    CreatureEntity *creature_ptr;
};
