/*!
 * @file main-win.cpp
 * @brief Windows版固有実装(メインエントリポイント含む)
 * @date 2018/03/16
 * @author Hengband Team
 * @details
 *
 * <h3>概要</h3>
 * Windows98かその前後の頃を起点としたAPI実装。
 * 各種のゲームエンジンは無論、
 * DirectXといった昨今描画に標準的となったライブラリも用いていない。
 * タイルの描画処理などについては、現在動作の詳細を検証中。
 *
 * <h3>フォーク元の概要</h3>
 * <p>
 * Copyright (c) 1997 Ben Harrison, Skirmantas Kligys, and others
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.
 * </p>
 * <p>
 * This file helps Angband work with Windows computers.
 *
 * To use this file, use an appropriate "Makefile" or "Project File",
 * make sure that "WINDOWS" and/or "WIN32" are defined somewhere, and
 * make sure to obtain various extra files as described below.
 *
 * The official compilation uses the CodeWarrior Pro compiler, which
 * includes a special project file and precompilable header file.
 * </p>
 *
 * <p>
 * The "lib/user/pref-win.prf" file contains keymaps, macro definitions,
 * and/or color redefinitions.
 * </p>
 *
 * <p>
 * The "lib/user/font-win.prf" contains attr/char mappings for wall.bmp.
 * </p>
 *
 * <p>
 * The "lib/user/graf-win.prf" contains attr/char mappings for use with the
 * special bitmap files in "lib/xtra/graf", which are activated by a menu
 * item.
 * </p>
 *
 * <p>
 * Compiling this file, and using the resulting executable, requires
 * several extra files not distributed with the standard Angband code.
 * All of these extra files can be found in the "ext-win" archive.
 * </p>
 *
 * <p>
 * The "term_xtra_win_clear()" function should probably do a low-level
 * clear of the current window, and redraw the borders and other things,
 * if only for efficiency.
 * </p>
 *
 * <p>
 * A simpler method is needed for selecting the "tile size" for windows.
 * </p>
 *
 * <p>
 * The various "warning" messages assume the existance of the "screen.w"
 * window, I think, and only a few calls actually check for its existance,
 * this may be okay since "nullptr" means "on top of all windows". (?)  The
 * user must never be allowed to "hide" the main window, or the "menubar"
 * will disappear.
 * </p>
 *
 * <p>
 * Initial framework (and most code) by Ben Harrison (benh@phial.com).
 *
 * Original code by Skirmantas Kligys (kligys@scf.usc.edu).
 *
 * Additional code by Ross E Becker (beckerr@cis.ohio-state.edu),
 * and Chris R. Martin (crm7479@tam2000.tamu.edu).
 * </p>
 */

#ifdef WINDOWS

#include "cmd-io/cmd-save.h"
#include "cmd-visual/cmd-draw.h"
#include "core/game-play.h"
#include "core/player-processor.h"
#include "core/score-util.h"
#include "core/scores.h"
#include "core/special-internal-keys.h"
#include "core/stuff-handler.h"
#include "core/visuals-reseter.h"
#include "core/window-redrawer.h"
#include "floor/floor-events.h"
#include "game-option/runtime-arguments.h"
#include "game-option/special-options.h"
#include "io/files-util.h"
#include "io/input-key-acceptor.h"
#include "io/record-play-movie.h"
#include "io/signal-handlers.h"
#include "io/write-diary.h"
#include "main-win/commandline-win.h"
#include "main-win/graphics-win.h"
#include "main-win/main-win-bg.h"
#include "main-win/main-win-exception.h"
#include "main-win/main-win-mci.h"
#include "main-win/main-win-menuitem.h"
#include "main-win/main-win-music.h"
#include "main-win/main-win-sound.h"
#include "main-win/main-win-term.h"
#include "main-win/main-win-utils.h"
#include "main/angband-initializer.h"
#include "main/sound-of-music.h"
#include "monster-floor/monster-lite.h"
#include "save/save.h"
#include "system/angband.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/system-variables.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/angband-files.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "view/display-scores.h"
#include "wizard/spoiler-util.h"
#include "wizard/wizard-spoiler.h"
#include "world/world.h"
#include <algorithm>
#include <commdlg.h>
#include <cstdlib>
#include <direct.h>
#include <locale>
#include <string>
#include <string_view>
#include <vector>

/*
 * Window names
 */
LPCWSTR win_term_name[] = { L"Bakabakaband", L"Term-1", L"Term-2", L"Term-3", L"Term-4", L"Term-5", L"Term-6", L"Term-7" };

static term_data data[MAX_TERM_DATA]; //!< An array of term_data's
static bool is_main_term(term_data *td)
{
    return td == &data[0];
}
static term_data *my_td; //!< Hack -- global "window creation" pointer
POINT normsize; //!< Remember normal size of main window when maxmized

/*
 * was main window maximized on previous playing
 */
bool win_maximized = false;

/*
 * game in progress
 */
bool game_in_progress = false;

/*
 * movie in progress
 */
bool movie_in_progress = false;

/*
 * note when "open"/"new" become valid
 */
bool initialized = false;

/*
 * Saved instance handle
 */
static HINSTANCE hInstance;

/*
 * Yellow brush for the cursor
 */
static HBRUSH hbrYellow;

/*
 * An icon
 */
static HICON hIcon;

/* bg */
bg_mode current_bg_mode = bg_mode::BG_NONE;
constexpr auto DEFAULT_BG_FILENAME = "bg.bmp";
std::filesystem::path wallpaper_path = ""; //!< 壁紙ファイル名。

/*
 * Show sub-windows even when Hengband is not in focus
 */
static bool keep_subwindows = true;

/*
 * Full path to ANGBAND.INI
 */
static std::filesystem::path path_ini_file;

/*
 * Name of application
 */
static LPCWSTR AppName = L"ANGBAND";

/*
 * Name of sub-window type
 */
static LPCWSTR AngList = L"AngList";

/*
 * The "complex" color values
 */
static COLORREF win_clr[256];

/*
 * Flag for macro trigger with dump ASCII
 */
static bool term_no_press = false;

/*
 * Copy and paste
 */
static bool mouse_down = false;
static bool paint_rect = false;
static TERM_LEN mousex = 0, mousey = 0;
static TERM_LEN oldx, oldy;

/*
 * Hack -- define which keys are "special"
 */
static bool special_key[256];
static bool ignore_key[256];

/*
 * Hack -- initialization list for "special_key"
 */
static byte special_key_list[] = {
    VK_CLEAR, VK_PAUSE, VK_CAPITAL, VK_KANA, VK_JUNJA, VK_FINAL, VK_KANJI, VK_CONVERT, VK_NONCONVERT, VK_ACCEPT, VK_MODECHANGE, VK_PRIOR, VK_NEXT, VK_END,
    VK_HOME, VK_LEFT, VK_UP, VK_RIGHT, VK_DOWN, VK_SELECT, VK_PRINT, VK_EXECUTE, VK_SNAPSHOT, VK_INSERT, VK_DELETE, VK_HELP, VK_APPS, VK_NUMPAD0, VK_NUMPAD1,
    VK_NUMPAD2, VK_NUMPAD3, VK_NUMPAD4, VK_NUMPAD5, VK_NUMPAD6, VK_NUMPAD7, VK_NUMPAD8, VK_NUMPAD9, VK_MULTIPLY, VK_ADD, VK_SEPARATOR, VK_SUBTRACT, VK_DECIMAL,
    VK_DIVIDE, VK_F1, VK_F2, VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8, VK_F9, VK_F10, VK_F11, VK_F12, VK_F13, VK_F14, VK_F15, VK_F16, VK_F17, VK_F18, VK_F19,
    VK_F20, VK_F21, VK_F22, VK_F23, VK_F24, VK_NUMLOCK, VK_SCROLL, VK_ATTN, VK_CRSEL, VK_EXSEL, VK_EREOF, VK_PLAY, VK_ZOOM, VK_NONAME, VK_PA1,
    0 /* End of List */
};

static byte ignore_key_list[] = {
    VK_ESCAPE, VK_TAB, VK_SPACE, 'F', 'W', 'O', /*'H',*/ /* these are menu characters.*/
    VK_SHIFT, VK_CONTROL, VK_MENU, VK_LWIN, VK_RWIN, VK_LSHIFT, VK_RSHIFT, VK_LCONTROL, VK_RCONTROL, VK_LMENU, VK_RMENU, 0 /* End of List */
};

/*!
 * @brief Validate a file
 */
static void validate_file(const std::filesystem::path &s)
{
    if (std::filesystem::is_regular_file(s)) {
        return;
    }

    const auto &file = s.string();
    quit_fmt(_("必要なファイル[%s]が見あたりません。", "Cannot find required file:\n%s"), file.data());
}

/*!
 * @brief Validate a directory
 */
static void validate_dir(const std::filesystem::path &s, bool vital)
{
    if (std::filesystem::is_directory(s)) {
        return;
    }

    const auto &dir = s.string();
    if (vital) {
        quit_fmt(_("必要なディレクトリ[%s]が見あたりません。", "Cannot find required directory:\n%s"), dir.data());
    } else if (mkdir(dir.data())) {
        quit_fmt("Unable to create directory:\n%s", dir.data());
    }
}

/*!
 * @brief (Windows版固有実装)Get the "size" for a window
 */
static void term_getsize(term_data *td)
{
    if (td->cols < 1) {
        td->cols = 1;
    }
    if (td->rows < 1) {
        td->rows = 1;
    }

    TERM_LEN wid = td->cols * td->tile_wid + td->size_ow1 + td->size_ow2;
    TERM_LEN hgt = td->rows * td->tile_hgt + td->size_oh1 + td->size_oh2;

    RECT rw, rc;
    if (td->w) {
        GetWindowRect(td->w, &rw);
        GetClientRect(td->w, &rc);

        td->size_wid = (rw.right - rw.left) - (rc.right - rc.left) + wid;
        td->size_hgt = (rw.bottom - rw.top) - (rc.bottom - rc.top) + hgt;

        td->pos_x = rw.left;
        td->pos_y = rw.top;
    } else {
        /* Tempolary calculation */
        rc.left = 0;
        rc.right = wid;
        rc.top = 0;
        rc.bottom = hgt;
        AdjustWindowRectEx(&rc, td->dwStyle, TRUE, td->dwExStyle);
        td->size_wid = rc.right - rc.left;
        td->size_hgt = rc.bottom - rc.top;
    }
}

/*!
 * @brief Write the "prefs" for a single term
 */
