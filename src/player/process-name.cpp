#include "player/process-name.h"
#include "system/creature-entity.h"
#include "autopick/autopick-reader-writer.h"
#include "core/asking-player.h"
#include "game-option/birth-options.h"
#include "io/files-util.h"
#include "player/player-personality.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/angband-files.h"
#include "util/finalizer.h"
#include "util/string-processor.h"
#include "view/display-player-misc-info.h"
#include "world/world.h"
#include <sstream>
#ifdef SAVEFILE_USE_UID
#include "main-unix/unix-user-ids.h"
#endif

/*!
 * @brief プレイヤーの名前をチェックして修正する
 * Process the player name.
 * @param creature クリーチャーへの参照
 * @param sf セーブファイル名に合わせた修正を行うならばTRUE
 * @details
 * Extract a clean "base name".
 * Build the savefile name if needed.
 */
void process_player_name(CreatureEntity &creature, bool is_new_savefile)
{
    auto &player = static_cast<PlayerType &>(creature);
    const auto &world = AngbandWorld::get_instance();
    const std::string old_player_base = world.character_generated ? player.base_name : "";

    const auto name_c_str = player.name.c_str();
    for (size_t i = 0; i < player.name.length(); i++) {
#ifdef JP
        if (iskanji(name_c_str[i])) {
            i++;
            continue;
        }

        if (iscntrl((unsigned char)name_c_str[i])) {
#else
        if (iscntrl(name_c_str[i])) {
#endif
            quit_fmt(_("'%s' という名前は不正なコントロールコードを含んでいます。", "The name '%s' contains control chars!"), name_c_str);
        }
    }

    player.base_name.clear();
    for (size_t i = 0; i < player.name.length(); i++) {
#ifdef JP
        unsigned char c = name_c_str[i];
#else
        char c = name_c_str[i];
#endif

#ifdef JP
        if (iskanji(c)) {
            if (player.base_name.length() + 2 >= 40 || i + 1 >= player.name.length()) {
                break;
            }

            player.base_name += c;
            i++;
            player.base_name += name_c_str[i];
        }
#ifdef SJIS
        else if (iskana(c))
            player.base_name += c;
#endif
        else
#endif
            if (!strncmp(PATH_SEP, name_c_str + i, strlen(PATH_SEP))) {
            player.base_name += '_';
            i += strlen(PATH_SEP);
        }
#if defined(WINDOWS)
        else if (angband_strchr("\"*,/:;<>?\\|", c))
            player.base_name += '_';
#endif
        else if (isprint(c)) {
            player.base_name += c;
        }
    }

    if (player.base_name.empty()) {
        player.base_name = "PLAYER";
    }

    auto is_modified = false;
    if (is_new_savefile && (savefile.empty() || !keep_savefile)) {
        std::stringstream ss;

#ifdef SAVEFILE_USE_UID
        ss << UnixUserIds::get_instance().get_user_id();
        ss << '.' << player.base_name;
#else
        ss << player.base_name;
#endif
        savefile = path_build(ANGBAND_DIR_SAVE, ss.str());
        is_modified = true;
    }

    if (is_modified || savefile_base.empty()) {
#ifdef SAVEFILE_USE_UID
        const auto savefile_str = savefile.filename().string();
        const auto split = str_split(savefile_str, '.');
        savefile_base = split[1];
#else
        savefile_base = savefile.filename();
#endif
    }

    if (world.character_generated && old_player_base != player.base_name) {
        autopick_load_pref(creature, false);
    }
}

/*!
 * @brief プレイヤーの名前を変更する
 * @param creature クリーチャーへの参照
 * @details PlayerType::name は32バイトで定義されているが、
 * スコアファイル（lib/apex/scores.raw）に保存されているプレイヤー名が最大16バイト (ヌル文字含)となっている
 * このため最大値を16バイトに制限する
 */
void get_name(CreatureEntity &creature)
{
    auto &player = static_cast<PlayerType &>(creature);
    const auto finalizer = util::make_finalizer([&creature]() {
        display_player_misc_info(creature);
    });

    std::string initial_name = player.name;
    const auto max_name_size = 40;
    constexpr auto prompt = _("キャラクターの名前を入力して下さい: ", "Enter a name for your character: ");
    const auto name = input_string(prompt, max_name_size, initial_name);
    if (name && !name->empty()) {
        player.name = *name;
        return;
    }

    if (initial_name.empty()) {
        player.name = "PLAYER";
    }
}
