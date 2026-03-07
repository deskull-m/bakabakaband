/*!
 * @brief 突然変異の一覧を出力する
 * @date 2020/04/24
 * @author Hourier
 */

#include "knowledge/knowledge-mutations.h"
#include "core/show-file.h"
#include "io-dump/dump-util.h"
#include "io/mutations-dump.h"
#include "system/creature-entity.h"
#include "util/angband-files.h"

/*!
 * @brief 突然変異表示コマンドの実装 / List mutations we have...
 */
void do_cmd_knowledge_mutations(CreatureEntity &creature)
{
    FILE *fff = nullptr;
    GAME_TEXT file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name)) {
        return;
    }

    dump_mutations(creature, fff);
    angband_fclose(fff);

    FileDisplayer(creature.name).display(true, file_name, 0, 0, _("突然変異", "Mutations"));
    fd_kill(file_name);
}
