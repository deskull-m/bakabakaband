#pragma once

#include <functional>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <string_view>

class CreatureEntity;
class PlayerType;
class StoreScreen;

inline constexpr auto BOT_JSON_PROTOCOL_VERSION = 2; //!< JSONスナップショットの互換性を壊す変更で更新する

enum class BotKnowledgeCategory {
    ARTIFACTS_KNOWN,
    ARTIFACTS_IDENTIFIED,
    OBJECTS_KNOWN,
    UNIQUES_ALIVE,
    UNIQUES_DEAD,
    BOUNTY,
    HOME,
    EQUIP_RESISTANCES,
    FEATURES,
    SELF_INFO,
    MUTATIONS,
    WEAPON_EXP,
    SPELL_EXP,
    SKILL_EXP,
    VIRTUES,
    DUNGEONS,
    QUESTS,
    PETS,
    AUTOPICK,
    MAX,
};

std::string to_json_utf8(std::string_view str);
nlohmann::json make_message_history_json(int count);
nlohmann::json make_bot_json_snapshot(CreatureEntity &creature, bool include_map = true);
void output_bot_json_snapshot(CreatureEntity &creature);
void output_bot_json_store_snapshot(CreatureEntity &creature, const StoreScreen &screen);
void output_bot_json_character_snapshot(CreatureEntity &creature);
void output_bot_json_knowledge_snapshot(CreatureEntity &creature, BotKnowledgeCategory category);
void output_bot_json_look_snapshot(CreatureEntity &creature);

/*!
 * @brief 開いている画面の情報を、制御サーバの state のスナップショットに足すためのもの
 */
struct BotScreen {
    std::function<void(nlohmann::json &snapshot, PlayerType *player_ptr)> add_to_snapshot; //!< スナップショットの type を変え、画面の情報を足す
    bool include_map = true; //!< 地図を載せてよいか (state の map の指定より優先して地図を省く)
};

/*!
 * @brief 画面を開いている間、その画面を登録する
 * @details 制御サーバの state は、登録された画面があれば、JSONL のその画面のスナップショットと同じく
 * type を変えて画面の情報を載せる。入れ子に登録したときは、最後に登録した画面が有効になる。
 */
class BotScreenScope {
public:
    explicit BotScreenScope(BotScreen screen);
    ~BotScreenScope();

    BotScreenScope(const BotScreenScope &) = delete;
    BotScreenScope &operator=(const BotScreenScope &) = delete;
    BotScreenScope(BotScreenScope &&) = delete;
    BotScreenScope &operator=(BotScreenScope &&) = delete;

private:
    BotScreen registered; //!< 登録した画面
    const BotScreen *previous; //!< 前に登録されていた画面
};

BotScreen make_bot_store_screen(const StoreScreen &screen);
BotScreen make_bot_store_screen(StoreScreen &&screen) = delete;
