#pragma once

#include "term/term-color-types.h"
#include <string>
#include <tuple>

enum class PlayerStunRank;
class PlayerStun {
public:
    PlayerStun() = delete;

    static PlayerStunRank get_rank(short value);
    static std::string_view get_stun_mes(PlayerStunRank stun_rank);
    static const char *get_stun_mes_others(PlayerStunRank stun_rank);
    static short get_accumulation(int rank);
    static int get_accumulation_rank(int total, int damage);

    static int get_magic_chance_penalty(short value);
    static int get_item_chance_penalty(short value);
    static short get_damage_penalty(short value);
    static bool is_stunned(short value);
    static bool is_knocked_out(short value);
    static std::tuple<term_color_type, std::string_view> get_expr(short value);
};
