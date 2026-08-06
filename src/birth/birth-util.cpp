#include "birth/birth-util.h"
#include "cmd-io/cmd-gameoption.h"
#include "core/show-file.h"
#include "game-option/game-option-page.h"
#include "main/sound-of-music.h"
#include "system/creature-entity.h"
#include "term/screen-processor.h"
#include <string>

/*!
 * @brief プレイヤー作成を中断して馬鹿馬鹿蛮怒を終了する
 */
void birth_quit(void)
{
    quit("");
}

/*!
 * @brief 指定されたヘルプファイルを表示する / Show specific help file
 * @param creature クリーチャーへの参照
 * @param helpfile ファイル名
 */
void show_help(CreatureEntity &creature, std::string_view helpfile)
{
    screen_save();
    FileDisplayer(creature.name).display(true, helpfile, 0, 0);
    screen_load();
}

void birth_help_option(CreatureEntity &creature, char c, BirthKind bk)
{
    std::string help_file;
    switch (bk) {
    case BirthKind::RACE:
        help_file = _("jraceclas.txt#TheRaces", "raceclas.txt#TheRaces");
        break;
    case BirthKind::CLASS:
        help_file = _("jraceclas.txt#TheClasses", "raceclas.txt#TheClasses");
        break;
    case BirthKind::REALM:
        help_file = _("jmagic.txt#MagicRealms", "magic.txt#MagicRealms");
        break;
    case BirthKind::PERSONALITY:
        help_file = _("jraceclas.txt#ThePersonalities", "raceclas.txt#ThePersonalities");
        break;
    case BirthKind::PATRON:
        help_file = _("jraceclas.txt#ThePatrons", "raceclas.txt#ThePatrons");
        break;
    case BirthKind::AUTO_ROLLER:
        help_file = _("jbirth.txt#AutoRoller", "birth.txt#AutoRoller");
        break;
    default:
        help_file = "";
        break;
    }

    if (c == '?') {
        show_help(creature, help_file);
    } else if (c == '=') {
        screen_save();
        do_cmd_options_aux(creature, GameOptionPage::BIRTH, _("初期オプション((*)はスコアに影響)", "Birth Options ((*)) affect score"));
        screen_load();
    } else if (c != '2' && c != '4' && c != '6' && c != '8') {
        bell();
    }
}
