#pragma once

#include "system/angband.h"
#include <filesystem>
#include <fmt/format.h>
#include <string_view>
#include <utility>

extern char auto_dump_header[];
extern char auto_dump_footer[];

class CreatureEntity;
constexpr errr PREF_EXPRESSION_ERROR = 2;
errr check_autopick_file_conditions(CreatureEntity &creature, std::string_view name);
errr process_pref_file(CreatureEntity &creature, std::string_view name, bool only_user_dir = false);
errr process_autopick_file(CreatureEntity &creature, std::string_view name);
errr process_histpref_file(CreatureEntity &creature, std::string_view name);
bool read_histpref(CreatureEntity &creature);

void auto_dump_print(FILE *auto_dump_stream, std::string_view str);
bool open_auto_dump(FILE **fpp, const std::filesystem::path &path, std::string_view mark);
bool close_auto_dump(FILE **fpp, std::string_view mark);

void load_all_pref_files(CreatureEntity &creature);

template <typename... Args>
void auto_dump_print(FILE *auto_dump_stream, fmt::format_string<Args...> fmt, Args &&...args)
{
    auto_dump_print(auto_dump_stream, fmt::format(fmt, std::forward<Args>(args)...));
}
