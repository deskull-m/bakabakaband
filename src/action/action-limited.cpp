/*!
 * @file action-limited.cpp
 * @brief プレイヤーの行動制約判定定義
 */

#include "action/action-limited.h"
#include "dungeon/dungeon-flag-types.h"
#include "game-option/disturbance-options.h"
#include "grid/grid.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "player-base/player-class.h"
#include "player/player-status.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor/floor-info.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"

/*!
 * @brief 魔法系コマンドが制限されているかを返す。
 * @return 魔法系コマンドを使用可能ならFALSE、不可能ならば理由をメッセージ表示してTRUEを返す。
 */
bool cmd_limit_cast(PlayerType *player_ptr)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    if (floor.is_underground() && (floor.get_dungeon_definition().flags.has(DungeonFeatureType::NO_MAGIC))) {
        msg_print(_("ダンジョンが魔法を吸収した！", "The dungeon absorbs all attempted magic!"));
        msg_erase();
        return true;
    }

    if (player_ptr->anti_magic) {
        msg_print(_("反魔法バリアが魔法を邪魔した！", "An anti-magic shell disrupts your magic!"));
        return true;
    }

    if (is_shero(player_ptr) && !PlayerClass(player_ptr).equals(PlayerClassType::BERSERKER)) {
        msg_format(_("狂戦士化していて頭が回らない！", "You cannot think directly!"));
        return true;
    }

    return false;
}

bool cmd_limit_confused(PlayerType *player_ptr)
{
    if (player_ptr->effects()->confusion().is_confused()) {
        msg_print(_("混乱していてできない！", "You are too confused!"));
        return true;
    }

    return false;
}

bool cmd_limit_image(PlayerType *player_ptr)
{
    if (player_ptr->effects()->hallucination().is_hallucinated()) {
        msg_print(_("幻覚が見えて集中できない！", "Your hallucinations prevent you from concentrating!"));
        return true;
    }

    return false;
}

bool cmd_limit_stun(PlayerType *player_ptr)
{
    if (player_ptr->effects()->stun().is_stunned()) {
        msg_print(_("頭が朦朧としていて集中できない！", "You are too stunned!"));
        return true;
    }

    return false;
}

bool cmd_limit_arena(PlayerType *player_ptr)
{
    if (player_ptr->current_floor_ptr->inside_arena) {
        msg_print(_("アリーナが魔法を吸収した！", "The arena absorbs all attempted magic!"));
        msg_erase();
        return true;
    }

    return false;
}

bool cmd_limit_blind(PlayerType *player_ptr)
{
    if (player_ptr->effects()->blindness().is_blind()) {
        msg_print(_("目が見えない。", "You can't see anything."));
        return true;
    }

    if (no_lite(player_ptr)) {
        msg_print(_("明かりがないので見えない。", "You have no light."));
        return true;
    }

    return false;
}

bool cmd_limit_time_walk(PlayerType *player_ptr)
{
    if (player_ptr->timewalk) {
        if (flush_failure) {
            flush();
        }

        msg_print(_("止まった時の中ではうまく働かないようだ。", "It shows no reaction."));
        sound(SoundKind::FAIL);
        return true;
    }

    return false;
}
