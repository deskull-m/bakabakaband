#pragma once

#include "term/term-color-types.h"
#include <string>
#include <tuple>

enum class PlayerCutRank {
    NONE = 0,
    GRAZING = 1,
    LIGHT = 2,
    BAD = 3,
    NASTY = 4,
    SEVERE = 5,
    DEEP = 6,
    MORTAL = 7,
};

class PlayerCut {
public:
    PlayerCut() = delete;

    static PlayerCutRank get_rank(short value);
    static std::string get_cut_mes(PlayerCutRank stun_rank);
    static const char *get_cut_mes_others(PlayerCutRank cut_rank);
    static short get_accumulation(int total, int damage);

    static bool is_cut(short value);
    static std::tuple<term_color_type, std::string> get_expr(short value);
    static int get_damage(short value);

private:
    static int get_accumulation_rank(int total, int damage);
};
