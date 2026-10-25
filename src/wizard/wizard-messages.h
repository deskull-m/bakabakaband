#pragma once

#include "system/angband.h"
#include <fmt/format.h>
#include <string_view>
#include <utility>

class CreatureEntity;
bool is_wizard_message_enabled(int cheat_type);
void msg_print_wizard(CreatureEntity &creature, int cheat_type, std::string_view msg);
void msg_print_wizard_damage(CreatureEntity &creature, const CreatureEntity &monster, int damage);

template <typename... Args>
void msg_print_wizard(CreatureEntity &creature, int cheat_type, fmt::format_string<Args...> fmt, Args &&...args)
{
    // 表示しない種類のメッセージは書式を組み立てない
    if (!is_wizard_message_enabled(cheat_type)) {
        return;
    }

    msg_print_wizard(creature, cheat_type, fmt::format(fmt, std::forward<Args>(args)...));
}
