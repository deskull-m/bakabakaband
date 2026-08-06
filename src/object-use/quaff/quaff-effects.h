#pragma once

class CreatureEntity;
class ItemEntity;
class QuaffEffects {
public:
    QuaffEffects(CreatureEntity &creature);
    QuaffEffects(const QuaffEffects &) = default;
    QuaffEffects(QuaffEffects &&) = default;
    QuaffEffects &operator=(const QuaffEffects &) = delete;
    QuaffEffects &operator=(QuaffEffects &&) = delete;

    bool influence(const ItemEntity &item, const bool is_rectal);

private:
    CreatureEntity &creature;

    bool salt_water();
    bool poison();
    bool blindness();
    bool booze();
    bool sleep();
    bool lose_memories();
    bool ruination();
    bool detonation();
    bool death();
    bool speed();
    bool augmentation();
    bool enlightenment();
    bool star_enlightenment();
    bool experience();
    bool resistance();
    bool new_life();
    bool neo_tsuyoshi();
    bool tsuyoshi();
    bool polymorph();
    bool mesudachi();
};
