#include "view/display-player-misc-info.h"
#include "player-info/class-info.h"
#include "player-info/mimic-info-table.h"
#include "player/player-personality.h"
#include "player/player-sex.h"
#include "system/creature-entity.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "view/display-player-stat-info.h"
#include <sstream>

/*!
 * @brief 画面上部にプレイヤーの名前を表示する
 *
 * @param creature クリーチャーへの参照
 * @param name_only trueならば名前のみ表示する。falseならばプレイヤーの性格も表示する。
 */
void display_player_name(CreatureEntity &creature, bool name_only)
{
    std::stringstream ss;
    if (!name_only && creature.personality != nullptr) {
        ss << (*creature.personality).title << _((*creature.personality).no == 1 ? "の" : "", " ");
    }
    ss << creature.name;
    const auto display_name = ss.str();

    constexpr std::string_view header = _("名前  : ", "Name  : ");
    const auto length = header.length() + display_name.length();

    const auto &[wid, hgt] = term_get_size();
    const auto center_col = (wid - length) / 2 - 4; // ヘッダがあるぶん少し左に寄せたほうが見やすい
    constexpr auto row = 1;

    term_erase(0, row);
    term_putstr(center_col, row, -1, TERM_WHITE, header);
    term_putstr(center_col + header.length(), row, -1, TERM_L_BLUE, display_name);
}

/*!
 * @brief プレイヤーの特性フラグ一覧表示2a /
 * @param creature クリーチャーへの参照
 * Special display, part 2a
 */
void display_player_misc_info(CreatureEntity &creature)
{
    display_player_name(creature);

    put_str(_("性別  :", "Sex   :"), 3, 1);
    put_str(_("種族  :", "Race  :"), 4, 1);
    put_str(_("職業  :", "Class :"), 5, 1);

    c_put_str(TERM_L_BLUE, sp_ptr->title, 3, 9);
    c_put_str(TERM_L_BLUE, (creature.get_mimic_form() != MimicKindType::NONE ? mimic_info.at(creature.get_mimic_form()).title : creature.race->title), 4, 9);
    c_put_str(TERM_L_BLUE, (*creature.pclass_ref).title, 5, 9);

    put_str(_("レベル:", "Level :"), 6, 1);
    put_str(_("ＨＰ  :", "Hits  :"), 7, 1);
    put_str(_("ＭＰ  :", "Mana  :"), 8, 1);

    c_put_str(TERM_L_BLUE, format("%d", (int)creature.level), 6, 9);
    c_put_str(TERM_L_BLUE, format("%d/%d", (int)creature.hp, (int)creature.maxhp), 7, 9);
    c_put_str(TERM_L_BLUE, format("%d/%d", (int)creature.csp, (int)creature.msp), 8, 9);
}
