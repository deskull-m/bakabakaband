#pragma once

#include "floor/geometry.h"
#include "game-option/keymap-directory-getter.h"
#include "system/angband.h"
#include <string>
#include <tl/optional.hpp>

extern bool use_menu;

extern COMMAND_CODE command_cmd;
extern COMMAND_ARG command_arg;
extern short command_rep;
extern Direction command_dir;
extern int16_t command_see;
extern TERM_LEN command_gap;
extern int16_t command_wrk;
extern int16_t command_new;

enum class KeymapMode;
class PlayerType;
class SpecialMenuContent;
class InputKeyRequestor {
public:
    InputKeyRequestor(PlayerType *player_ptr, bool shopping);
    void request_command();

private:
    PlayerType *player_ptr;
    bool shopping;
    KeymapMode mode;
    int base_y;
    int base_x = 15;
    int menu_num = 0;
    int num = 0;
    char command = 0;
    int max_num = 0;
    bool is_max_num_odd = false;
    char sub_cmd = 0;

    void process_input_command();
    short get_command();
    char inkey_from_menu();
    bool process_repeat_num(short &cmd);
    char input_repeat_num();
    void process_command_command(short &cmd);
    void process_control_command(short &cmd);
    void change_shopping_command() const;
    int get_caret_command() const;
    void sweep_confirmation_equipments();
    void confirm_command(const tl::optional<std::string> &inscription, const int caret_command);

    void make_commands_frame() const;
    std::string switch_special_menu_condition(const SpecialMenuContent &special_menu) const;
    int get_command_per_menu_num();
    bool check_continuous_command();
    bool check_escape_key(const int old_num);
    bool process_down_cursor();
    bool process_up_cursor();
    void process_right_left_cursor();
};
