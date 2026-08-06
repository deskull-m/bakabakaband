#pragma once

#include <filesystem>

class CreatureEntity;
class PlayerType;
void exe_cmd_save_screen_html(const std::filesystem::path &path, bool need_message);
void do_cmd_save_screen(CreatureEntity &creature);
void do_cmd_load_screen(void);
