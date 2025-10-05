/*!
 * @brief 武術スタイル切り替えコマンド
 * @date 2024/12/19
 * @author Bakabakaband Development Team
 */

#include "cmd-action/cmd-martial-arts.h"
#include "combat/martial-arts-style.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "io/input-key-acceptor.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "player-base/player-class.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "view/display-player.h"

/*!
 * @brief 武術スタイル選択メニューを表示
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param current_style 現在のスタイル
 * @return 選択されたスタイル (キャンセル時は MAX)
 */
static MartialArtsStyleType select_martial_arts_style_menu(MartialArtsStyleType current_style)
{
    constexpr auto max_style = static_cast<int>(MartialArtsStyleType::MAX);

    screen_save();

    for (int i = 0; i < max_style; i++) {
        auto style = static_cast<MartialArtsStyleType>(i);
        auto key = I2A(i);
        auto name = get_martial_arts_style_name(style);
        auto desc = get_martial_arts_style_desc(style);
        auto marker = (style == current_style) ? "*" : " ";

        prt(format("%c)%s%s - %s", key, marker, name, desc), 2 + i, 5);
    }

    prt("どの武術スタイルを使用しますか？", 1, 5);
    prt("(ESCキーでキャンセル)", max_style + 3, 5);

    while (true) {
        auto key = inkey();

        if (key == ESCAPE) {
            screen_load();
            return MartialArtsStyleType::MAX;
        }

        auto index = A2I(key);
        if (index < max_style) {
            screen_load();
            return static_cast<MartialArtsStyleType>(index);
        }
    }
}

/*!
 * @brief 武術スタイル切り替えコマンドのメイン処理
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_martial_arts_style(PlayerType *player_ptr)
{
    // 修行僧、狂戦士、練気術師のみ使用可能
    PlayerClass pc(player_ptr);
    if (!pc.equals(PlayerClassType::MONK) &&
        !pc.equals(PlayerClassType::BERSERKER) &&
        !pc.equals(PlayerClassType::FORCETRAINER)) {
        msg_print("あなたは武術スタイルを変更できない。");
        return;
    }

    auto current_style = player_ptr->martial_arts_style;
    auto new_style = select_martial_arts_style_menu(current_style);

    if (new_style == MartialArtsStyleType::MAX) {
        msg_print("キャンセルしました。");
        return;
    }

    if (new_style == current_style) {
        msg_print("同じスタイルが選択されています。");
        return;
    }

    player_ptr->martial_arts_style = new_style;

    auto style_name = get_martial_arts_style_name(new_style);
    msg_format("武術スタイルを%sに変更しました。", style_name);

    handle_stuff(player_ptr);
}
