/*
 * @brief 読むことができるアイテム群の内、中つ国ガイドを読んだ時の効果や処理を記述する.
 * @date 2022/02/26
 * @author Hourier
 */

#include "object-use/read/parchment-read-executor.h"
#include "core/show-file.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "io/files-util.h"
#include "system/angband.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "term/screen-processor.h"
#include "util/angband-files.h"
#include "view/display-messages.h"
#include <filesystem>
#include <iomanip>
#include <sstream>

ParchmentReadExecutor::ParchmentReadExecutor(CreatureEntity &creature, ItemEntity *o_ptr)
    : creature(creature)
    , o_ptr(o_ptr)
{
}

bool ParchmentReadExecutor::is_identified() const
{
    return false;
}

bool ParchmentReadExecutor::read()
{
    std::stringstream ss;
    ss << "book-" << std::setfill('0') << std::right << std::setw(3) << *this->o_ptr->bi_key.sval();
    ss << "_" << _("jp", "en") << ".txt";
    const auto item_name = describe_flavor(this->creature, *this->o_ptr, OD_NAME_ONLY);
    auto path = path_build(ANGBAND_DIR_FILE, "books");
    path.append(ss.str());

    // 本文ファイルが無い読み物 (宝の地図、英語版の一部の石碑等) もある。FileDisplayer::display() は
    // ファイルが無いと例外を投げて異常終了させるため、先に存在を確かめて読めない旨を伝える。
    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec)) {
        msg_print(_("{}には読み取れることが何も書かれていない。", "There is nothing legible written on {}."), item_name);
        return false;
    }

    screen_save();
    FileDisplayer(this->creature.name).display(true, path.string(), 0, 0, item_name);
    screen_load();
    return false;
}
