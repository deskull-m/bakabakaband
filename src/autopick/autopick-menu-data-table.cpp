/*!
 * @brief 自動拾いのユーティリティ
 * @date 2020/04/25
 * @author Hourier
 */

#include "autopick/autopick-menu-data-table.h"
#include "autopick/autopick-commands-table.h"
#include "autopick/autopick-keys-table.h"
#include "locale/language-switcher.h"
#include "util/int-char-converter.h"

/*
 * These MN_* preprocessor macros were formerly defined in
 * autopick-editor-table.h.  Fold them in here to satisfy two requirements:
 * 1) The platforms using configure/automake only preprocess .c files (.h
 * files are not touched) with nkf to modify the encoding of Japanese
 * characters.
 * 2) The file scope initializer for menu_data requires constant expressions
 * that can be evaluated at compile time.
 */
#define MN_QUIT _("セーブ無しで終了", "Quit without save")
#define MN_SAVEQUIT _("セーブして終了", "Save & Quit")
#define MN_REVERT _("全ての変更を破棄", "Revert all changes")
#define MN_HELP _("ヘルプ", "Help")

#define MN_MOVE _("カーソル移動", "Move cursor")
#define MN_LEFT _("左       (←矢印キー)", "Left              (Left Arrow key)")
#define MN_DOWN _("下       (↓矢印キー)", "Down              (Down Arrow key)")
#define MN_UP _("上       (↑矢印キー)", "Up                (Up Arrow key)")
#define MN_RIGHT _("右       (→矢印キー)", "Right             (Right Arrow key)")
#define MN_BOL _("行頭     (Homeキー)", "Beginning of line (Home key)")
#define MN_EOL _("行末     (Endキー)", "End of line       (End key)")
#define MN_PGUP _("上ページ (PageUpキー)", "Page up           (PageUp key)")
#define MN_PGDOWN _("下ページ (PageDownキー)", "Page down         (PageDown key)")
#define MN_TOP _("先頭行   (Ctrl+Homeキー)", "Top               (Ctrl+Home key)")
#define MN_BOTTOM _("最終行   (Ctrl+Endキー)", "Bottom            (Ctrl+End key)")

#define MN_EDIT _("編集", "Edit")
#define MN_CUT _("カット", "Cut")
#define MN_COPY _("コピー", "Copy")
#define MN_PASTE _("ペースト", "Paste")
#define MN_BLOCK _("選択範囲の指定", "Select block")
#define MN_KILL_LINE _("行の残りを削除", "Kill rest of line")
#define MN_DELETE_CHAR _("1文字削除", "Delete character")
#define MN_BACKSPACE _("バックスペース", "Backspace")
#define MN_RETURN _("改行", "Return")

#define MN_SEARCH _("検索", "Search")
#define MN_SEARCH_STR _("文字列で検索", "Search by string")
#define MN_SEARCH_FORW _("前方へ再検索", "Search foward")
#define MN_SEARCH_BACK _("後方へ再検索", "Search backward")
#define MN_SEARCH_OBJ _("アイテムを選択して検索", "Search by inventory object")
#define MN_SEARCH_DESTROYED _("自動破壊されたアイテムで検索", "Search by destroyed object")

#define MN_INSERT _("色々挿入", "Insert...")
#define MN_INSERT_OBJECT _("選択したアイテムの名前を挿入", "Insert name of choosen object")
#define MN_INSERT_DESTROYED _("自動破壊されたアイテムの名前を挿入", "Insert name of destroyed object")
#define MN_INSERT_BLOCK _("条件分岐ブロックの例を挿入", "Insert conditional block")
#define MN_INSERT_MACRO _("マクロ定義を挿入", "Insert a macro definition")
#define MN_INSERT_KEYMAP _("キーマップ定義を挿入", "Insert a keymap definition")

#define MN_COMMAND_LETTER _("拾い/破壊/放置の選択", "Command letter")
#define MN_CL_AUTOPICK _("「 」 (自動拾い)", "' ' (Auto pick)")
#define MN_CL_DESTROY _("「!」 (自動破壊)", "'!' (Auto destroy)")
#define MN_CL_LEAVE _("「~」 (放置)", "'~' (Leave it on the floor)")
#define MN_CL_QUERY _("「;」 (確認して拾う)", "';' (Query to pick up)")
#define MN_CL_NO_DISP _("「(」 (マップコマンドで表示しない)", "'(' (No display on the large map)")

