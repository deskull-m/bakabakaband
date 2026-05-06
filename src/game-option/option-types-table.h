#pragma once

#include <cstdint>
#include <string>
#include <tl/optional.hpp>
#include <vector>

enum class GameOptionPage : int;
enum class GameOptionType : int {
    ROGUE_LIKE_COMMANDS = 0 * 32 + 0,
    QUICK_MESSAGES = 0 * 32 + 1,
    OTHER_QUERY_FLAG = 0 * 32 + 2,
    CARRY_QUERY_FLAG = 0 * 32 + 3,
    USE_OLD_TARGET = 0 * 32 + 4,
    ALWAYS_PICKUP = 0 * 32 + 5,
    ALWAYS_REPEAT = 0 * 32 + 6,
    DEPTH_IN_FEET = 0 * 32 + 7,
    STACK_FORCE_NOTES = 0 * 32 + 8,
    STACK_FORCE_COSTS = 0 * 32 + 9,
    SHOW_LABELS = 0 * 32 + 10,
    SHOW_WEIGHTS = 0 * 32 + 11,
    RING_BELL = 0 * 32 + 14,
    FIND_IGNORE_STAIRS = 0 * 32 + 16,
    FIND_IGNORE_DOORS = 0 * 32 + 17,
    FIND_CUT = 0 * 32 + 18,
    DISTURB_MOVE = 0 * 32 + 20,
    DISTURB_NEAR = 0 * 32 + 21,
    DISTURB_PANEL = 0 * 32 + 22,
    DISTURB_STATE = 0 * 32 + 23,
    DISTURB_MINOR = 0 * 32 + 24,
    ALERT_TRAP_DETECT = 0 * 32 + 25,
    DISTURB_UNKNOWN = 0 * 32 + 26,
    DISTURB_TRAP_DETECT = 0 * 32 + 27,
    LAST_WORDS = 0 * 32 + 28,
    OVER_EXERT = 0 * 32 + 29,
    SMALL_LEVELS = 0 * 32 + 30,
    EMPTY_LEVELS = 0 * 32 + 31,

    VIEW_UNSAFE_WALLS = 1 * 32 + 1,
    VIEW_HIDDEN_WALLS = 1 * 32 + 2,
    DISTURB_HIGH = 1 * 32 + 3,
    EXPAND_LIST = 1 * 32 + 5,
    VIEW_PERMA_GRIDS = 1 * 32 + 6,
    VIEW_TORCH_GRIDS = 1 * 32 + 7,
    VIEW_UNSAFE_GRIDS = 1 * 32 + 8,
    CONFIRM_QUEST = 1 * 32 + 9,
    FRESH_ONCE = 1 * 32 + 10,
    EQUIPPY_CHARS = 1 * 32 + 12,
    SMART_LEARN = 1 * 32 + 14,
    SMART_CHEAT = 1 * 32 + 15,
    VIEW_REDUCE_VIEW = 1 * 32 + 17,
    CHECK_ABORT = 1 * 32 + 18,
    FLUSH_FAILURE = 1 * 32 + 20,
    FLUSH_DISTURB = 1 * 32 + 21,
    FRESH_BEFORE = 1 * 32 + 23,
    FRESH_AFTER = 1 * 32 + 24,
    FRESH_MESSAGE = 1 * 32 + 25,
    COMPRESS_SAVEFILE = 1 * 32 + 26,
    HILITE_PLAYER = 1 * 32 + 27,
    VIEW_YELLOW_LITE = 1 * 32 + 28,
    VIEW_BRIGHT_LITE = 1 * 32 + 29,
    VIEW_GRANITE_LITE = 1 * 32 + 30,
    VIEW_SPECIAL_LITE = 1 * 32 + 31,

    SHOW_ITEM_GRAPH = 2 * 32 + 0,
    BOUND_WALLS_PERM = 2 * 32 + 1,
    MONSTER_TOMBSTONES = 2 * 32 + 2, /*!< bakabakaband 独自: モンスターが死ぬ度に墓石を立てる(ジョーク) */
    ALWAYS_SMALL_LEVELS = 2 * 32 + 3,
    TARGET_PET = 2 * 32 + 5,
    AUTO_MORE = 2 * 32 + 6,
    COMMAND_MENU = 2 * 32 + 7,
    DISPLAY_PATH = 2 * 32 + 8,
    ABBREV_EXTRA = 2 * 32 + 10,
    ABBREV_ALL = 2 * 32 + 11,
    EXP_NEED = 2 * 32 + 12,
    IGNORE_UNVIEW = 2 * 32 + 13,
    SHOW_AMMO_DETAIL = 2 * 32 + 14,
    SHOW_AMMO_NO_CRIT = 2 * 32 + 15,
    SHOW_AMMO_CRIT_RATIO = 2 * 32 + 16,
    SHOW_ACTUAL_VALUE = 2 * 32 + 17,
    SKIP_MORE = 2 * 32 + 18,
    SHOW_LORE_SUMMARY = 2 * 32 + 19,
    NUMPAD_AS_CURSORKEY = 2 * 32 + 31,

