#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

class CreatureEntity;
enum class PickType;
void autopick_load_pref(CreatureEntity &creature, bool disp_mes);
std::filesystem::path search_pickpref_path(std::string_view player_base_name);
std::pair<PickType, std::vector<std::unique_ptr<std::string>>> read_pickpref_text_lines(std::string_view player_base_name);
bool write_text_lines(std::string_view filename, const std::vector<std::unique_ptr<std::string>> &lines);
std::string pickpref_filename(std::string_view player_base_name, PickType filename_mode);
