/*!
 * @file score-submission.cpp
 * @brief スコアサーバへ送信する JSON の組み立て
 * @details 構造は bakabakaband-score リポジトリの schema/ScoreSubmission.schema.json と対応する。
 * サーバはこの中の score.options と progress.* からスコアを再計算して申告値と突き合わせる。
 */

#include "io/score-submission.h"
#include "game-option/birth-options.h"
#include "io-dump/player-status-dump-json.h"
#include "locale/character-encoding.h"
#include "market/arena-entry.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/class-types.h"
#include "player-info/race-types.h"
#include "player/player-personality-types.h"
#include "player/player-status.h"
#include "system/angband-system.h"
#include "system/creature-entity.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/dungeon/dungeon-record.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/inner-game-data.h"
#include "system/services/dungeon-service.h"
#include "world/world.h"
#include <algorithm>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <random>

namespace {

constexpr auto SUBMISSION_FORMAT = "bakabakaband-score-submission";
constexpr auto SUBMISSION_FORMAT_VERSION = 1;

/*!
 * @brief システム文字コードの文字列を UTF-8 にして返す。変換できなければそのまま返す
 */
std::string to_utf8_or_raw(std::string_view str)
{
    const auto converted = sys_to_utf8(str);
    return converted ? *converted : std::string(str);
}

const char *platform_name()
{
#if defined(WINDOWS)
    return "win";
#elif defined(USE_GODOT)
    return "godot";
#else
    return "unix";
#endif
}

nlohmann::json make_score_section(CreatureEntity &creature)
{
    const auto &entries = ArenaEntryList::get_instance();
    nlohmann::json score;
    score["points"] = calc_score(creature);
    score["multiplier"] = calc_score_multiplier();
    score["options"] = {
        { "preserve_mode", preserve_mode },
        { "autoroller", autoroller },
        { "smart_learn", smart_learn },
        { "smart_cheat", smart_cheat },
        { "ironman_shops", ironman_shops },
        { "ironman_smallest_floor", ironman_smallest_floor },
        { "ironman_force_arena_floor", ironman_force_arena_floor },
        { "powerup_home", powerup_home },
        { "ironman_rooms", ironman_rooms },
        { "ironman_nightmare", ironman_nightmare },
        { "ironman_downward", ironman_downward },
    };
    score["arena_wins"] = std::min(entries.get_current_entry(), entries.get_max_entries());
    score["spectre_berserker"] = CreatureClass(creature).equals(PlayerClassType::BERSERKER) && CreatureRace(&creature).equals(PlayerRaceType::SPECTRE);
    score["munchkin"] = creature.get_ppersonality() == PERSONALITY_MUNCHKIN;
    return score;
}

nlohmann::json make_progress_section(CreatureEntity &creature)
{
    const auto &world = AngbandWorld::get_instance();
    const auto &igd = InnerGameData::get_instance();
    nlohmann::json progress;
    progress["max_level"] = creature.get_max_plv();
    progress["max_max_experience"] = creature.get_max_max_exp();
    progress["real_turns"] = igd.get_real_turns(world.game_turn);
    progress["play_time_sec"] = world.play_time.elapsed_sec();
    progress["max_depth_overall"] = DungeonService::find_max_level();

    nlohmann::json max_depth = nlohmann::json::object();
    const auto &records = DungeonRecords::get_instance();
    const auto &dungeons = DungeonList::get_instance();
    for (const auto dungeon_id : DUNGEON_IDS) {
        const auto &record = records.get_record(dungeon_id);
        if (!record.has_entered()) {
            continue;
        }

        max_depth[dungeons.get_dungeon(dungeon_id).tag] = record.get_max_level();
    }

    progress["max_depth"] = max_depth;
    progress["death_count"] = creature.get_death_count();
    progress["total_winner"] = world.total_winner > 0;
    progress["true_winner"] = creature.is_true_winner();
    return progress;
}
}

nlohmann::json make_score_submission(CreatureEntity &creature, const std::string &character_dump, const nlohmann::json &screen)
{
    nlohmann::json submission;
    submission["format"] = SUBMISSION_FORMAT;
    submission["format_version"] = SUBMISSION_FORMAT_VERSION;
    submission["submission_id"] = generate_submission_id();
    submission["game"] = {
        { "version", to_utf8_or_raw(AngbandSystem::get_instance().build_version_expression(VersionExpression::FULL)) },
        { "platform", platform_name() },
        { "locale", _("ja", "en") },
    };
    submission["score"] = make_score_section(creature);
    submission["progress"] = make_progress_section(creature);
    submission["character"] = make_creature_status_json(creature);

    nlohmann::json attachments = nlohmann::json::object();
    if (!character_dump.empty()) {
        attachments["character_dump"] = to_utf8_or_raw(character_dump);
    }

    if (!screen.is_null()) {
        attachments["screen"] = screen;
    }

    if (!attachments.empty()) {
        submission["attachments"] = attachments;
    }

    return submission;
}

std::string generate_submission_id()
{
    static std::mt19937_64 engine{ std::random_device{}() };
    std::uniform_int_distribution<uint64_t> dist;
    auto hi = dist(engine);
    auto lo = dist(engine);
    hi = (hi & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL; // version 4
    lo = (lo & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL; // variant 1
    char buf[37];
    std::snprintf(buf, sizeof(buf), "%08x-%04x-%04x-%04x-%012llx",
        static_cast<unsigned>(hi >> 32), static_cast<unsigned>((hi >> 16) & 0xFFFF), static_cast<unsigned>(hi & 0xFFFF),
        static_cast<unsigned>(lo >> 48), static_cast<unsigned long long>(lo & 0xFFFFFFFFFFFFULL));
    return buf;
}