static void save_prefs_aux(int i)
{
    term_data *td = &data[i];
    GAME_TEXT sec_name[128];
    char buf[1024];

    if (!td->w) {
        return;
    }

    wsprintfA(sec_name, "Term-%d", i);
    const auto &str_ini_file = path_ini_file.string();
    const auto *ini_file = str_ini_file.data();
    if (i > 0) {
        strcpy(buf, td->visible ? "1" : "0");
        WritePrivateProfileStringA(sec_name, "Visible", buf, ini_file);
    }

    auto pwchar = td->lf.lfFaceName[0] != '\0' ? td->lf.lfFaceName : _(L"ＭＳ ゴシック", L"Courier");
    WritePrivateProfileStringA(sec_name, "Font", to_multibyte(pwchar).c_str(), ini_file);

    wsprintfA(buf, "%d", td->lf.lfWidth);
    WritePrivateProfileStringA(sec_name, "FontWid", buf, ini_file);
    wsprintfA(buf, "%d", td->lf.lfHeight);
    WritePrivateProfileStringA(sec_name, "FontHgt", buf, ini_file);
    wsprintfA(buf, "%d", td->lf.lfWeight);
    WritePrivateProfileStringA(sec_name, "FontWgt", buf, ini_file);

    wsprintfA(buf, "%d", td->tile_wid);
    WritePrivateProfileStringA(sec_name, "TileWid", buf, ini_file);

    wsprintfA(buf, "%d", td->tile_hgt);
    WritePrivateProfileStringA(sec_name, "TileHgt", buf, ini_file);

    WINDOWPLACEMENT lpwndpl{};
    lpwndpl.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(td->w, &lpwndpl);

    RECT rc = lpwndpl.rcNormalPosition;
    if (i == 0) {
        wsprintfA(buf, "%d", normsize.x);
    } else {
        wsprintfA(buf, "%d", td->cols);
    }

    WritePrivateProfileStringA(sec_name, "NumCols", buf, ini_file);

    if (i == 0) {
        wsprintfA(buf, "%d", normsize.y);
    } else {
        wsprintfA(buf, "%d", td->rows);
    }

    WritePrivateProfileStringA(sec_name, "NumRows", buf, ini_file);
    if (i == 0) {
        strcpy(buf, IsZoomed(td->w) ? "1" : "0");
        WritePrivateProfileStringA(sec_name, "Maximized", buf, ini_file);
    }

    GetWindowRect(td->w, &rc);
    wsprintfA(buf, "%d", rc.left);
    WritePrivateProfileStringA(sec_name, "PositionX", buf, ini_file);

    wsprintfA(buf, "%d", rc.top);
    WritePrivateProfileStringA(sec_name, "PositionY", buf, ini_file);
    if (i > 0) {
        strcpy(buf, td->posfix ? "1" : "0");
        WritePrivateProfileStringA(sec_name, "PositionFix", buf, ini_file);
    }
}

/*!
 * @brief Write the "prefs"
 * We assume that the windows have all been initialized
 */
static void save_prefs()
{
    const auto &str_ini_file = path_ini_file.string();
    const auto *ini_file = str_ini_file.data();
    char buf[128];
    wsprintfA(buf, "%d", arg_graphics);
    WritePrivateProfileStringA("Angband", "Graphics", buf, ini_file);

    strcpy(buf, arg_bigtile ? "1" : "0");
    WritePrivateProfileStringA("Angband", "Bigtile", buf, ini_file);

    strcpy(buf, arg_sound ? "1" : "0");
    WritePrivateProfileStringA("Angband", "Sound", buf, ini_file);
    WritePrivateProfileStringA("Angband", "SoundVolumeTableIndex", std::to_string(arg_sound_volume_table_index).data(), ini_file);

    strcpy(buf, arg_music ? "1" : "0");
    WritePrivateProfileStringA("Angband", "Music", buf, ini_file);
    strcpy(buf, use_pause_music_inactive ? "1" : "0");
    WritePrivateProfileStringA("Angband", "MusicVolumeTableIndex", std::to_string(arg_music_volume_table_index).data(), ini_file);
    WritePrivateProfileStringA("Angband", "MusicPauseInactive", buf, ini_file);

    wsprintfA(buf, "%d", current_bg_mode);
    WritePrivateProfileStringA("Angband", "BackGround", buf, ini_file);
    const auto &wallpaper_filename = wallpaper_path.string();
    WritePrivateProfileStringA("Angband", "BackGroundBitmap", !wallpaper_path.empty() ? wallpaper_filename.data() : DEFAULT_BG_FILENAME, ini_file);

    auto angband_dir_str = ANGBAND_DIR.string();
    const auto path_length = angband_dir_str.length() - 4; // "\lib" を除く.
    angband_dir_str = angband_dir_str.substr(0, path_length);
    const auto savefile_str = savefile.string();
    const auto savefile_dir_str = savefile_str.substr(0, path_length);
    if (angband_dir_str == savefile_dir_str) {
        const auto relative_path = format(".\\%s", (savefile_str.data() + path_length));
        WritePrivateProfileStringA("Angband", "SaveFile", relative_path.data(), ini_file);
    } else {
        WritePrivateProfileStringA("Angband", "SaveFile", savefile_str.data(), ini_file);
    }

    strcpy(buf, keep_subwindows ? "1" : "0");
    WritePrivateProfileStringA("Angband", "KeepSubwindows", buf, ini_file);

    for (int i = 0; i < MAX_TERM_DATA; ++i) {
        save_prefs_aux(i);
    }
}

/*!
 * @brief callback for EnumDisplayMonitors API
 */
static BOOL CALLBACK monitor_enum_procedure([[maybe_unused]] HMONITOR hMon, [[maybe_unused]] HDC hdcMon, [[maybe_unused]] LPRECT lpMon, LPARAM dwDate)
{
    bool *result = (bool *)dwDate;
    *result = true;
    return false;
}

/*!
 * @brief Load the "prefs" for a single term
 */
static void load_prefs_aux(int i)
{
    term_data *td = &data[i];
    GAME_TEXT sec_name[128];
    wsprintfA(sec_name, "Term-%d", i);
    const auto &str_ini_file = path_ini_file.string();
    const auto *ini_file = str_ini_file.data();
    if (i > 0) {
        td->visible = (GetPrivateProfileIntA(sec_name, "Visible", td->visible, ini_file) != 0);
    }

    char tmp[1024]{};
    GetPrivateProfileStringA(sec_name, "Font", _("ＭＳ ゴシック", "Courier"), tmp, 127, ini_file);
    td->font_want = tmp;

    const auto hgt = 15;
    const auto wid = 0;
    td->lf.lfWidth = GetPrivateProfileIntA(sec_name, "FontWid", wid, ini_file);
    td->lf.lfHeight = GetPrivateProfileIntA(sec_name, "FontHgt", hgt, ini_file);
    td->lf.lfWeight = GetPrivateProfileIntA(sec_name, "FontWgt", 0, ini_file);

    td->tile_wid = GetPrivateProfileIntA(sec_name, "TileWid", td->lf.lfWidth, ini_file);
    td->tile_hgt = GetPrivateProfileIntA(sec_name, "TileHgt", td->lf.lfHeight, ini_file);

    td->cols = GetPrivateProfileIntA(sec_name, "NumCols", td->cols, ini_file);
    td->rows = GetPrivateProfileIntA(sec_name, "NumRows", td->rows, ini_file);
    normsize.x = td->cols;
    normsize.y = td->rows;

    if (i == 0) {
        win_maximized = (GetPrivateProfileIntA(sec_name, "Maximized", win_maximized, ini_file) != 0);
    }

    int posx = GetPrivateProfileIntA(sec_name, "PositionX", 0, ini_file);
    int posy = GetPrivateProfileIntA(sec_name, "PositionY", 0, ini_file);
    // 保存座標がモニタ内の領域にあるかチェック
    RECT rect = { posx, posy, posx + 128, posy + 128 };
    bool in_any_monitor = false;
    ::EnumDisplayMonitors(NULL, &rect, monitor_enum_procedure, (LPARAM)&in_any_monitor);
    if (in_any_monitor) {
        // いずれかのモニタに表示可能、ウインドウ位置を復元
        td->pos_x = posx;
        td->pos_y = posy;
    }

    if (i > 0) {
        td->posfix = (GetPrivateProfileIntA(sec_name, "PositionFix", td->posfix, ini_file) != 0);
    }
}

/*!
 * @brief Load the "prefs"
 */
static void load_prefs()
{
    const auto &str_ini_file = path_ini_file.string();
    const auto *ini_file = str_ini_file.data();
    arg_graphics = (byte)GetPrivateProfileIntA("Angband", "Graphics", enum2i(graphics_mode::GRAPHICS_NONE), ini_file);
    arg_bigtile = (GetPrivateProfileIntA("Angband", "Bigtile", false, ini_file) != 0);
    use_bigtile = arg_bigtile;
    arg_sound = (GetPrivateProfileIntA("Angband", "Sound", 0, ini_file) != 0);
    arg_sound_volume_table_index = std::clamp<int>(GetPrivateProfileIntA("Angband", "SoundVolumeTableIndex", 0, ini_file), 0, SOUND_VOLUME_TABLE.size() - 1);
    arg_music = (GetPrivateProfileIntA("Angband", "Music", 0, ini_file) != 0);
    arg_music_volume_table_index = std::clamp<int>(GetPrivateProfileIntA("Angband", "MusicVolumeTableIndex", 0, ini_file), 0, main_win_music::VOLUME_TABLE.size() - 1);
    use_pause_music_inactive = (GetPrivateProfileIntA("Angband", "MusicPauseInactive", 0, ini_file) != 0);
    current_bg_mode = static_cast<bg_mode>(GetPrivateProfileIntA("Angband", "BackGround", 0, ini_file));
    char wallpaper_buf[1024]{};
    GetPrivateProfileStringA("Angband", "BackGroundBitmap", DEFAULT_BG_FILENAME, wallpaper_buf, 1023, ini_file);
    wallpaper_path = wallpaper_buf;
    char savefile_buf[1024]{};
    GetPrivateProfileStringA("Angband", "SaveFile", "", savefile_buf, 1023, ini_file);
    const auto loaded_save_path = std::filesystem::path(savefile_buf).lexically_normal();
    if (loaded_save_path.is_relative()) {
        const auto angband_path = ANGBAND_DIR.parent_path().parent_path(); // remove "\\lib\\"
        savefile = angband_path / loaded_save_path;
    } else {
        savefile = loaded_save_path;
    }

    keep_subwindows = (GetPrivateProfileIntA("Angband", "KeepSubwindows", 0, ini_file) != 0);
    for (int i = 0; i < MAX_TERM_DATA; ++i) {
        load_prefs_aux(i);
    }
}

/*!
 * @brief Initialize music
 */
static void init_music()
{
    // Flag set once "music" has been initialized
    static bool can_use_music = false;

    if (!can_use_music) {
        main_win_music::load_music_prefs();
        can_use_music = true;
    }
}

/*!
 * @brief Initialize sound
 */
static void init_sound()
{
    // Flag set once "sound" has been initialized
    static bool can_use_sound = false;

    if (!can_use_sound) {
        load_sound_prefs();
        can_use_sound = true;
    }
}

/*!
 * @brief Change sound mode
 * @param new_mode bool
 */
static void change_sound_mode(bool new_mode)
{
    use_sound = new_mode;
    if (use_sound) {
        init_sound();
    }
}

/*!
 * @brief Initialize background
 */
static void init_background()
{
    // Flag set once "background" has been initialized
    static bool can_use_background = false;

    if (!can_use_background) {
        load_bg_prefs();
        can_use_background = true;
    }
}

/*!
 * @brief Change background mode
 * @param new_mode bg_mode
 * @param show_error trueに設定した場合のみ、エラーダイアログを表示する
 * @param force_redraw trueの場合、モード変更に関わらずウインドウを再描画する
 * @retval true success
 * @retval false failed
 */
static bool change_bg_mode(bg_mode new_mode, bool show_error = false, bool force_redraw = false)
{
    bg_mode old_bg_mode = current_bg_mode;
    current_bg_mode = new_mode;
    if (current_bg_mode != bg_mode::BG_NONE) {
        init_background();
        if (!load_bg(wallpaper_path)) {
            current_bg_mode = bg_mode::BG_NONE;
            if (show_error) {
                const auto &wallaper_filename = wallpaper_path.string();
                plog_fmt(_("壁紙用ファイル '%s' を読み込めません。", "Can't load the image file '%s'."), wallaper_filename.data());
            }
        }
    } else {
        delete_bg();
    }

    const bool mode_changed = (current_bg_mode != old_bg_mode);
    if (mode_changed || force_redraw) {
        // 全ウインドウ再描画
        term_type *old = game_term;
        for (int i = 0; i < MAX_TERM_DATA; i++) {
            term_data *td = &data[i];
            if (td->visible) {
                term_activate(&td->t);
                term_redraw();
            }
        }
        term_activate(old);
    }

    return current_bg_mode == new_mode;
}

/*!
 * @brief Resize a window
 */
