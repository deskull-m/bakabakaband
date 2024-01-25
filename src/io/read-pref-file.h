﻿#pragma once

#include "system/angband.h"
#include <filesystem>
#include <string_view>

extern char auto_dump_header[];
extern char auto_dump_footer[];

class PlayerType;
errr process_pref_file(PlayerType *player_ptr, concptr name, bool only_user_dir = false);
errr process_autopick_file(PlayerType *player_ptr, concptr name);
errr process_histpref_file(PlayerType *player_ptr, concptr name);
bool read_histpref(PlayerType *player_ptr);

void auto_dump_printf(FILE *auto_dump_stream, concptr fmt, ...);
bool open_auto_dump(FILE **fpp, const std::filesystem::path &path, std::string_view mark);
void close_auto_dump(FILE **fpp, std::string_view mark);

void load_all_pref_files(PlayerType *player_ptr);
