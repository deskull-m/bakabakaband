#pragma once

#include "system/angband.h"
#include <nlohmann/json.hpp>
#include <string_view>

class MonraceDefinition;

class RaceReader {
public:
    explicit RaceReader(nlohmann::json &monrace_data);
    RaceReader(nlohmann::json &&) = delete;
    RaceReader(const RaceReader &) = delete;
    RaceReader(RaceReader &&) = delete;
    RaceReader &operator=(const RaceReader &) = delete;
    RaceReader &operator=(RaceReader &&) = delete;

    errr read();

private:
    bool grab_one_basic_flag(MonraceDefinition &monrace, std::string_view what);
    bool grab_one_spell_flag(MonraceDefinition &monrace, std::string_view what);
    errr set_mon_name(const nlohmann::json &name_data, MonraceDefinition &monrace);
    errr set_mon_symbol(const nlohmann::json &symbol_data, MonraceDefinition &monrace);
    errr set_mon_speed(const nlohmann::json &speed_data, MonraceDefinition &monrace);
    errr set_mon_stat_modifiers(const nlohmann::json &stat_data, MonraceDefinition &monrace);
    errr set_mon_evolve(nlohmann::json &evolve_data, MonraceDefinition &monrace);
    errr set_mon_transform(nlohmann::json &transform_data, MonraceDefinition &monrace);
    errr set_mon_sex(const nlohmann::json &sex_data, MonraceDefinition &monrace);
    errr set_mon_personality(const nlohmann::json &personality_data, MonraceDefinition &monrace);
    errr set_mon_player_race(const nlohmann::json &race_data, MonraceDefinition &monrace);
    errr set_mon_player_class(const nlohmann::json &class_data, MonraceDefinition &monrace);
    errr set_mon_mutations(const nlohmann::json &mutations_data, MonraceDefinition &monrace);
    errr set_mon_realm_abilities(const nlohmann::json &realm_data, MonraceDefinition &monrace);
    errr set_mon_realm_abilities2(const nlohmann::json &realm_data, MonraceDefinition &monrace);
    errr set_mon_spellbook_realm(const nlohmann::json &realm_data, MonraceDefinition &monrace);
    errr set_mon_spellbook_indices(const nlohmann::json &indices_data, MonraceDefinition &monrace);
    errr set_mon_materials(const nlohmann::json &materials_data, MonraceDefinition &monrace);
    errr set_mon_body_structure(const nlohmann::json &body_data, MonraceDefinition &monrace);
    errr set_mon_extended_slots(const nlohmann::json &slots_data, MonraceDefinition &monrace);
    errr set_mon_artifacts(nlohmann::json &artifact_data, MonraceDefinition &monrace);
    errr set_mon_escorts(nlohmann::json &escort_data, MonraceDefinition &monrace);
    errr set_mon_blows(nlohmann::json &blow_data, MonraceDefinition &monrace);
    errr set_mon_flags(const nlohmann::json &flag_data, MonraceDefinition &monrace);
    errr set_mon_skills(const nlohmann::json &skill_data, MonraceDefinition &monrace);
    errr set_mon_alliance(const nlohmann::json &alliance_data, MonraceDefinition &monrace);
    errr set_mon_final_summons(const nlohmann::json &summon_data, MonraceDefinition &monrace);
    errr set_mon_message(const nlohmann::json &message_data, MonraceDefinition &monrace);
    errr set_mon_terrain_feature(const nlohmann::json &terrain_data, MonraceDefinition &monrace);
    errr set_mon_spawn_creature(const nlohmann::json &spawn_data, MonraceDefinition &monrace);
    errr set_mon_spawn_item(const nlohmann::json &spawn_data, MonraceDefinition &monrace);
    errr set_mon_drop_kinds(const nlohmann::json &drop_data, MonraceDefinition &monrace);
    errr set_mon_dead_spawns(const nlohmann::json &dead_spawn_data, MonraceDefinition &monrace);

    nlohmann::json &monrace_data;
};
