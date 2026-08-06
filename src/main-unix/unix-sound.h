#pragma once
#include <filesystem>

namespace unix_sound {

bool init_sound(std::filesystem::path, std::filesystem::path);
bool play_sound(int);
void finalize_sound();

};
