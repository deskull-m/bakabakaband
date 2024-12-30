#pragma once

#include "system/h-type.h"
#include <cstdint>
#include <map>
#include <string>
#include <vector>

enum class MonraceId : int16_t;

struct vault_type {
    vault_type() = default;
    short idx = 0;

    std::string name = ""; /* Name (offset) */
    std::string text = ""; /* Text (offset) */

    uint8_t typ = 0; /* Vault type */
    int rat = 0; /* Vault rating (unused) */
    int hgt = 0; /* Vault height */
    int wid = 0; /* Vault width */
    int min_depth = 0;
    int max_depth = 999;
    int rarity = 1;
    std::map<char, FEAT_IDX> feature_list;
    std::map<char, FEAT_IDX> feature_ap_list;
    std::map<char, MonraceId> place_monster_list;
};

extern std::vector<vault_type> vaults_info;

struct dun_data_type;
class PlayerType;
bool build_type10(PlayerType *player_ptr, dun_data_type *dd_ptr);
bool build_fixed_room(PlayerType *player_ptr, dun_data_type *dd_ptr, int typ, bool more_space, int id);
void build_vault(vault_type *v_ptr, PlayerType *player_ptr, int yval, int xval, int xoffset, int yoffset, int transno);