#define MN_ADJECTIVE_GEN _("形容詞(一般)の選択", "Adjective (general)")
#define MN_RARE _("レアな (装備)", "rare (equipment)")
#define MN_COMMON _("ありふれた (装備)", "common (equipment)")

#define MN_ADJECTIVE_SPECIAL _("形容詞(特殊)の選択", "Adjective (special)")
#define MN_BOOSTED _("ダイス目の違う (武器)", "dice boosted (weapons)")
#define MN_MORE_DICE _("ダイス目 # 以上の (武器)", "more than # dice (weapons)")
#define MN_MORE_BONUS _("修正値 # 以上の (指輪等)", "more bonus than # (rings etc.)")
#define MN_WANTED _("賞金首の (死体)", "wanted (corpse)")
#define MN_UNIQUE _("ユニーク・モンスターの (死体)", "unique (corpse)")
#define MN_HUMAN _("人間の (死体)", "human (corpse)")
#define MN_UNREADABLE _("読めない (魔法書)", "unreadable (spellbooks)")
#define MN_REALM1 _("第一領域の (魔法書)", "realm1 (spellbooks)")
#define MN_REALM2 _("第二領域の (魔法書)", "realm2 (spellbooks)")
#define MN_FIRST _("1冊目の (魔法書)", "first (spellbooks)")
#define MN_SECOND _("2冊目の (魔法書)", "second (spellbooks)")
#define MN_THIRD _("3冊目の (魔法書)", "third (spellbooks)")
#define MN_FOURTH _("4冊目の (魔法書)", "fourth (spellbooks)")

#define MN_NOUN _("名詞の選択", "Keywords (noun)")

constexpr char DELETE = 0x7F;

CommandMenuData CommandMenuData::instance{};

CommandMenuData &CommandMenuData::get_instance()
{
    return instance;
}

