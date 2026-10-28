#pragma once

#include <functional>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <string_view>
#include <tl/optional.hpp>
#include <vector>

class CreatureEntity;
class PlayerType;
class StoreScreen;
enum class MonraceId : short;

/*!
 * @brief 呪文一覧 (spell_list) の 1 行分。print_spells() が画面に書いた内容を言語非依存で持つ
 */
struct BotSpellListRow {
    int spell_id = 0; //!< 呪文ID
    std::string name; //!< 呪文名
    std::string status; //!< available / untried / unknown / forgotten / illegible
    int level = 0; //!< 習得レベル
    int mana = 0; //!< 消費MP
    tl::optional<int> fail; //!< 失敗率 (%)
    std::string proficiency; //!< 熟練度の略記 ("[初心]" 等の画面表記)
    bool proficiency_mark = false; //!< 熟練度の前の '!'
    std::string info; //!< 効果欄 (未知・忘却・未経験の場合はその表記)
};

/*!
 * @brief 特殊能力の一覧 (power_list) の 1 行分
 */
struct BotPowerListRow {
    char letter; //!< 選択キー
    int page; //!< 表示されるページ
    std::string name; //!< 能力名
    int level; //!< 使用可能レベル
    int cost; //!< 消費MP
    int fail; //!< 失敗率 (%)
    std::string info; //!< 効果欄
};

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
void output_bot_json_spell_list_snapshot(PlayerType *player_ptr, int realm_id, const std::vector<BotSpellListRow> &rows);
void output_bot_json_power_list_snapshot(PlayerType *player_ptr, std::string_view kind, const std::vector<BotPowerListRow> &rows, int page, bool browse_mode);
void output_bot_json_lore_snapshot(PlayerType *player_ptr, MonraceId monrace_id, int lore_mode);

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

/*!
 * @brief 描いたときに中身が決まる画面 (呪文の一覧など) を、開いている間だけ登録する
 * @details 一覧を出すメニューの間などに置く。一覧を描くと、output_bot_json_*_snapshot() が
 * いちばん内側のこのスコープの登録を、描いた中身に入れ替える。一覧を隠したら hide_bot_drawn_screen() で登録を外す。
 * 登録がない間は、外側で登録された画面 (店など) が有効になる。
 * 中身を入れ替えるときに登録し直すので、このスコープより内側に BotScreenScope を置いたまま一覧を描かないこと。
 * 構築した後に登録を書き換えるので、const で宣言しないこと。
 */
class BotDrawnScreenScope {
public:
    BotDrawnScreenScope();
    ~BotDrawnScreenScope();

    BotDrawnScreenScope(const BotDrawnScreenScope &) = delete;
    BotDrawnScreenScope &operator=(const BotDrawnScreenScope &) = delete;
    BotDrawnScreenScope(BotDrawnScreenScope &&) = delete;
    BotDrawnScreenScope &operator=(BotDrawnScreenScope &&) = delete;

private:
    friend void show_bot_drawn_screen(BotScreen screen);
    friend void hide_bot_drawn_screen();

    tl::optional<BotScreenScope> registered; //!< 描いた中身の登録
    BotDrawnScreenScope *previous; //!< 外側のスコープ
};

void hide_bot_drawn_screen();
BotScreen make_bot_store_screen(const StoreScreen &screen);
BotScreen make_bot_store_screen(StoreScreen &&screen) = delete;
