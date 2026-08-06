#include "cmd-io/cmd-help.h"
#include "core/show-file.h"
#include "system/creature-entity.h"
#include "term/screen-processor.h"

/*!
 * @brief ヘルプを表示するコマンドのメインルーチン
 * Peruse the On-Line-Help
 * @param creature クリーチャーへの参照
 * @details
 */
void do_cmd_help(CreatureEntity &creature)
{
    screen_save();
    FileDisplayer(creature.name).display(true, _("jhelp.hlp", "help.hlp"), 0, 0);
    screen_load();
}