void CommandMenuData::initialize()
{
    if (!this->menu_data.empty()) {
        return;
    }

    this->menu_data.reserve(100);
    this->menu_data.emplace_back(MN_HELP, 0, tl::nullopt, EC_HELP);
    this->menu_data.emplace_back(MN_QUIT, 0, KTRL('q'), EC_QUIT);
    this->menu_data.emplace_back(MN_SAVEQUIT, 0, KTRL('w'), EC_SAVEQUIT);
    this->menu_data.emplace_back(MN_REVERT, 0, KTRL('z'), EC_REVERT);

    this->menu_data.emplace_back(MN_EDIT, 0, tl::nullopt, tl::nullopt);
    this->menu_data.emplace_back(MN_CUT, 1, KTRL('x'), EC_CUT);
    this->menu_data.emplace_back(MN_COPY, 1, KTRL('c'), EC_COPY);
    this->menu_data.emplace_back(MN_PASTE, 1, KTRL('v'), EC_PASTE);
    this->menu_data.emplace_back(MN_BLOCK, 1, KTRL('g'), EC_BLOCK);
    this->menu_data.emplace_back(MN_KILL_LINE, 1, KTRL('k'), EC_KILL_LINE);
    this->menu_data.emplace_back(MN_DELETE_CHAR, 1, KTRL('d'), EC_DELETE_CHAR);
    this->menu_data.emplace_back(MN_BACKSPACE, 1, KTRL('h'), EC_BACKSPACE);
    this->menu_data.emplace_back(MN_RETURN, 1, KTRL('j'), EC_RETURN);
    this->menu_data.emplace_back(MN_RETURN, 1, KTRL('m'), EC_RETURN);

    this->menu_data.emplace_back(MN_SEARCH, 0, tl::nullopt, tl::nullopt);
    this->menu_data.emplace_back(MN_SEARCH_STR, 1, KTRL('s'), EC_SEARCH_STR);
    this->menu_data.emplace_back(MN_SEARCH_FORW, 1, tl::nullopt, EC_SEARCH_FORW);
    this->menu_data.emplace_back(MN_SEARCH_BACK, 1, KTRL('r'), EC_SEARCH_BACK);
    this->menu_data.emplace_back(MN_SEARCH_OBJ, 1, KTRL('y'), EC_SEARCH_OBJ);
    this->menu_data.emplace_back(MN_SEARCH_DESTROYED, 1, tl::nullopt, EC_SEARCH_DESTROYED);

    this->menu_data.emplace_back(MN_MOVE, 0, tl::nullopt, tl::nullopt);
    this->menu_data.emplace_back(MN_LEFT, 1, KTRL('b'), EC_LEFT);
    this->menu_data.emplace_back(MN_DOWN, 1, KTRL('n'), EC_DOWN);
    this->menu_data.emplace_back(MN_UP, 1, KTRL('p'), EC_UP);
    this->menu_data.emplace_back(MN_RIGHT, 1, KTRL('f'), EC_RIGHT);
    this->menu_data.emplace_back(MN_BOL, 1, KTRL('a'), EC_BOL);
    this->menu_data.emplace_back(MN_EOL, 1, KTRL('e'), EC_EOL);
    this->menu_data.emplace_back(MN_PGUP, 1, KTRL('o'), EC_PGUP);
    this->menu_data.emplace_back(MN_PGDOWN, 1, KTRL('l'), EC_PGDOWN);
    this->menu_data.emplace_back(MN_TOP, 1, KTRL('t'), EC_TOP);
    this->menu_data.emplace_back(MN_BOTTOM, 1, KTRL('u'), EC_BOTTOM);

    this->menu_data.emplace_back(MN_INSERT, 0, tl::nullopt, tl::nullopt);
    this->menu_data.emplace_back(MN_INSERT_OBJECT, 1, KTRL('i'), EC_INSERT_OBJECT);
    this->menu_data.emplace_back(MN_INSERT_DESTROYED, 1, tl::nullopt, EC_INSERT_DESTROYED);
    this->menu_data.emplace_back(MN_INSERT_BLOCK, 1, tl::nullopt, EC_INSERT_BLOCK);
    this->menu_data.emplace_back(MN_INSERT_MACRO, 1, tl::nullopt, EC_INSERT_MACRO);
    this->menu_data.emplace_back(MN_INSERT_KEYMAP, 1, tl::nullopt, EC_INSERT_KEYMAP);

    this->menu_data.emplace_back(MN_ADJECTIVE_GEN, 0, tl::nullopt, tl::nullopt);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::UNAWARE), 1, tl::nullopt, EC_IK_UNAWARE);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::UNIDENTIFIED), 1, tl::nullopt, EC_IK_UNIDENTIFIED);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::IDENTIFIED), 1, tl::nullopt, EC_IK_IDENTIFIED);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::STAR_IDENTIFIED), 1, tl::nullopt, EC_IK_STAR_IDENTIFIED);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::COLLECTING), 1, tl::nullopt, EC_OK_COLLECTING);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::ARTIFACT), 1, tl::nullopt, EC_OK_ARTIFACT);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::EGO), 1, tl::nullopt, EC_OK_EGO);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::GOOD), 1, tl::nullopt, EC_OK_GOOD);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::NAMELESS), 1, tl::nullopt, EC_OK_NAMELESS);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::AVERAGE), 1, tl::nullopt, EC_OK_AVERAGE);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::WORTHLESS), 1, tl::nullopt, EC_OK_WORTHLESS);
    this->menu_data.emplace_back(MN_RARE, 1, tl::nullopt, EC_OK_RARE);
    this->menu_data.emplace_back(MN_COMMON, 1, tl::nullopt, EC_OK_COMMON);

    this->menu_data.emplace_back(MN_ADJECTIVE_SPECIAL, 0, tl::nullopt, tl::nullopt);
    this->menu_data.emplace_back(MN_BOOSTED, 1, tl::nullopt, EC_OK_BOOSTED);
    this->menu_data.emplace_back(MN_MORE_DICE, 1, tl::nullopt, EC_OK_MORE_DICE);
    this->menu_data.emplace_back(MN_MORE_BONUS, 1, tl::nullopt, EC_OK_MORE_BONUS);
    this->menu_data.emplace_back(MN_WANTED, 1, tl::nullopt, EC_OK_WANTED);
    this->menu_data.emplace_back(MN_UNIQUE, 1, tl::nullopt, EC_OK_UNIQUE);
    this->menu_data.emplace_back(MN_HUMAN, 1, tl::nullopt, EC_OK_HUMAN);
    this->menu_data.emplace_back(MN_UNREADABLE, 1, tl::nullopt, EC_OK_UNREADABLE);
    this->menu_data.emplace_back(MN_REALM1, 1, tl::nullopt, EC_OK_REALM1);
    this->menu_data.emplace_back(MN_REALM2, 1, tl::nullopt, EC_OK_REALM2);
    this->menu_data.emplace_back(MN_FIRST, 1, tl::nullopt, EC_OK_FIRST);
    this->menu_data.emplace_back(MN_SECOND, 1, tl::nullopt, EC_OK_SECOND);
    this->menu_data.emplace_back(MN_THIRD, 1, tl::nullopt, EC_OK_THIRD);
    this->menu_data.emplace_back(MN_FOURTH, 1, tl::nullopt, EC_OK_FOURTH);

    this->menu_data.emplace_back(MN_NOUN, 0, tl::nullopt, tl::nullopt);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::WEAPONS), 1, tl::nullopt, EC_KK_WEAPONS);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::FAVORITE_WEAPONS), 1, tl::nullopt, EC_KK_FAVORITE_WEAPONS);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::ARMORS), 1, tl::nullopt, EC_KK_ARMORS);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::MISSILES), 1, tl::nullopt, EC_KK_MISSILES);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::DEVICES), 1, tl::nullopt, EC_KK_DEVICES);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::LIGHTS), 1, tl::nullopt, EC_KK_LIGHTS);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::JUNKS), 1, tl::nullopt, EC_KK_JUNKS);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::CORPSES), 1, tl::nullopt, EC_KK_CORPSES);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::SPELLBOOKS), 1, tl::nullopt, EC_KK_SPELLBOOKS);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::SHIELDS), 1, tl::nullopt, EC_KK_SHIELDS);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::BOWS), 1, tl::nullopt, EC_KK_BOWS);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::RINGS), 1, tl::nullopt, EC_KK_RINGS);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::AMULETS), 1, tl::nullopt, EC_KK_AMULETS);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::SUITS), 1, tl::nullopt, EC_KK_SUITS);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::CLOAKS), 1, tl::nullopt, EC_KK_CLOAKS);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::HELMS), 1, tl::nullopt, EC_KK_HELMS);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::GLOVES), 1, tl::nullopt, EC_KK_GLOVES);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::BOOTS), 1, tl::nullopt, EC_KK_BOOTS);

    this->menu_data.emplace_back(MN_COMMAND_LETTER, 0, tl::nullopt, tl::nullopt);
    this->menu_data.emplace_back(MN_CL_AUTOPICK, 1, tl::nullopt, EC_CL_AUTOPICK);
    this->menu_data.emplace_back(MN_CL_DESTROY, 1, tl::nullopt, EC_CL_DESTROY);
    this->menu_data.emplace_back(MN_CL_LEAVE, 1, tl::nullopt, EC_CL_LEAVE);
    this->menu_data.emplace_back(MN_CL_QUERY, 1, tl::nullopt, EC_CL_QUERY);
    this->menu_data.emplace_back(MN_CL_NO_DISP, 1, tl::nullopt, EC_CL_NO_DISP);

    this->menu_data.emplace_back(MN_DELETE_CHAR, tl::nullopt, DELETE, EC_DELETE_CHAR);
}

const CommandMenuDatum &CommandMenuData::get_datum(size_t num) const
{
    return this->menu_data.at(num);
}

/*!
 * @brief Find a command by 'key'.
 */
int CommandMenuData::get_com_id(char key) const
{
    for (const auto &menu_datum : this->menu_data) {
        if (menu_datum.key == key) {
            return menu_datum.com_id.value_or(-1);
        }
    }

    return 0;
}