static void term_window_resize(term_data *td)
{
    if (!td->w) {
        return;
    }

    SetWindowPos(td->w, 0, 0, 0, td->size_wid, td->size_hgt, SWP_NOMOVE | SWP_NOZORDER);
    if (!td->size_hack) {
        td->dispose_offscreen();
        term_activate(&td->t);
        term_redraw();
    }
}

/*!
 * @brief Force the use of a new font for a term_data.
 * This function may be called before the "window" is ready.
 * This function returns zero only if everything succeeds.
 * @note that the "font name" must be capitalized!!!
 */
static errr term_force_font(term_data *td)
{
    if (td->font_id) {
        DeleteObject(td->font_id);
    }

    td->font_id = CreateFontIndirectW(&(td->lf));
    int wid = td->lf.lfWidth;
    int hgt = td->lf.lfHeight;
    if (!td->font_id) {
        return 1;
    }

    if (!wid || !hgt) {
        HDC hdcDesktop;
        HFONT hfOld;
        TEXTMETRIC tm;

        hdcDesktop = GetDC(HWND_DESKTOP);
        hfOld = static_cast<HFONT>(SelectObject(hdcDesktop, td->font_id));
        GetTextMetrics(hdcDesktop, &tm);
        SelectObject(hdcDesktop, hfOld);
        ReleaseDC(HWND_DESKTOP, hdcDesktop);

        wid = tm.tmAveCharWidth;
        hgt = tm.tmHeight;
    }

    td->font_wid = wid;
    td->font_hgt = hgt;

    return 0;
}

/*!
 * @brief Allow the user to change the font for this window.
 */
static void term_change_font(term_data *td)
{
    CHOOSEFONTW cf{};
    cf.lStructSize = sizeof(cf);
    cf.Flags = CF_SCREENFONTS | CF_FIXEDPITCHONLY | CF_NOVERTFONTS | CF_INITTOLOGFONTSTRUCT;
    cf.lpLogFont = &(td->lf);

    if (!ChooseFontW(&cf)) {
        return;
    }

    term_force_font(td);
    td->tile_wid = td->font_wid;
    td->tile_hgt = td->font_hgt;
    term_getsize(td);
    term_window_resize(td);
}

/*!
 * @brief Allow the user to lock this window.
 */
static void term_window_pos(term_data *td, HWND hWnd)
{
    SetWindowPos(td->w, hWnd, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
}

/*!
 * @brief Hack -- redraw a term_data
 */
static void term_data_redraw(term_data *td)
{
    term_activate(&td->t);
    term_redraw();
    term_activate(term_screen);
}

/*!
 * @brief termの反転色表示
 */
static void term_inversed_area(HWND hWnd, int x, int y, int w, int h)
{
    term_data *td = (term_data *)GetWindowLong(hWnd, 0);
    int tx = td->size_ow1 + x * td->tile_wid;
    int ty = td->size_oh1 + y * td->tile_hgt;
    int tw = w * td->tile_wid - 1;
    int th = h * td->tile_hgt - 1;

    HDC hdc = td->get_hdc();
    HBRUSH myBrush = CreateSolidBrush(RGB(255, 255, 255));
    HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(hdc, myBrush));
    HPEN oldPen = static_cast<HPEN>(SelectObject(hdc, GetStockObject(NULL_PEN)));

    PatBlt(hdc, tx, ty, tw, th, PATINVERT);

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);

    RECT rect{ tx, ty, tx + tw, ty + th };
    td->refresh(&rect);
}

/*!
 * @brief Windows版ユーザ設定項目実装部(実装必須) /Interact with the User
 */
static errr term_user_win(int n)
{
    (void)n;
    return 0;
}

/*!
 * @brief カラーパレットの変更？
 */
static void refresh_color_table()
{
    for (int i = 0; i < 256; i++) {
        byte rv = angband_color_table[i][1];
        byte gv = angband_color_table[i][2];
        byte bv = angband_color_table[i][3];
        win_clr[i] = PALETTERGB(rv, gv, bv);
    }
}

/*!
 * @brief グラフィクスのモード変更
 */
static void change_graphics_mode(graphics_mode mode)
{
    graphics_mode ret = graphic.change_graphics(mode);
    if (ret != mode) {
        plog(_("グラフィクスを初期化できません!", "Cannot initialize graphics!"));
    }
    arg_graphics = static_cast<byte>(ret);
    use_graphics = (arg_graphics > 0);
}

/*!
 * @brief ターミナルのサイズ更新
 * @details 行数、列数の変更に対応する。
 * @param td term_dataのポインタ
 * @param resize_window trueの場合に再計算されたウインドウサイズにリサイズする
 */
static void rebuild_term(term_data *td, bool resize_window = true)
{
    term_type *old = game_term;
    td->size_hack = true;
    term_activate(&td->t);
    term_getsize(td);
    if (resize_window) {
        term_window_resize(td);
    }
    td->dispose_offscreen();
    term_resize(td->cols, td->rows);
    td->size_hack = false;
    term_activate(old);
}

/*!
 * @brief React to global changes
 */
static errr term_xtra_win_react(PlayerType *player_ptr)
{
    refresh_color_table();

    const byte current_mode = static_cast<byte>(graphic.get_mode());
    if (current_mode != arg_graphics) {
        change_graphics_mode(static_cast<graphics_mode>(arg_graphics));
        reset_visuals(player_ptr);
    }

    for (int i = 0; i < MAX_TERM_DATA; i++) {
        term_data *td = &data[i];
        if ((td->cols != td->t.wid) || (td->rows != td->t.hgt)) {
            rebuild_term(td);
        }
    }

    return 0;
}

/*!
 * @brief Process at least one event
 */
