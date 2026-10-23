#pragma once

#include "info-reader/parse-error-types.h"
#include "system/angband.h"
#include <nlohmann/json_fwd.hpp>
#include <string_view>

class DungeonDefinition;

class DungeonReader {
public:
    explicit DungeonReader(nlohmann::json &dungeon_data);
    DungeonReader(nlohmann::json &&) = delete;
    DungeonReader(const DungeonReader &) = delete;
    DungeonReader(DungeonReader &&) = delete;
    DungeonReader &operator=(const DungeonReader &) = delete;
    DungeonReader &operator=(DungeonReader &&) = delete;

    parse_error_type read();

private:
    bool grab_one_dungeon_flag(DungeonDefinition &dungeon, std::string_view what);
    bool grab_one_basic_monster_flag(DungeonDefinition &dungeon, std::string_view what);
    bool grab_one_spell_monster_flag(DungeonDefinition &dungeon, std::string_view what);
    parse_error_type parse_dungeons_info(std::string_view buf);
    parse_error_type set_dungeon_description(DungeonDefinition &dungeon, const nlohmann::json &desc_obj);
    parse_error_type set_dungeon_generation(DungeonDefinition &dungeon, const nlohmann::json &gen_obj);
    parse_error_type set_dungeon_floor(DungeonDefinition &dungeon, const nlohmann::json &floor_obj);
    parse_error_type set_dungeon_wall(DungeonDefinition &dungeon, const nlohmann::json &wall_obj);
    parse_error_type set_dungeon_final_floor(DungeonDefinition &dungeon, const nlohmann::json &final_floor_obj);
    parse_error_type set_dungeon_feature_flags(DungeonDefinition &dungeon, const nlohmann::json &dungeon_json);
    parse_error_type set_dungeon_monster_flags(DungeonDefinition &dungeon, const nlohmann::json &flags_array);
    parse_error_type set_dungeon_monster_symbols(DungeonDefinition &dungeon, const nlohmann::json &symbols_array);
    parse_error_type set_dungeon_monster_spells(DungeonDefinition &dungeon, const nlohmann::json &flags_array);
    parse_error_type set_dungeon_specific_items(DungeonDefinition &dungeon, const nlohmann::json &items_array);
    parse_error_type set_dungeon_specific_vaults(DungeonDefinition &dungeon, const nlohmann::json &vaults_array);
    parse_error_type set_dungeon_room_rates(DungeonDefinition &dungeon, const nlohmann::json &rates_array);
    void set_dungeon_position(DungeonDefinition &dungeon, const nlohmann::json &pos_obj);

    nlohmann::json &dungeon_data;
};
