#include "autopick/autopick-pref-processor.h"
#include "autopick/autopick-entry.h"
#include "autopick/autopick-util.h"
#include "system/angband.h"

/*!
 * @brief Process line for auto picker/destroyer.
 */
void process_autopick_file_command(std::string_view buf)
{
    autopick_type entry;
    size_t pos = 0;
    for (; pos < buf.length(); pos++) {
        const uint8_t c = buf[pos];
#ifdef JP
        if (iskanji(c)) {
            pos++;
            continue;
        }
#endif
        if (iswspace(c) && c != ' ') {
            break;
        }
    }
    if (!autopick_new_entry(&entry, buf.substr(0, pos), false)) {
        return;
    }

    for (const auto &item : autopick_list) {
        if ((entry.name == item.name) && entry.flags[0] == item.flags[0] && entry.flags[1] == item.flags[1] && entry.dice == item.dice && entry.bonus == item.bonus) {
            return;
        }
    }

    autopick_list.push_back(std::move(entry));
}