static errr term_xtra_win_event(int v)
{
    MSG msg;
    if (v) {
        if (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    } else {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return 0;
}

/*!
 * @brief Process all pending events
 */
static errr term_xtra_win_flush()
{
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

/*!
 * @brief Hack -- clear the screen
 * @details
 * Make this more efficient
 */
static errr term_xtra_win_clear()
{
    term_data *td = (term_data *)(game_term->data);

    RECT rc;
    GetClientRect(td->w, &rc);

    HDC hdc = td->get_hdc();
    SetBkColor(hdc, RGB(0, 0, 0));
    SelectObject(hdc, td->font_id);
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

    if (current_bg_mode != bg_mode::BG_NONE) {
        rc.left = 0;
        rc.top = 0;
        draw_bg(hdc, &rc);
    }

    td->refresh();
    return 0;
}

/*!
 * @brief Hack -- make a noise
 */
static errr term_xtra_win_noise()
{
    MessageBeep(MB_ICONASTERISK);
    return 0;
}

/*!
 * @brief Hack -- make a sound
 */
static errr term_xtra_win_sound(int v)
{
    if (!use_sound) {
        return 1;
    }
    return play_sound(v, SOUND_VOLUME_TABLE[arg_sound_volume_table_index]);
}

/*!
 * @brief Hack -- play a music
 */
static bool term_xtra_win_music(int n, int v)
{
    if (!use_music) {
        return false;
    }

    return main_win_music::play_music(n, v);
}

/*!
 * @brief Hack -- play a music matches a situation
 */
static bool term_xtra_win_scene(int v)
{
    // TODO 場面に合った壁紙変更対応
    if (!use_music) {
        return false;
    }

    main_win_music::play_music_scene(v);
    return true;
}

/*!
 * @brief Delay for "x" milliseconds
 */
static int term_xtra_win_delay(int v)
{
    Sleep(v);
    return 0;
}

/*!
 * @brief Do a "special thing"
 * @todo z-termに影響があるのでPlayerTypeの追加は保留
 */
static errr term_xtra_win(int n, int v)
{
    switch (n) {
    case TERM_XTRA_NOISE: {
        return term_xtra_win_noise();
    }
    case TERM_XTRA_FRESH: {
        term_data *td = (term_data *)(game_term->data);
        if (td->w) {
            UpdateWindow(td->w);
        }
        return 0;
    }
    case TERM_XTRA_MUSIC_BASIC:
    case TERM_XTRA_MUSIC_DUNGEON:
    case TERM_XTRA_MUSIC_QUEST:
    case TERM_XTRA_MUSIC_TOWN:
    case TERM_XTRA_MUSIC_MONSTER: {
        return term_xtra_win_music(n, v) ? 0 : 1;
    }
    case TERM_XTRA_MUSIC_MUTE:
        main_win_music::stop_music();
        return 0;
    case TERM_XTRA_SCENE: {
        return term_xtra_win_scene(v) ? 0 : 1;
    }
    case TERM_XTRA_SOUND: {
        return term_xtra_win_sound(v);
    }
    case TERM_XTRA_BORED: {
        return term_xtra_win_event(0);
    }
    case TERM_XTRA_EVENT: {
        return term_xtra_win_event(v);
    }
    case TERM_XTRA_FLUSH: {
        return term_xtra_win_flush();
    }
    case TERM_XTRA_CLEAR: {
        return term_xtra_win_clear();
    }
    case TERM_XTRA_REACT: {
        return term_xtra_win_react(p_ptr);
    }
    case TERM_XTRA_DELAY: {
        return term_xtra_win_delay(v);
    }
    }

    return 1;
}

/*!
 * @brief Low level graphics (Assumes valid input).
 * @details
 * Draw a "cursor" at (x,y), using a "yellow box".
 */
static errr term_curs_win(int x, int y)
{
    term_data *td = (term_data *)(game_term->data);
    int tile_wid, tile_hgt;
    tile_wid = td->tile_wid;
    tile_hgt = td->tile_hgt;

    RECT rc{};
    rc.left = x * tile_wid + td->size_ow1;
    rc.right = rc.left + tile_wid;
    rc.top = y * tile_hgt + td->size_oh1;
    rc.bottom = rc.top + tile_hgt;

    HDC hdc = td->get_hdc();
    FrameRect(hdc, &rc, hbrYellow);
    td->refresh(&rc);
    return 0;
}

/*!
 * @brief Low level graphics (Assumes valid input).
 * @details
 * Draw a "big cursor" at (x,y), using a "yellow box".
 */
static errr term_bigcurs_win(int x, int y)
{
    term_data *td = (term_data *)(game_term->data);
    int tile_wid, tile_hgt;
    tile_wid = td->tile_wid;
    tile_hgt = td->tile_hgt;

    RECT rc{};
    rc.left = x * tile_wid + td->size_ow1;
    rc.right = rc.left + 2 * tile_wid;
    rc.top = y * tile_hgt + td->size_oh1;
    rc.bottom = rc.top + tile_hgt;

    HDC hdc = td->get_hdc();
    FrameRect(hdc, &rc, hbrYellow);
    td->refresh(&rc);
    return 0;
}

/*!
 * @brief Low level graphics (Assumes valid input).
 * @details
 * Erase a "block" of "n" characters starting at (x,y).
 */
static errr term_wipe_win(int x, int y, int n)
{
    term_data *td = (term_data *)(game_term->data);
    RECT rc{};
    rc.left = x * td->tile_wid + td->size_ow1;
    rc.right = rc.left + n * td->tile_wid;
    rc.top = y * td->tile_hgt + td->size_oh1;
    rc.bottom = rc.top + td->tile_hgt;

    HDC hdc = td->get_hdc();
    SetBkColor(hdc, RGB(0, 0, 0));
    SelectObject(hdc, td->font_id);
    if (current_bg_mode != bg_mode::BG_NONE) {
        draw_bg(hdc, &rc);
    } else {
        ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
    }

    td->refresh(&rc);
    return 0;
}

/*!
 * @brief Low level graphics.  Assumes valid input.
 * @details
 * Draw several ("n") chars, with an attr, at a given location.
 *
 * All "graphic" data is handled by "term_pict_win()", below.
 *
 * One would think there is a more efficient method for telling a window
 * what color it should be using to draw with, but perhaps simply changing
 * it every time is not too inefficient.
 */
static errr term_text_win(int x, int y, int n, TERM_COLOR a, concptr s)
{
    term_data *td = (term_data *)(game_term->data);
    static HBITMAP WALL;
    static HBRUSH myBrush, oldBrush;
    static HPEN oldPen;
    static bool init_done = false;

    if (!init_done) {
        WALL = LoadBitmapW(hInstance, AppName);
        myBrush = CreatePatternBrush(WALL);
        init_done = true;
    }

    RECT rc{ static_cast<LONG>(x * td->tile_wid + td->size_ow1), static_cast<LONG>(y * td->tile_hgt + td->size_oh1),
        static_cast<LONG>(rc.left + n * td->tile_wid), static_cast<LONG>(rc.top + td->tile_hgt) };
    RECT rc_start = rc;

    HDC hdc = td->get_hdc();
    SetBkColor(hdc, RGB(0, 0, 0));
    SetTextColor(hdc, win_clr[a]);

    SelectObject(hdc, td->font_id);
    if (current_bg_mode != bg_mode::BG_NONE) {
        SetBkMode(hdc, TRANSPARENT);
    }

    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
    if (current_bg_mode != bg_mode::BG_NONE) {
        draw_bg(hdc, &rc);
    }

    rc.left += ((td->tile_wid - td->font_wid) / 2);
    rc.right = rc.left + td->font_wid;
    rc.top += ((td->tile_hgt - td->font_hgt) / 2);
    rc.bottom = rc.top + td->font_hgt;

    for (int i = 0; i < n; i++) {
#ifdef JP
        if (use_bigtile && *(s + i) == "■"[0] && *(s + i + 1) == "■"[1]) {
            rc.right += td->font_wid;
            oldBrush = static_cast<HBRUSH>(SelectObject(hdc, myBrush));
            oldPen = static_cast<HPEN>(SelectObject(hdc, GetStockObject(NULL_PEN)));
            Rectangle(hdc, rc.left, rc.top, rc.right + 1, rc.bottom + 1);
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            rc.right -= td->font_wid;
            i++;
            rc.left += 2 * td->tile_wid;
            rc.right += 2 * td->tile_wid;
        } else if (iskanji(*(s + i))) /* 2バイト文字 */
        {
            to_wchar wc({ s + i, 2 });
            const auto *buf = wc.wc_str();
            rc.right += td->font_wid;
            if (buf == NULL) {
                ExtTextOutA(hdc, rc.left, rc.top, ETO_CLIPPED, &rc, s + i, 2, NULL);
            } else {
                ExtTextOutW(hdc, rc.left, rc.top, ETO_CLIPPED, &rc, buf, wcslen(buf), NULL);
            }
            rc.right -= td->font_wid;
            i++;
            rc.left += 2 * td->tile_wid;
            rc.right += 2 * td->tile_wid;
        } else if (*(s + i) == 127) {
            oldBrush = static_cast<HBRUSH>(SelectObject(hdc, myBrush));
            oldPen = static_cast<HPEN>(SelectObject(hdc, GetStockObject(NULL_PEN)));
            Rectangle(hdc, rc.left, rc.top, rc.right + 1, rc.bottom + 1);
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            rc.left += td->tile_wid;
            rc.right += td->tile_wid;
        } else {
            ExtTextOutA(hdc, rc.left, rc.top, ETO_CLIPPED, &rc, s + i, 1, NULL);
            rc.left += td->tile_wid;
            rc.right += td->tile_wid;
        }
#else
        if (*(s + i) == 127) {
            oldBrush = static_cast<HBRUSH>(SelectObject(hdc, myBrush));
            oldPen = static_cast<HPEN>(SelectObject(hdc, GetStockObject(NULL_PEN)));
            Rectangle(hdc, rc.left, rc.top, rc.right + 1, rc.bottom + 1);
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            rc.left += td->tile_wid;
            rc.right += td->tile_wid;
        } else {
            ExtTextOutA(hdc, rc.left, rc.top, ETO_CLIPPED, &rc, s + i, 1, NULL);
            rc.left += td->tile_wid;
            rc.right += td->tile_wid;
        }
#endif
    }

    rc.left = rc_start.left;
    rc.top = rc_start.top;
    td->refresh(&rc);
    return 0;
}

/*!
 * @brief Low level graphics.  Assumes valid input.
 * @details
 * Draw an array of "special" attr/char pairs at the given location.
 *
 * We use the "term_pict_win()" function for "graphic" data, which are
 * encoded by setting the "high-bits" of both the "attr" and the "char"
 * data.  We use the "attr" to represent the "row" of the main bitmap,
 * and the "char" to represent the "col" of the main bitmap.  The use
 * of this function is induced by the "higher_pict" flag.
 *
 * If "graphics" is not available, we simply "wipe" the given grids.
 */
static errr term_pict_win(TERM_LEN x, TERM_LEN y, int n, const TERM_COLOR *ap, concptr cp, const TERM_COLOR *tap, concptr tcp)
{
    term_data *td = (term_data *)(game_term->data);
    int i;
    HDC hdcMask = NULL;
    if (!use_graphics) {
        return term_wipe_win(x, y, n);
    }

    const tile_info &infGraph = graphic.get_tile_info();
    const bool has_mask = (infGraph.hBitmapMask != NULL);
    TERM_LEN w1 = infGraph.CellWidth;
    TERM_LEN h1 = infGraph.CellHeight;
    TERM_LEN tw1 = infGraph.TileWidth;
    TERM_LEN th1 = infGraph.TileHeight;
    TERM_LEN w2, h2, tw2 = 0;
    w2 = td->tile_wid;
    h2 = td->tile_hgt;
    tw2 = w2;
    if (use_bigtile) {
        tw2 *= 2;
    }

    TERM_LEN x2 = x * w2 + td->size_ow1 + infGraph.OffsetX;
    TERM_LEN y2 = y * h2 + td->size_oh1 + infGraph.OffsetY;
    HDC hdc = td->get_hdc();
    HDC hdcSrc = CreateCompatibleDC(hdc);
    HBITMAP hbmSrcOld = static_cast<HBITMAP>(SelectObject(hdcSrc, infGraph.hBitmap));

    if (has_mask) {
        hdcMask = CreateCompatibleDC(hdc);
        SelectObject(hdcMask, infGraph.hBitmapMask);
    }

    for (i = 0; i < n; i++, x2 += w2) {
        TERM_COLOR a = ap[i];
        char c = cp[i];
        int row = (a & 0x7F);
        int col = (c & 0x7F);
        TERM_LEN x1 = col * w1;
        TERM_LEN y1 = row * h1;

        if (has_mask) {
            TERM_LEN x3 = (tcp[i] & 0x7F) * w1;
            TERM_LEN y3 = (tap[i] & 0x7F) * h1;
            tw2 = tw2 * w1 / tw1;
            h2 = h2 * h1 / th1;
            if ((tw1 == tw2) && (th1 == h2)) {
                BitBlt(hdc, x2, y2, tw2, h2, hdcSrc, x3, y3, SRCCOPY);
                BitBlt(hdc, x2, y2, tw2, h2, hdcMask, x1, y1, SRCAND);
                BitBlt(hdc, x2, y2, tw2, h2, hdcSrc, x1, y1, SRCPAINT);
                continue;
            }

            SetStretchBltMode(hdc, COLORONCOLOR);
            StretchBlt(hdc, x2, y2, tw2, h2, hdcMask, x3, y3, w1, h1, SRCAND);
            StretchBlt(hdc, x2, y2, tw2, h2, hdcSrc, x3, y3, w1, h1, SRCPAINT);
            if ((x1 != x3) || (y1 != y3)) {
                StretchBlt(hdc, x2, y2, tw2, h2, hdcMask, x1, y1, w1, h1, SRCAND);
                StretchBlt(hdc, x2, y2, tw2, h2, hdcSrc, x1, y1, w1, h1, SRCPAINT);
            }

            continue;
        }

        if ((w1 == tw2) && (h1 == h2)) {
            BitBlt(hdc, x2, y2, tw2, h2, hdcSrc, x1, y1, SRCCOPY);
            continue;
        }

        SetStretchBltMode(hdc, COLORONCOLOR);
        StretchBlt(hdc, x2, y2, tw2, h2, hdcSrc, x1, y1, w1, h1, SRCCOPY);
    }

    SelectObject(hdcSrc, hbmSrcOld);
    DeleteDC(hdcSrc);
    if (has_mask) {
        SelectObject(hdcMask, hbmSrcOld);
        DeleteDC(hdcMask);
    }

    td->refresh();
    return 0;
}

/*!
 * @brief Create and initialize a "term_data" given a title
 */
static void term_data_link(term_data *td)
{
    term_type *t = &td->t;
    term_init(t, td->cols, td->rows, FILE_READ_BUFF_SIZE);
    t->soft_cursor = true;
    t->higher_pict = true;
    t->attr_blank = TERM_WHITE;
    t->char_blank = ' ';
    t->user_hook = term_user_win;
    t->xtra_hook = term_xtra_win;
    t->curs_hook = term_curs_win;
    t->bigcurs_hook = term_bigcurs_win;
    t->wipe_hook = term_wipe_win;
    t->text_hook = term_text_win;
    t->pict_hook = term_pict_win;
    t->data = (vptr)(td);
}

/*!
 * @brief Create the windows
 * @details
 * First, instantiate the "default" values, then read the "ini_file"
 * to over-ride selected values, then create the windows, and fonts.
 *
 * Must use SW_SHOW not SW_SHOWNA, since on 256 color display
 * must make active to realize the palette.
 */
static void init_windows()
{
    term_data *td;
    td = &data[0];
    *td = {};
    td->name = win_term_name[0];

    td->rows = MAIN_TERM_MIN_ROWS;
    td->cols = MAIN_TERM_MIN_COLS;
    td->visible = true;
    td->size_ow1 = 2;
    td->size_ow2 = 2;
    td->size_oh1 = 2;
    td->size_oh2 = 2;
    td->pos_x = 7 * 30;
    td->pos_y = 7 * 20;
    td->posfix = false;

    for (int i = 1; i < MAX_TERM_DATA; i++) {
        td = &data[i];
        *td = {};
        td->name = win_term_name[i];
        td->rows = TERM_DEFAULT_ROWS;
        td->cols = TERM_DEFAULT_COLS;
        td->visible = false;
        td->size_ow1 = 1;
        td->size_ow2 = 1;
        td->size_oh1 = 1;
        td->size_oh2 = 1;
        td->pos_x = (7 - i) * 30;
        td->pos_y = (7 - i) * 20;
        td->posfix = false;
    }

    load_prefs();

    /* Atrributes of main window */
    td = &data[0];
    td->dwStyle = (WS_OVERLAPPED | WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CAPTION | WS_VISIBLE);
    td->dwExStyle = 0;
    td->visible = true;

    /* Attributes of sub windows */
    for (int i = 1; i < MAX_TERM_DATA; i++) {
        td = &data[i];
        td->dwStyle = (WS_OVERLAPPED | WS_THICKFRAME | WS_SYSMENU);
        td->dwExStyle = (WS_EX_TOOLWINDOW);
    }

    /* Font of each window */
    for (int i = 0; i < MAX_TERM_DATA; i++) {
        td = &data[i];
        wcsncpy(td->lf.lfFaceName, to_wchar(td->font_want).wc_str(), LF_FACESIZE);
        td->lf.lfCharSet = _(SHIFTJIS_CHARSET, DEFAULT_CHARSET);
        td->lf.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
        term_force_font(td);
        if (!td->tile_wid) {
            td->tile_wid = td->font_wid;
        }
        if (!td->tile_hgt) {
            td->tile_hgt = td->font_hgt;
        }
        term_getsize(td);
        term_window_resize(td);
    }

    /* Create sub windows */
    for (int i = MAX_TERM_DATA - 1; i >= 1; --i) {
        td = &data[i];

        my_td = td;
        td->w = CreateWindowExW(
            td->dwExStyle, AngList, td->name, td->dwStyle, td->pos_x, td->pos_y, td->size_wid, td->size_hgt, HWND_DESKTOP, NULL, hInstance, NULL);
        my_td = NULL;

        if (td->w == NULL) {
            quit(_("サブウィンドウに作成に失敗しました", "Failed to create sub-window"));
            return; // 静的解析対応.
        }

        td->size_hack = true;
        term_getsize(td);
        term_window_resize(td);

        if (td->visible) {
            ShowWindow(td->w, SW_SHOW);
        }
        td->size_hack = false;

        term_data_link(td);
        angband_terms[i] = &td->t;

        if (td->visible) {
            /* Activate the window */
            SetActiveWindow(td->w);
        }

        if (td->posfix) {
            term_window_pos(td, HWND_TOPMOST);
        } else {
            term_window_pos(td, td->w);
        }
    }

    /* Create main window */
    td = &data[0];
    my_td = td;
    td->w = CreateWindowExW(
        td->dwExStyle, AppName, _(L"馬鹿馬鹿蛮怒", td->name), td->dwStyle,
        td->pos_x, td->pos_y, td->size_wid, td->size_hgt, HWND_DESKTOP, NULL, hInstance, NULL);
    my_td = NULL;

    if (td->w == NULL) {
        quit(_("メインウィンドウの作成に失敗しました", "Failed to create main window"));
        return; // 静的解析対応.
    }

    /* Resize */
    td->size_hack = true;
    term_getsize(td);
    term_window_resize(td);
    td->size_hack = false;

    term_data_link(td);
    angband_terms[0] = &td->t;
    normsize.x = td->cols;
    normsize.y = td->rows;

    if (win_maximized) {
        ShowWindow(td->w, SW_SHOWMAXIMIZED);
    } else {
        ShowWindow(td->w, SW_SHOW);
    }

    SetWindowPos(td->w, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    hbrYellow = CreateSolidBrush(win_clr[TERM_YELLOW]);
    (void)term_xtra_win_flush();
}

/*!
 * @brief Prepare the menus
 */
static void setup_menus()
{
    HMENU hm = GetMenu(data[0].w);

    if (AngbandWorld::get_instance().character_generated) {
        EnableMenuItem(hm, IDM_FILE_NEW, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        EnableMenuItem(hm, IDM_FILE_OPEN, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        EnableMenuItem(hm, IDM_FILE_SAVE, MF_BYCOMMAND | MF_ENABLED);
    } else {
        EnableMenuItem(hm, IDM_FILE_NEW, MF_BYCOMMAND | MF_ENABLED);
        EnableMenuItem(hm, IDM_FILE_OPEN, MF_BYCOMMAND | MF_ENABLED);
        EnableMenuItem(hm, IDM_FILE_SAVE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
    }

    for (int i = 0; i < MAX_TERM_DATA; i++) {
        EnableMenuItem(hm, IDM_WINDOW_VIS_0 + i, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        CheckMenuItem(hm, IDM_WINDOW_VIS_0 + i, (data[i].visible ? MF_CHECKED : MF_UNCHECKED));
        EnableMenuItem(hm, IDM_WINDOW_VIS_0 + i, MF_BYCOMMAND | MF_ENABLED);
    }

    for (int i = 0; i < MAX_TERM_DATA; i++) {
        EnableMenuItem(hm, IDM_WINDOW_FONT_0 + i, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

        if (data[i].visible) {
            EnableMenuItem(hm, IDM_WINDOW_FONT_0 + i, MF_BYCOMMAND | MF_ENABLED);
        }
    }

    for (int i = 0; i < MAX_TERM_DATA; i++) {
        EnableMenuItem(hm, IDM_WINDOW_POS_0 + i, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        CheckMenuItem(hm, IDM_WINDOW_POS_0 + i, (data[i].posfix ? MF_CHECKED : MF_UNCHECKED));
        if (data[i].visible) {
            EnableMenuItem(hm, IDM_WINDOW_POS_0 + i, MF_BYCOMMAND | MF_ENABLED);
        }
    }

    for (int i = 0; i < MAX_TERM_DATA; i++) {
        EnableMenuItem(hm, IDM_WINDOW_I_WID_0 + i, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        if (data[i].visible) {
            EnableMenuItem(hm, IDM_WINDOW_I_WID_0 + i, MF_BYCOMMAND | MF_ENABLED);
        }
    }

    for (int i = 0; i < MAX_TERM_DATA; i++) {
        EnableMenuItem(hm, IDM_WINDOW_D_WID_0 + i, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        if (data[i].visible) {
            EnableMenuItem(hm, IDM_WINDOW_D_WID_0 + i, MF_BYCOMMAND | MF_ENABLED);
        }
    }

    for (int i = 0; i < MAX_TERM_DATA; i++) {
        EnableMenuItem(hm, IDM_WINDOW_I_HGT_0 + i, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        if (data[i].visible) {
            EnableMenuItem(hm, IDM_WINDOW_I_HGT_0 + i, MF_BYCOMMAND | MF_ENABLED);
        }
    }

    for (int i = 0; i < MAX_TERM_DATA; i++) {
        EnableMenuItem(hm, IDM_WINDOW_D_HGT_0 + i, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

        if (data[i].visible) {
            EnableMenuItem(hm, IDM_WINDOW_D_HGT_0 + i, MF_BYCOMMAND | MF_ENABLED);
        }
    }
    CheckMenuItem(hm, IDM_WINDOW_KEEP_SUBWINDOWS, (keep_subwindows ? MF_CHECKED : MF_UNCHECKED));

    CheckMenuItem(hm, IDM_OPTIONS_NO_GRAPHICS, (arg_graphics == enum2i(graphics_mode::GRAPHICS_NONE) ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hm, IDM_OPTIONS_OLD_GRAPHICS, (arg_graphics == enum2i(graphics_mode::GRAPHICS_ORIGINAL) ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hm, IDM_OPTIONS_NEW_GRAPHICS, (arg_graphics == enum2i(graphics_mode::GRAPHICS_ADAM_BOLT) ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hm, IDM_OPTIONS_NEW2_GRAPHICS, (arg_graphics == enum2i(graphics_mode::GRAPHICS_HENGBAND) ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hm, IDM_OPTIONS_BIGTILE, (arg_bigtile ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hm, IDM_OPTIONS_MUSIC, (arg_music ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuRadioItem(hm, IDM_OPTIONS_MUSIC_VOLUME_100, IDM_OPTIONS_MUSIC_VOLUME_010,
        IDM_OPTIONS_MUSIC_VOLUME_100 + arg_music_volume_table_index, MF_BYCOMMAND);
    CheckMenuItem(hm, IDM_OPTIONS_MUSIC_PAUSE_INACTIVE, (use_pause_music_inactive ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hm, IDM_OPTIONS_SOUND, (arg_sound ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuRadioItem(hm, IDM_OPTIONS_SOUND_VOLUME_100, IDM_OPTIONS_SOUND_VOLUME_010,
        IDM_OPTIONS_SOUND_VOLUME_100 + arg_sound_volume_table_index, MF_BYCOMMAND);
    CheckMenuItem(hm, IDM_OPTIONS_NO_BG, ((current_bg_mode == bg_mode::BG_NONE) ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hm, IDM_OPTIONS_BG, ((current_bg_mode == bg_mode::BG_ONE) ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hm, IDM_OPTIONS_PRESET_BG, ((current_bg_mode == bg_mode::BG_PRESET) ? MF_CHECKED : MF_UNCHECKED));
    // TODO IDM_OPTIONS_PRESET_BG を有効にする
    EnableMenuItem(hm, IDM_OPTIONS_PRESET_BG, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
}

/*!
 * @brief Check for double clicked (or dragged) savefile
 * @details
 * Apparently, Windows copies the entire filename into the first
 * piece of the "command line string".  Perhaps we should extract
 * the "basename" of that filename and append it to the "save" dir.
 * @param savefile_option savefile path
 */
static void check_for_save_file(const std::string &savefile_option)
{
    if (savefile_option.empty()) {
        return;
    }

    savefile = savefile_option;
    validate_file(savefile);
    game_in_progress = true;
}

/*!
 * @brief Process a menu command
 */
static void process_menus(PlayerType *player_ptr, WORD wCmd)
{
    if (!initialized) {
        plog(_("まだ初期化中です...", "You cannot do that yet..."));
        return;
    }

    term_data *td;
    OPENFILENAMEW ofn{};
    switch (wCmd) {
    case IDM_FILE_NEW: {
        if (game_in_progress || movie_in_progress) {
            plog(_("プレイ中は新しいゲームを始めることができません！", "You can't start a new game while you're still playing!"));
        } else {
            game_in_progress = true;
            savefile = "";
        }

        break;
    }
    case IDM_FILE_OPEN: {
        if (game_in_progress || movie_in_progress) {
            plog(_("プレイ中はゲームをロードすることができません！", "You can't open a new game while you're still playing!"));
        } else {
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = data[0].w;
            ofn.lpstrFilter = L"Save Files (*.)\0*\0";
            ofn.nFilterIndex = 1;
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_HIDEREADONLY;
            const auto &filename = get_open_filename(&ofn, ANGBAND_DIR_SAVE, savefile, MAIN_WIN_MAX_PATH);
            if (filename) {
                savefile = *filename;
                validate_file(savefile);
                game_in_progress = true;
            }
        }

        break;
    }
    case IDM_FILE_SAVE: {
        if (game_in_progress && AngbandWorld::get_instance().character_generated) {
            if (!can_save) {
                plog(_("今はセーブすることは出来ません。", "You may not do that right now."));
                break;
            }

            msg_flag = false;
            do_cmd_save_game(player_ptr, false);
        } else {
            plog(_("今、セーブすることは出来ません。", "You may not do that right now."));
        }

        break;
    }
    case IDM_FILE_EXIT: {
        if (game_in_progress && AngbandWorld::get_instance().character_generated) {
            if (!can_save) {
                plog(_("今は終了できません。", "You may not do that right now."));
                break;
            }

            msg_flag = false;
            forget_lite(*player_ptr->current_floor_ptr);
            forget_view(*player_ptr->current_floor_ptr);
            clear_mon_lite(*player_ptr->current_floor_ptr);

            term_key_push(SPECIAL_KEY_QUIT);
            break;
        }

        quit("");
        break;
    }
    case IDM_FILE_SCORE: {
        const auto path = path_build(ANGBAND_DIR_APEX, "scores.raw");
        highscore_fd = fd_open(path, O_RDONLY);
        if (highscore_fd < 0) {
            msg_print("Score file unavailable.");
        } else {
            screen_save();
            term_clear();
            display_scores(0, MAX_HISCORES, -1, nullptr);
            (void)fd_close(highscore_fd);
            highscore_fd = -1;
            screen_load();
            term_fresh();
        }

        break;
    }
    case IDM_FILE_MOVIE: {
        if (game_in_progress || movie_in_progress) {
            plog(_("プレイ中はムービーをロードすることができません！", "You can't open a movie while you're playing!"));
        } else {
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = data[0].w;
            ofn.lpstrFilter = L"Angband Movie Files (*.amv)\0*.amv\0";
            ofn.nFilterIndex = 1;
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
            const auto &filename = get_open_filename(&ofn, ANGBAND_DIR_USER, savefile, MAIN_WIN_MAX_PATH);
            if (filename) {
                savefile = *filename;
                prepare_browse_movie_without_path_build(savefile);
                movie_in_progress = true;
            }
        }

        break;
    }
    case IDM_WINDOW_VIS_0: {
        plog(_("メインウィンドウは非表示にできません！", "You are not allowed to do that!"));
        break;
    }
    case IDM_WINDOW_VIS_1:
    case IDM_WINDOW_VIS_2:
    case IDM_WINDOW_VIS_3:
    case IDM_WINDOW_VIS_4:
    case IDM_WINDOW_VIS_5:
    case IDM_WINDOW_VIS_6:
    case IDM_WINDOW_VIS_7: {
        int i = wCmd - IDM_WINDOW_VIS_0;
        if ((i < 0) || (i >= MAX_TERM_DATA)) {
            break;
        }

        td = &data[i];
        if (!td->visible) {
            td->visible = true;
            ShowWindow(td->w, SW_SHOW);
            term_data_redraw(td);
        } else {
            td->visible = false;
            td->posfix = false;
            ShowWindow(td->w, SW_HIDE);
        }

        break;
    }
    case IDM_WINDOW_FONT_0:
    case IDM_WINDOW_FONT_1:
    case IDM_WINDOW_FONT_2:
    case IDM_WINDOW_FONT_3:
    case IDM_WINDOW_FONT_4:
    case IDM_WINDOW_FONT_5:
    case IDM_WINDOW_FONT_6:
    case IDM_WINDOW_FONT_7: {
        int i = wCmd - IDM_WINDOW_FONT_0;
        if ((i < 0) || (i >= MAX_TERM_DATA)) {
            break;
        }

        td = &data[i];
        term_change_font(td);
        break;
    }
    case IDM_WINDOW_POS_1:
    case IDM_WINDOW_POS_2:
    case IDM_WINDOW_POS_3:
    case IDM_WINDOW_POS_4:
    case IDM_WINDOW_POS_5:
    case IDM_WINDOW_POS_6:
    case IDM_WINDOW_POS_7: {
        int i = wCmd - IDM_WINDOW_POS_0;
        if ((i < 0) || (i >= MAX_TERM_DATA)) {
            break;
        }

        td = &data[i];
        if (!td->posfix && td->visible) {
            td->posfix = true;
            term_window_pos(td, HWND_TOPMOST);
        } else {
            td->posfix = false;
            term_window_pos(td, data[0].w);
        }

        break;
    }
    case IDM_WINDOW_I_WID_0:
    case IDM_WINDOW_I_WID_1:
    case IDM_WINDOW_I_WID_2:
    case IDM_WINDOW_I_WID_3:
    case IDM_WINDOW_I_WID_4:
    case IDM_WINDOW_I_WID_5:
    case IDM_WINDOW_I_WID_6:
    case IDM_WINDOW_I_WID_7: {
        int i = wCmd - IDM_WINDOW_I_WID_0;
        if ((i < 0) || (i >= MAX_TERM_DATA)) {
            break;
        }

        td = &data[i];
        td->tile_wid += 1;
        term_getsize(td);
        term_window_resize(td);
        break;
    }
    case IDM_WINDOW_D_WID_0:
    case IDM_WINDOW_D_WID_1:
    case IDM_WINDOW_D_WID_2:
    case IDM_WINDOW_D_WID_3:
    case IDM_WINDOW_D_WID_4:
    case IDM_WINDOW_D_WID_5:
    case IDM_WINDOW_D_WID_6:
    case IDM_WINDOW_D_WID_7: {
        int i = wCmd - IDM_WINDOW_D_WID_0;
        if ((i < 0) || (i >= MAX_TERM_DATA)) {
            break;
        }

        td = &data[i];
        td->tile_wid -= 1;
        term_getsize(td);
        term_window_resize(td);
        break;
    }
    case IDM_WINDOW_I_HGT_0:
    case IDM_WINDOW_I_HGT_1:
    case IDM_WINDOW_I_HGT_2:
    case IDM_WINDOW_I_HGT_3:
    case IDM_WINDOW_I_HGT_4:
    case IDM_WINDOW_I_HGT_5:
    case IDM_WINDOW_I_HGT_6:
    case IDM_WINDOW_I_HGT_7: {
        int i = wCmd - IDM_WINDOW_I_HGT_0;
        if ((i < 0) || (i >= MAX_TERM_DATA)) {
            break;
        }

        td = &data[i];
        td->tile_hgt += 1;
        term_getsize(td);
        term_window_resize(td);
        break;
    }
    case IDM_WINDOW_D_HGT_0:
    case IDM_WINDOW_D_HGT_1:
    case IDM_WINDOW_D_HGT_2:
    case IDM_WINDOW_D_HGT_3:
    case IDM_WINDOW_D_HGT_4:
    case IDM_WINDOW_D_HGT_5:
    case IDM_WINDOW_D_HGT_6:
    case IDM_WINDOW_D_HGT_7: {
        int i = wCmd - IDM_WINDOW_D_HGT_0;
        if ((i < 0) || (i >= MAX_TERM_DATA)) {
            break;
        }

        td = &data[i];
        td->tile_hgt -= 1;
        term_getsize(td);
        term_window_resize(td);
        break;
    }
    case IDM_WINDOW_KEEP_SUBWINDOWS: {
        keep_subwindows = !keep_subwindows;
        break;
    }
    case IDM_OPTIONS_NO_GRAPHICS: {
        if (arg_graphics != enum2i(graphics_mode::GRAPHICS_NONE)) {
            arg_graphics = enum2i(graphics_mode::GRAPHICS_NONE);
            if (game_in_progress) {
                do_cmd_redraw(player_ptr);
            }
        }
        break;
    }
    case IDM_OPTIONS_OLD_GRAPHICS: {
        if (arg_graphics != enum2i(graphics_mode::GRAPHICS_ORIGINAL)) {
            arg_graphics = enum2i(graphics_mode::GRAPHICS_ORIGINAL);
            if (game_in_progress) {
                do_cmd_redraw(player_ptr);
            }
        }

        break;
    }
    case IDM_OPTIONS_NEW_GRAPHICS: {
        if (arg_graphics != enum2i(graphics_mode::GRAPHICS_ADAM_BOLT)) {
            arg_graphics = enum2i(graphics_mode::GRAPHICS_ADAM_BOLT);
            if (game_in_progress) {
                do_cmd_redraw(player_ptr);
            }
        }

        break;
    }
    case IDM_OPTIONS_NEW2_GRAPHICS: {
        if (arg_graphics != enum2i(graphics_mode::GRAPHICS_HENGBAND)) {
            arg_graphics = enum2i(graphics_mode::GRAPHICS_HENGBAND);
            if (game_in_progress) {
                do_cmd_redraw(player_ptr);
            }
        }

        break;
    }
    case IDM_OPTIONS_BIGTILE: {
        td = &data[0];
        arg_bigtile = !arg_bigtile;
        rebuild_term(td);
        break;
    }
    case IDM_OPTIONS_MUSIC: {
        arg_music = !arg_music;
        use_music = arg_music;
        if (!use_music) {
            main_win_music::stop_music();
            break;
        }

        init_music();
        if (game_in_progress) {
            select_floor_music(player_ptr);
        }

        break;
    }
    case IDM_OPTIONS_MUSIC_VOLUME_100:
    case IDM_OPTIONS_MUSIC_VOLUME_090:
    case IDM_OPTIONS_MUSIC_VOLUME_080:
    case IDM_OPTIONS_MUSIC_VOLUME_070:
    case IDM_OPTIONS_MUSIC_VOLUME_060:
    case IDM_OPTIONS_MUSIC_VOLUME_050:
    case IDM_OPTIONS_MUSIC_VOLUME_040:
    case IDM_OPTIONS_MUSIC_VOLUME_030:
    case IDM_OPTIONS_MUSIC_VOLUME_020:
    case IDM_OPTIONS_MUSIC_VOLUME_010: {
        arg_music_volume_table_index = wCmd - IDM_OPTIONS_MUSIC_VOLUME_100;
        if (use_music) {
            main_win_music::set_music_volume(main_win_music::VOLUME_TABLE[arg_music_volume_table_index]);
        }
        break;
    }
    case IDM_OPTIONS_MUSIC_PAUSE_INACTIVE: {
        use_pause_music_inactive = !use_pause_music_inactive;
        break;
    }
    case IDM_OPTIONS_OPEN_MUSIC_DIR: {
        const auto path = path_build(ANGBAND_DIR_XTRA_MUSIC, "music.cfg");
        open_dir_in_explorer(path);
        break;
    }
    case IDM_OPTIONS_SOUND: {
        arg_sound = !arg_sound;
        change_sound_mode(arg_sound);
        break;
    }
    case IDM_OPTIONS_SOUND_VOLUME_100:
    case IDM_OPTIONS_SOUND_VOLUME_090:
    case IDM_OPTIONS_SOUND_VOLUME_080:
    case IDM_OPTIONS_SOUND_VOLUME_070:
    case IDM_OPTIONS_SOUND_VOLUME_060:
    case IDM_OPTIONS_SOUND_VOLUME_050:
    case IDM_OPTIONS_SOUND_VOLUME_040:
    case IDM_OPTIONS_SOUND_VOLUME_030:
    case IDM_OPTIONS_SOUND_VOLUME_020:
    case IDM_OPTIONS_SOUND_VOLUME_010: {
        arg_sound_volume_table_index = wCmd - IDM_OPTIONS_SOUND_VOLUME_100;
        break;
    }
    case IDM_OPTIONS_OPEN_SOUND_DIR: {
        const auto path = path_build(ANGBAND_DIR_XTRA_SOUND, "sound.cfg");
        open_dir_in_explorer(path);
        break;
    }
    case IDM_OPTIONS_NO_BG: {
        change_bg_mode(bg_mode::BG_NONE);
        break;
    }
    case IDM_OPTIONS_PRESET_BG: {
        change_bg_mode(bg_mode::BG_PRESET);
        break;
    }
    case IDM_OPTIONS_BG: {
        if (change_bg_mode(bg_mode::BG_ONE)) {
            break;
        }
        // 壁紙の設定に失敗した（ファイルが存在しない等）場合、壁紙に使うファイルを選択させる
    }
        [[fallthrough]];
    case IDM_OPTIONS_OPEN_BG: {
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = data[0].w;
        ofn.lpstrFilter = L"Image Files (*.bmp;*.png;*.jpg;*.jpeg;)\0*.bmp;*.png;*.jpg;*.jpeg;\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrTitle = _(L"壁紙を選んでね。", L"Choose wall paper.");
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
        const auto &filename = get_open_filename(&ofn, "", wallpaper_path, MAIN_WIN_MAX_PATH);
        if (filename) {
            wallpaper_path = *filename;
            change_bg_mode(bg_mode::BG_ONE, true, true);
        }
        break;
    }
    case IDM_DUMP_SCREEN_HTML: {
        save_screen_as_html(data[0].w);
        break;
    }
    }
}

/*!
 * @brief Add a keypress to the "queue"
 */
static errr term_keypress(int k)
{
    /* Refuse to enqueue non-keys */
    if (!k) {
        return -1;
    }

    /* Store the char, advance the queue */
    game_term->key_queue[game_term->key_head++] = (char)k;

    /* Circular queue, handle wrap */
    if (game_term->key_head == game_term->key_size) {
        game_term->key_head = 0;
    }

    if (game_term->key_head != game_term->key_tail) {
        return 0;
    }

    return 1;
}

/*!
 * @brief Add a keypress to the "queue"
 * @details マルチバイト文字をkey_queueに追加する。
 * @param str マルチバイト文字列
 */
static void term_keypress(char *str)
{
    if (str) {
        char *psrc = str;
        while (*psrc) {
            term_keypress(*psrc);
            ++psrc;
        }
    }
}

/*!
 * @brief キーダウンのハンドラ
 */
static bool process_keydown(WPARAM wParam, LPARAM lParam)
{
    auto mc = any_bits(static_cast<ushort>(GetKeyState(VK_CONTROL)), 0x8000);
    auto ms = any_bits(static_cast<ushort>(GetKeyState(VK_SHIFT)), 0x8000);
    auto ma = any_bits(static_cast<ushort>(GetKeyState(VK_MENU)), 0x8000);
    term_no_press = ma;
    if (special_key[(byte)(wParam)] || (ma && !ignore_key[(byte)(wParam)])) {
        bool ext_key = any_bits(static_cast<ulong>(lParam), 0x1000000UL);
        bool numpad = false;

        term_keypress(31);
        if (mc) {
            term_keypress('C');
        }
        if (ms) {
            term_keypress('S');
        }
        if (ma) {
            term_keypress('A');
        }

        const auto i = LOBYTE(HIWORD(lParam));
        term_keypress('x');
        switch (wParam) {
        case VK_DIVIDE:
            term_no_press = true;
            [[fallthrough]];
        case VK_RETURN:
            numpad = ext_key;
            break;
        case VK_NUMPAD0:
        case VK_NUMPAD1:
        case VK_NUMPAD2:
        case VK_NUMPAD3:
        case VK_NUMPAD4:
        case VK_NUMPAD5:
        case VK_NUMPAD6:
        case VK_NUMPAD7:
        case VK_NUMPAD8:
        case VK_NUMPAD9:
        case VK_ADD:
        case VK_MULTIPLY:
        case VK_SUBTRACT:
        case VK_SEPARATOR:
        case VK_DECIMAL:
            term_no_press = true;
            [[fallthrough]];
        case VK_CLEAR:
        case VK_HOME:
        case VK_END:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_INSERT:
        case VK_DELETE:
        case VK_UP:
        case VK_DOWN:
        case VK_LEFT:
        case VK_RIGHT:
            numpad = !ext_key;
        }

        if (numpad) {
            term_keypress('K');
        }

        term_keypress(hexify_upper(i));
        term_keypress(hexify_lower(i));
        term_keypress(13);

        return 1;
    }

    return 0;
}

/*!
 * @brief ウィンドウのアクティブ/非アクティブのハンドラ
 */
static void handle_app_active(HWND hWnd, UINT uMsg, WPARAM wParam, [[maybe_unused]] LPARAM lParam)
{
    switch (uMsg) {
    case WM_ACTIVATEAPP: {
        if (wParam) {
            if (use_pause_music_inactive) {
                main_win_music::resume_music();
            }
        } else {
            if (use_pause_music_inactive) {
                main_win_music::pause_music();
            }
        }
        break;
    }
    case WM_WINDOWPOSCHANGING: {
        if (!IsIconic(hWnd)) {
            if (use_pause_music_inactive) {
                main_win_music::resume_music();
            }
        }
        break;
    }
    }
}

/*!
 * @brief ターミナルのサイズをウインドウのサイズに合わせる
 * @param td term_dataのポインタ
 * @param recalc_window_size trueの場合に行列数からウインドウサイズを再計算し設定する
 */
static void fit_term_size_to_window(term_data *td, bool recalc_window_size = false)
{
    RECT rc;
    ::GetClientRect(td->w, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    TERM_LEN cols = (width - td->size_ow1 - td->size_ow2) / td->tile_wid;
    TERM_LEN rows = (height - td->size_oh1 - td->size_oh2) / td->tile_hgt;
    if ((td->cols != cols) || (td->rows != rows)) {
        td->cols = cols;
        td->rows = rows;
        if (is_main_term(td) && !IsZoomed(td->w) && !IsIconic(td->w)) {
            normsize.x = td->cols;
            normsize.y = td->rows;
        }

        rebuild_term(td, recalc_window_size);

        if (!is_main_term(td)) {
            RedrawingFlagsUpdater::get_instance().fill_up_sub_flags();
            handle_stuff(p_ptr);
        }
    }
}

/*!
 * @brief Windowのリサイズをハンドリング
 * @retval true ウインドウメッセージを処理した
 * @retval false ウインドウメッセージを処理していない
 */
static bool handle_window_resize(term_data *td, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (!td) {
        return false;
    }
    if (!td->w) {
        return false;
    }

    switch (uMsg) {
    case WM_GETMINMAXINFO: {
        const bool is_main = is_main_term(td);
        const int min_cols = (is_main) ? MAIN_TERM_MIN_COLS : 20;
        const int min_rows = (is_main) ? MAIN_TERM_MIN_ROWS : 3;
        const LONG w = min_cols * td->tile_wid + td->size_ow1 + td->size_ow2;
        const LONG h = min_rows * td->tile_hgt + td->size_oh1 + td->size_oh2 + 1;
        RECT rc{ 0, 0, w, h };
        AdjustWindowRectEx(&rc, td->dwStyle, TRUE, td->dwExStyle);

        MINMAXINFO *lpmmi = (MINMAXINFO *)lParam;
        lpmmi->ptMinTrackSize.x = rc.right - rc.left;
        lpmmi->ptMinTrackSize.y = rc.bottom - rc.top;

        return true;
    }
    case WM_EXITSIZEMOVE: {
        fit_term_size_to_window(td, true);
        return true;
    }
    case WM_WINDOWPOSCHANGED: {
        if (!td->size_hack) {
            WINDOWPOS *pos = (WINDOWPOS *)lParam;
            if ((pos->flags & (SWP_NOCOPYBITS | SWP_NOSIZE)) == 0) {
                fit_term_size_to_window(td);
                return true;
            }
        }
        break;
    }
    case WM_SIZE: {
        if (td->size_hack) {
            break;
        }

        //!< @todo 二重のswitch文。後で分割する.
        switch (wParam) {
        case SIZE_MINIMIZED: {
            for (int i = 1; i < MAX_TERM_DATA; i++) {
                if (data[i].visible) {
                    ShowWindow(data[i].w, SW_HIDE);
                }
            }

            return true;
        }
        case SIZE_MAXIMIZED:
        case SIZE_RESTORED: {
            fit_term_size_to_window(td);

            td->size_hack = true;
            for (int i = 1; i < MAX_TERM_DATA; i++) {
                if (data[i].visible) {
                    ShowWindow(data[i].w, SW_SHOWNA);
                }
            }

            td->size_hack = false;

            return true;
        }
        }

        break;
    }
    }

    return false;
}

/*!
 * @brief メインウインドウ用ウインドウプロシージャ
 */
static LRESULT PASCAL angband_window_procedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    term_data *td = (term_data *)GetWindowLong(hWnd, 0);

    handle_app_active(hWnd, uMsg, wParam, lParam);
    if (handle_window_resize(td, uMsg, wParam, lParam)) {
        return 0;
    }

    switch (uMsg) {
    case WM_NCCREATE: {
        SetWindowLong(hWnd, 0, (LONG)(my_td));
        break;
    }
    case WM_CREATE: {
        setup_mci(hWnd);
        return 0;
    }
    case WM_ERASEBKGND: {
        return 1;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        if (td) {
            if (!td->render(ps.rcPaint)) {
                SetBkColor(hdc, RGB(0, 0, 0));
                ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &ps.rcPaint, NULL, 0, NULL);
            }
        }
        EndPaint(hWnd, &ps);
        ValidateRect(hWnd, NULL);
        return 0;
    }
    case MM_MCINOTIFY: {
        main_win_music::on_mci_notify(wParam, lParam, main_win_music::VOLUME_TABLE[arg_music_volume_table_index]);

        return 0;
    }
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN: {
        if (process_keydown(wParam, lParam)) {
            return 0;
        }
        break;
    }
    case WM_CHAR: {
        // wParam is WCHAR because using RegisterClassW
        if (term_no_press) {
            term_no_press = false;
        } else {
            WCHAR wc[2] = { (WCHAR)wParam, '\0' };
            term_keypress(to_multibyte(wc).c_str());
        }
        return 0;
    }
    case WM_LBUTTONDOWN: {
        if (macro_running()) {
            return 0;
        }
        mousex = std::min<int>(LOWORD(lParam) / td->tile_wid, td->cols - 1);
        mousey = std::min<int>(HIWORD(lParam) / td->tile_hgt, td->rows - 1);
        mouse_down = true;
        oldx = mousex;
        oldy = mousey;
        return 0;
    }
    case WM_LBUTTONUP: {
        if (!mouse_down) {
            return 0;
        }
        TERM_LEN dx = abs(oldx - mousex) + 1;
        TERM_LEN dy = abs(oldy - mousey) + 1;
        TERM_LEN ox = (oldx > mousex) ? mousex : oldx;
        TERM_LEN oy = (oldy > mousey) ? mousey : oldy;

        mouse_down = false;
        paint_rect = false;

#ifdef JP
        int sz = (dx + 3) * dy;
#else
        int sz = (dx + 2) * dy;
#endif
        const auto window_size = GlobalAlloc(GHND, sz + 1);
        if (window_size == NULL) {
            return 0;
        }

        auto global_lock = static_cast<LPSTR>(GlobalLock(window_size));
        for (auto i = 0; (i < dy) && (global_lock != NULL); i++) {
#ifdef JP
            const auto &scr = data[0].t.scr->c;

            std::vector<char> s(dx + 1);
            strncpy(s.data(), &scr[oy + i][ox], dx);

            if (ox > 0) {
                if (iskanji(scr[oy + i][ox - 1])) {
                    s[0] = ' ';
                }
            }

            if (ox + dx < data[0].cols) {
                if (iskanji(scr[oy + i][ox + dx - 1])) {
                    s[dx - 1] = ' ';
                }
            }

            for (int j = 0; j < dx; j++) {
                if (s[j] == 127) {
                    s[j] = '#';
                }
                *global_lock++ = s[j];
            }
#else
            for (int j = 0; j < dx; j++) {
                *global_lock++ = data[0].t.scr->c[oy + i][ox + j];
            }
#endif
            if (dy > 1) {
                *global_lock++ = '\r';
                *global_lock++ = '\n';
            }
        }

        GlobalUnlock(window_size);
        if (!OpenClipboard(hWnd)) {
            GlobalFree(window_size);
            return 0;
        }

        EmptyClipboard();
        if (SetClipboardData(CF_TEXT, window_size) == NULL) {
            CloseClipboard();
            GlobalFree(window_size);
            return 0;
        }

        CloseClipboard();
        term_redraw();
        return 0;
    }
    case WM_MOUSEMOVE: {
        if (!mouse_down) {
            return 0;
        }

        int dx, dy;
        auto cx = std::min<int>(LOWORD(lParam) / td->tile_wid, td->cols - 1);
        auto cy = std::min<int>(HIWORD(lParam) / td->tile_hgt, td->rows - 1);
        int ox, oy;

        if (paint_rect) {
            dx = abs(oldx - mousex) + 1;
            dy = abs(oldy - mousey) + 1;
            ox = (oldx > mousex) ? mousex : oldx;
            oy = (oldy > mousey) ? mousey : oldy;
            term_inversed_area(hWnd, ox, oy, dx, dy);
        } else {
            paint_rect = true;
        }

        dx = abs(cx - mousex) + 1;
        dy = abs(cy - mousey) + 1;
        ox = (cx > mousex) ? mousex : cx;
        oy = (cy > mousey) ? mousey : cy;
        term_inversed_area(hWnd, ox, oy, dx, dy);

        oldx = cx;
        oldy = cy;
        return 0;
    }
    case WM_INITMENU: {
        setup_menus();
        return 0;
    }
    case WM_CLOSE: {
        if (!game_in_progress || !AngbandWorld::get_instance().character_generated) {
            quit("");
            return 0;
        }

        if (!can_save) {
            plog(_("今は終了できません。", "You may not do that right now."));
            return 0;
        }

        msg_flag = false;
        forget_lite(*p_ptr->current_floor_ptr);
        forget_view(*p_ptr->current_floor_ptr);
        clear_mon_lite(*p_ptr->current_floor_ptr);
        term_key_push(SPECIAL_KEY_QUIT);
        return 0;
    }
    case WM_QUERYENDSESSION: {
        if (!game_in_progress || !AngbandWorld::get_instance().character_generated) {
            quit("");
            return 0;
        }

        msg_flag = false;
        if (p_ptr->chp < 0) {
            p_ptr->is_dead = false;
        }
        exe_write_diary(*p_ptr->current_floor_ptr, DiaryKind::GAMESTART, 0, _("----ゲーム中断----", "---- Save and Exit Game ----"));
        AngbandSystem::get_instance().set_panic_save(true);
        signals_ignore_tstp();
        p_ptr->died_from = _("(緊急セーブ)", "(panic save)");
        (void)save_player(p_ptr, SaveType::CLOSE_GAME);
        quit("");
        return 0;
    }
    case WM_QUIT: {
        quit("");
        return 0;
    }
    case WM_COMMAND: {
        process_menus(p_ptr, LOWORD(wParam));
        return 0;
    }
    case WM_ACTIVATE: {
        if (!wParam || HIWORD(lParam)) {
            break;
        }

        for (int i = 1; i < MAX_TERM_DATA; i++) {
            if (!data[i].posfix) {
                term_window_pos(&data[i], hWnd);
            }
        }

        SetFocus(hWnd);
        return 0;
    }
    case WM_ACTIVATEAPP: {
        if (IsIconic(td->w)) {
            break;
        }

        for (int i = 1; i < MAX_TERM_DATA; i++) {
            if (data[i].visible) {
                if (wParam == 1) {
                    ShowWindow(data[i].w, SW_SHOWNA);
                } else {
                    ShowWindow(data[i].w, SW_HIDE);
                }
            }
        }
    }
        [[fallthrough]];
    case WM_ENABLE: {
        if (wParam == FALSE && keep_subwindows) {
            for (int i = 1; i < MAX_TERM_DATA; i++) {
                if (data[i].visible) {
                    ShowWindow(data[i].w, SW_SHOWNA);
                }
            }
        }
    }
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*!
 * @brief サブウインドウ用ウインドウプロシージャ
 */
static LRESULT PASCAL AngbandListProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    term_data *td = (term_data *)GetWindowLong(hWnd, 0);
    if (handle_window_resize(td, uMsg, wParam, lParam)) {
        return 0;
    }

    switch (uMsg) {
    case WM_NCCREATE: {
        SetWindowLong(hWnd, 0, (LONG)(my_td));
        break;
    }
    case WM_CREATE: {
        return 0;
    }
    case WM_ERASEBKGND: {
        return 1;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        if (td) {
            if (!td->render(ps.rcPaint)) {
                SetBkColor(hdc, RGB(0, 0, 0));
                ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &ps.rcPaint, NULL, 0, NULL);
            }
        }
        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN: {
        if (process_keydown(wParam, lParam)) {
            return 0;
        }

        break;
    }
    case WM_CHAR: {
        // wParam is WCHAR because using RegisterClassW
        if (term_no_press) {
            term_no_press = false;
        } else {
            WCHAR wc[2] = { (WCHAR)wParam, '\0' };
            term_keypress(to_multibyte(wc).c_str());
        }
        return 0;
    }
    case WM_NCLBUTTONDOWN: {
        if (wParam == HTCLOSE) {
            wParam = HTSYSMENU;
        }

        if (wParam == HTSYSMENU) {
            if (td->visible) {
                td->visible = false;
                ShowWindow(td->w, SW_HIDE);
            }

            return 0;
        }

        break;
    }
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*!
 * @brief Display warning message (see "z-util.c")
 */
static void hook_plog(std::string_view str)
{
    if (!str.empty()) {
        MessageBoxW(data[0].w, to_wchar(str).wc_str(), _(L"警告！", L"Warning"), MB_ICONEXCLAMATION | MB_OK);
    }
}

/*!
 * @brief Display error message and quit (see "z-util.c")
 */
static void hook_quit(std::string_view str)
{
    if (!str.empty()) {
        MessageBoxW(data[0].w, to_wchar(str).wc_str(), _(L"エラー！", L"Error"), MB_ICONEXCLAMATION | MB_OK | MB_ICONSTOP);
    }

    save_prefs();
    for (int i = MAX_TERM_DATA - 1; i >= 0; --i) {
        term_force_font(&data[i]);
        data[i].font_want = "";
        if (data[i].w) {
            DestroyWindow(data[i].w);
        }

        data[i].w = 0;
    }

    DeleteObject(hbrYellow);
    finalize_bg();
    graphic.finalize();
    finalize_sound();

    UnregisterClassW(AppName, hInstance);
    if (hIcon) {
        DestroyIcon(hIcon);
    }

    std::exit(0);
}

/*!
 * @brief Init some stuff
 */
static void init_stuff()
{
    char path[MAIN_WIN_MAX_PATH];
    DWORD path_len = GetModuleFileNameA(hInstance, path, MAIN_WIN_MAX_PATH);
    strcpy(path + path_len - 4, ".INI");
    path_ini_file = path;

    int i = path_len;
    for (; i > 0; i--) {
        if (path[i] == '\\') {
            break;
        }
    }

    strcpy(path + i + 1, "lib\\");
    validate_dir(path, true);
    init_file_paths(path);
    validate_dir(ANGBAND_DIR_APEX, false);
    validate_dir(ANGBAND_DIR_BONE, false);
    if (!is_directory(ANGBAND_DIR_EDIT)) {
        validate_dir(ANGBAND_DIR_DATA, true);
    } else {
        validate_dir(ANGBAND_DIR_DATA, false);
    }

    validate_dir(ANGBAND_DIR_FILE, true);
    validate_dir(ANGBAND_DIR_HELP, false);
    validate_dir(ANGBAND_DIR_INFO, false);
    validate_dir(ANGBAND_DIR_PREF, true);
    validate_dir(ANGBAND_DIR_SAVE, false);
    validate_dir(ANGBAND_DIR_USER, true);
    validate_dir(ANGBAND_DIR_XTRA, true);
    const auto path_news = path_build(ANGBAND_DIR_FILE, _("news_j.txt", "news.txt"));
    validate_file(path_news);

    ANGBAND_DIR_XTRA_GRAF = path_build(ANGBAND_DIR_XTRA, "graf");
    validate_dir(ANGBAND_DIR_XTRA_GRAF, true);

    ANGBAND_DIR_XTRA_SOUND = path_build(ANGBAND_DIR_XTRA, "sound");
    validate_dir(ANGBAND_DIR_XTRA_SOUND, false);

    ANGBAND_DIR_XTRA_MUSIC = path_build(ANGBAND_DIR_XTRA, "music");
    validate_dir(ANGBAND_DIR_XTRA_MUSIC, false);

    for (i = 0; special_key_list[i]; ++i) {
        special_key[special_key_list[i]] = true;
    }

    for (i = 0; ignore_key_list[i]; ++i) {
        ignore_key[ignore_key_list[i]] = true;
    }

    ANGBAND_SYS = "win";
    if (7 != GetKeyboardType(0)) {
        ANGBAND_KEYBOARD = "0";
    } else {
        switch (GetKeyboardType(1)) {
        case 0x0D01:
        case 0x0D02:
        case 0x0D03:
        case 0x0D04:
        case 0x0D05:
        case 0x0D06:
            /* NEC PC-98x1 */
            ANGBAND_KEYBOARD = "NEC98";
            break;
        default:
            /* PC/AT */
            ANGBAND_KEYBOARD = "JAPAN";
        }
    }
}

/*!
 * @brief 全スポイラー出力を行う
 * Create Spoiler files
 * @details スポイラー出力処理の成功、失敗に関わらずプロセスを終了する。
 */
void create_debug_spoiler()
{
    init_stuff();
    init_angband(p_ptr, true);

    switch (output_all_spoilers()) {
    case SpoilerOutputResultType::SUCCESSFUL:
        fprintf(stdout, "Successfully created a spoiler file.");
        break;
    case SpoilerOutputResultType::FILE_OPEN_FAILED:
        fprintf(stderr, "Cannot create spoiler file.");
        break;
    case SpoilerOutputResultType::FILE_CLOSE_FAILED:
        fprintf(stderr, "Cannot close spoiler file.");
        break;
    default:
        break;
    }

    quit("");
}

/*!
 * @todo よく見るとhMutexはちゃんと使われていない……？
 * @brief (Windows固有)メインウインドウ、サブウインドウのウインドウクラス登録
 */
static void register_wndclass()
{
    WNDCLASSW wc{};
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = angband_window_procedure;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 4;
    wc.hInstance = hInstance;
    wc.hIcon = hIcon = LoadIconW(hInstance, AppName);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName = AppName;
    wc.lpszClassName = AppName;

    if (!RegisterClassW(&wc)) {
        std::exit(1);
    }

    wc.lpfnWndProc = AngbandListProc;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = AngList;

    if (!RegisterClassW(&wc)) {
        std::exit(2);
    }
}

/*!
 * @brief (Windows固有)Windowsアプリケーションとしてのエントリポイント
 */
int WINAPI WinMain(
    _In_ HINSTANCE hInst, [[maybe_unused]] _In_opt_ HINSTANCE hPrevInst, [[maybe_unused]] _In_ LPSTR lpCmdLine, [[maybe_unused]] _In_ int nCmdShow)
{
    setlocale(LC_ALL, "ja_JP");
    hInstance = hInst;
    if (is_already_running()) {
        constexpr auto mes = _(L"馬鹿馬鹿蛮怒はすでに起動しています。", L"Bakabakaband is already running.");
        constexpr auto caption = _(L"エラー！", L"Error");
        MessageBoxW(NULL, mes, caption, MB_ICONEXCLAMATION | MB_OK | MB_ICONSTOP);
        return 0;
    }

    command_line.handle();
    register_wndclass();

    // before term_data initialize
    plog_aux = [](std::string_view str) {
        if (!str.empty()) {
            MessageBoxW(NULL, to_wchar(str).wc_str(), _(L"警告！", L"Warning"), MB_ICONEXCLAMATION | MB_OK);
        }
    };
    quit_aux = [](std::string_view str) {
        if (!str.empty()) {
            MessageBoxW(NULL, to_wchar(str).wc_str(), _(L"エラー！", L"Error"), MB_ICONEXCLAMATION | MB_OK | MB_ICONSTOP);
        }

        UnregisterClassW(AppName, hInstance);
        if (hIcon) {
            DestroyIcon(hIcon);
        }

        exit(0);
    };
    core_aux = quit_aux;

    init_stuff();
    refresh_color_table();
    init_windows();
    change_graphics_mode(static_cast<graphics_mode>(arg_graphics));
    change_bg_mode(current_bg_mode, true);

    // after term_data initialize
    plog_aux = hook_plog;
    quit_aux = hook_quit;
    core_aux = hook_quit;

    signals_init();
    term_activate(term_screen);
    {
        TermCenteredOffsetSetter tcos(MAIN_TERM_MIN_COLS, MAIN_TERM_MIN_ROWS);

        init_angband(p_ptr, false);
        initialized = true;

        check_for_save_file(command_line.get_savefile_option());
        prt(_("[ファイル] メニューの [新規] または [開く] を選択してください。", "[Choose 'New' or 'Open' from the 'File' menu]"), 23, _(8, 17));
        term_fresh();
    }

    change_sound_mode(arg_sound);
    use_music = arg_music;
    if (use_music) {
        init_music();
    }

    // ユーザーがゲーム開始を選択するまで待つループ
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (game_in_progress || movie_in_progress) {
            break;
        }
    }

    term_flush();
    if (movie_in_progress) {
        // selected movie
        play_game(p_ptr, false, true);
    } else if (savefile.empty()) {
        // new game
        play_game(p_ptr, true, false);
    } else {
        // selected savefile
        play_game(p_ptr, false, false);
    }

    quit("");
#ifdef WIN_DEBUG
    return 0;
#endif
}

#endif /* WINDOWS */
