#include "core/score-util.h"
#include "locale/character-encoding.h"
#include "player/player-status.h"
#include "system/angband-version.h"
#include "system/creature-entity.h"
#include "system/dungeon/dungeon-record.h"
#include "system/floor/floor-info.h"
#include "system/inner-game-data.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include "world/world.h"
#include <algorithm>
#include <cstdlib>
#include <fmt/format.h>
#ifdef SET_UID
#include "main-unix/unix-user-ids.h"
#endif

/*!
 * @brief クリーチャーの今の状態からスコアの記録を作る
 * @param creature クリーチャーへの参照
 * @return スコアの記録。日付と死因は呼び出し元で設定する
 */
HighScore HighScore::create(CreatureEntity &creature)
{
    HighScore score;
    score.version = fmt::format("{}.{}.{}", H_VER_MAJOR, H_VER_MINOR, H_VER_PATCH);
    score.points = calc_score(creature);
    score.gold = creature.get_au();
    score.turns = InnerGameData::get_instance().get_real_turns(AngbandWorld::get_instance().game_turn);
    // 前の版のスコアファイルと同じく 15 文字までにする (-u で起動すると、入力の上限より長い名前になることがある)
    score.name = from_sys(str_substr(creature.name, 0, 15));
#ifdef SET_UID
    score.uid = UnixUserIds::get_instance().get_user_id();
#endif
    score.sex = creature.psex ? "m" : "f";
    score.race = std::min(enum2i(creature.prace), MAX_RACES);
    score.pclass = enum2i(std::min(creature.pclass, PlayerClassType::MAX));
    score.personality = static_cast<int>(std::min(creature.ppersonality, MAX_PERSONALITIES));
    score.level = std::min<int>(creature.get_level(), 999);
    const auto &floor = *creature.get_floor();
    score.dungeon_level = floor.dun_level;
    score.max_level = std::min<int>(creature.get_max_plv(), 999);
    score.max_dungeon_level = DungeonRecords::get_instance().get_record(floor.dungeon_id).get_max_level();
    return score;
}

/*!
 * @brief 内部の文字コードの文字列を、記録の文字列 (UTF-8) にする
 * @param str 内部の文字コードの文字列
 * @return UTF-8 の文字列。変換できないときは "?"
 */
std::string HighScore::from_sys(std::string_view str)
{
    return sys_to_utf8(str).value_or("?");
}

/*!
 * @brief 記録の文字列 (UTF-8) を、表示するために内部の文字コードにする
 * @param str UTF-8 の文字列
 * @return 内部の文字コードの文字列。変換できないときは "?"
 */
std::string HighScore::to_sys(std::string_view str)
{
    return utf8_to_sys(str).value_or("?");
}

/*!
 * @brief scores.raw の 1 件をスコアの記録にする
 * @return スコアの記録。数値の欄は先頭の空白を除いて読む
 */
HighScore high_score::to_score() const
{
    const auto field = []<size_t N>(const char(&chars)[N]) {
        return std::string(chars, std::find(chars, chars + N, '\0'));
    };
    const auto number = [&field]<size_t N>(const char(&chars)[N]) {
        return std::atoll(field(chars).data());
    };
    const auto small_number = [&number]<size_t N>(const char(&chars)[N]) {
        return static_cast<int>(number(chars));
    };

    HighScore score;
    score.version = field(this->what);
    score.points = number(this->pts);
    score.gold = number(this->gold);
    score.turns = number(this->turns);
    score.date = field(this->day);
    score.name = field(this->who);
    score.uid = number(this->uid);
    score.sex = field(this->sex);
    score.race = small_number(this->p_r);
    score.pclass = small_number(this->p_c);
    score.personality = small_number(this->p_a);
    score.level = small_number(this->cur_lev);
    score.dungeon_level = small_number(this->cur_dun);
    score.max_level = small_number(this->max_lev);
    score.max_dungeon_level = small_number(this->max_dun);
    score.died_from = field(this->how);
    return score;
}
