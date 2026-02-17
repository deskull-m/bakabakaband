#pragma once

#include "system/angband.h"
#include <filesystem>
#include <string_view>

extern char auto_dump_header[];
extern char auto_dump_footer[];

class CreatureEntity;
errr process_pref_file(CreatureEntity &creature, std::string_view name, bool only_user_dir = false);
errr process_autopick_file(CreatureEntity &creature, std::string_view name);
errr process_histpref_file(CreatureEntity &creature, std::string_view name);
bool read_histpref(CreatureEntity &creature);

void auto_dump_printf(FILE *auto_dump_stream, const char *fmt, ...);
bool open_auto_dump(FILE **fpp, const std::filesystem::path &path, std::string_view mark);
void close_auto_dump(FILE **fpp, std::string_view mark);

void load_all_pref_files(CreatureEntity &creature);