    ALWAYS_SHOW_LIST = 4 * 32 + 0,
    POWERUP_HOME = 4 * 32 + 3,
    KEEP_SAVEFILE = 4 * 32 + 4,
    AUTO_DUMP = 4 * 32 + 5,
    SEND_SCORE = 4 * 32 + 6,
    RECORD_FIX_ART = 4 * 32 + 11,
    RECORD_RAND_ART = 4 * 32 + 12,
    RECORD_DESTROY_UNIQ = 4 * 32 + 13,
    RECORD_FIX_QUEST = 4 * 32 + 14,
    RECORD_RAND_QUEST = 4 * 32 + 15,
    RECORD_MAXDEPTH = 4 * 32 + 16,
    RECORD_STAIR = 4 * 32 + 17,
    RECORD_BUY = 4 * 32 + 18,
    RECORD_SELL = 4 * 32 + 19,
    RECORD_DANGER = 4 * 32 + 20,
    RECORD_ARENA = 4 * 32 + 21,
    RECORD_IDENT = 4 * 32 + 22,
    RECORD_NAMED_PET = 4 * 32 + 23,

    DISPLAY_MUTATIONS = 5 * 32 + 0,
    PLAIN_DESCRIPTIONS = 5 * 32 + 1,
    CONFIRM_DESTROY = 5 * 32 + 3,
    CONFIRM_WEAR = 5 * 32 + 4,
    DISTURB_PETS = 5 * 32 + 6,
    EASY_OPEN = 5 * 32 + 7,
    EASY_DISARM = 5 * 32 + 8,
    EASY_FLOOR = 5 * 32 + 9,
    USE_COMMAND = 5 * 32 + 10,
    CENTER_PLAYER = 5 * 32 + 11,
    CENTER_RUNNING = 5 * 32 + 12,

    VANILLA_TOWN = 6 * 32 + 0,
    LITE_TOWN = 6 * 32 + 1,
    IRONMAN_SHOPS = 6 * 32 + 2,
    IRONMAN_SMALL_LEVELS = 6 * 32 + 3,
    IRONMAN_DOWNWARD = 6 * 32 + 4,
    PLAIN_PICKUP = 6 * 32 + 6,
    IRONMAN_EMPTY_LEVELS = 6 * 32 + 8,
    ALLOW_DEBUG_OPTS = 6 * 32 + 11,
    IRONMAN_ROOMS = 6 * 32 + 12,
    LEFT_HANDER = 6 * 32 + 13,
    PRESERVE_MODE = 6 * 32 + 14,
    AUTOROLLER = 6 * 32 + 15,
    AUTOCHARA = 6 * 32 + 16,
    IRONMAN_NIGHTMARE = 6 * 32 + 18,
    IRONMAN_ALLIANCE_HOSTILITY = 6 * 32 + 19, /*!< bakabakaband 独自: 全アライアンスから敵対される */

    DESTROY_ITEMS = 7 * 32 + 0,
    LEAVE_SPECIAL = 7 * 32 + 1,
    LEAVE_WORTH = 7 * 32 + 2,
    LEAVE_EQUIP = 7 * 32 + 3,
    LEAVE_WANTED = 7 * 32 + 4,
    LEAVE_CORPSE = 7 * 32 + 5,
    LEAVE_JUNK = 7 * 32 + 6,
    LEAVE_CHEST = 7 * 32 + 7,
    DESTROY_FEELING = 7 * 32 + 8,
    DESTROY_IDENTIFY = 7 * 32 + 9,

    MAX = 8 * 32,
};
class GameOption {
public:
    GameOption(bool *value, bool norm, GameOptionType type, std::string &&text, std::string &&description, const tl::optional<GameOptionPage> &page = tl::nullopt);
    GameOption(bool *value, bool norm, uint8_t set, uint8_t bits, std::string &&text, std::string &&description, const tl::optional<GameOptionPage> &page = tl::nullopt);
    bool *value;
    bool default_value;
    uint8_t flag_position;
    uint8_t offset;
    std::string text;
    std::string description;
    tl::optional<GameOptionPage> page;
};

extern const std::vector<GameOption> option_info;
extern const std::vector<GameOption> cheat_info;
extern const std::vector<GameOption> autosave_info;
