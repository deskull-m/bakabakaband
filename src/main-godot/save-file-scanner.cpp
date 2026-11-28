/*!
 * @file save-file-scanner.cpp
 * @brief lib/save/ 内のセーブファイルを解析してキャラクタ情報を返す独立スキャナ
 *
 * Hengband のロードインフラ (グローバル状態・シングルトン) を一切使わず、
 * バイナリを直接読み出す自己完結実装。
 *
 * 読む順序は src/load/ の実ローダ (load.cpp の rd_savefile() 以降) と完全に揃えてある。
 * 各関数のコメントに対応するローダの関数名を書いてあるので、セーブデータの形式を変えたときは
 * そちらと合わせて直すこと (ここだけ古いままだと、タイトル画面のロード一覧のレベル・種族・職業が狂う)。
 *
 * 馬鹿馬鹿蛮怒のローダは Hengband の版番号 (h_older_than) ではなくセーブデータのフォーマット版
 * (SAVEFILE_VERSION) だけで分岐する。セーブデータに書かれるゲームの版は 0.0.x なので、
 * Hengband 2.x の版番号で分岐すると全て「古い版」と判定されて読み違える (#9320)。
 */

#include "main-godot/save-file-scanner.h"
#include "save/save-header-reserved.h"
#include "system/angband-version.h"

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace hengband_godot {

namespace {

    // ローダが参照する定数 (ゲーム本体のヘッダに依存しないよう値を写す)
    //! ヘッダの長さ: 名前の長さ (1) + 名前 + ゲームの版 (4) + XOR キー (1) + 予約領域 (12) + セーブデータの版 (4)
    constexpr auto HEADER_SIZE = 1 + VARIANT_NAME.length() + 4 + 1 + 12 + 4;
    constexpr auto RAND_DEG = 63; //!< 乱数状態の語数 (z-rand.h)
    constexpr auto RNG_STATE_WORDS = 4; //!< xoshiro 4x32 の状態語数
    constexpr auto MAX_WINDOW_ENTITIES = 8; //!< オプションのウィンドウ数 (option-flags.h)
    constexpr auto A_MAX = 6; //!< 能力値の数
    constexpr auto PY_MAX_LEVEL = 60; //!< 最大プレイヤーレベル (system-variables.h)
    constexpr auto OLD_PY_MAX_LEVEL = 50; //!< セーブ版 48 以前の最大プレイヤーレベル (birth-loader.cpp)
    constexpr auto MIN_RANDOM_QUEST = 40; //!< ランダムクエストの最初の ID (QuestId::RANDOM_QUEST1)
    constexpr auto TIMED_EFFECT_COUNT_UNTIL_V59 = 56; //!< セーブ版 53〜59 の時限効果の件数 (creature-common-loader.cpp)
    constexpr int16_t QUEST_STATUS_TAKEN = 1;
    constexpr int16_t QUEST_STATUS_COMPLETED = 2;

    /*!
     * @brief 最小の XOR ストリームリーダ (load-util.cpp の rd_byte() 等と同じ復号)
     */
    class SaveReader {
    public:
        explicit SaveReader(const std::vector<uint8_t> &data)
            : data(data)
        {
        }

        CharacterSummary scan()
        {
            CharacterSummary result;
            if (!this->read_header()) {
                return result;
            }

            result.valid = true;
            result.savefile_version = this->sv_version;
            result.play_time_sec = this->play_time_sec;
            try {
                this->read_body(result);
            } catch (const std::exception &) {
                // 途中で読めなくなった項目は -1 のまま返す (ロード自体は試せるよう一覧には残す)
            }

            return result;
        }

    private:
        const std::vector<uint8_t> &data;
        size_t pos = 0;
        uint8_t xor_byte = 0;
        uint32_t sv_version = 0;
        int64_t play_time_sec = -1;

        uint8_t raw_byte()
        {
            if (this->pos >= this->data.size()) {
                throw std::runtime_error("SaveReader: unexpected EOF");
            }

            return this->data[this->pos++];
        }

        uint8_t rd_byte()
        {
            const auto c = this->raw_byte();
            const auto v = static_cast<uint8_t>(c ^ this->xor_byte);
            this->xor_byte = c;
            return v;
        }

        uint16_t rd_u16b()
        {
            const uint16_t lo = this->rd_byte();
            const uint16_t hi = this->rd_byte();
            return static_cast<uint16_t>(lo | (hi << 8));
        }

        int16_t rd_s16b()
        {
            return static_cast<int16_t>(this->rd_u16b());
        }

        uint32_t rd_u32b()
        {
            uint32_t v = this->rd_byte();
            v |= static_cast<uint32_t>(this->rd_byte()) << 8;
            v |= static_cast<uint32_t>(this->rd_byte()) << 16;
            v |= static_cast<uint32_t>(this->rd_byte()) << 24;
            return v;
        }

        int32_t rd_s32b()
        {
            return static_cast<int32_t>(this->rd_u32b());
        }

        void strip(size_t n)
        {
            while (n-- > 0) {
                this->rd_byte();
            }
        }

        std::string rd_string()
        {
            std::string s;
            while (true) {
                const auto ch = static_cast<char>(this->rd_byte());
                if (ch == '\0') {
                    return s;
                }

                s.push_back(ch);
            }
        }

        //! rd_FlagGroup(): 先頭 2 バイトがデータのバイト数
        void skip_flag_group()
        {
            this->strip(this->rd_u16b());
        }

        bool older_than(uint32_t version) const
        {
            return this->sv_version < version;
        }

        //! info-loader.cpp の rd_version_info()
        bool read_header()
        {
            // ヘッダの途中で切れたファイルは読まない (読み進めると例外になり、一覧から外せない)
            if (this->data.size() < HEADER_SIZE) {
                return false;
            }

            // 名前がこのゲームのもの ("Bakabaka", セーブの writer と同じ VARIANT_NAME) か
            if (this->raw_byte() != VARIANT_NAME.length()) {
                return false;
            }

            for (const auto ch : VARIANT_NAME) {
                if (this->raw_byte() != static_cast<uint8_t>(ch)) {
                    return false;
                }
            }

            this->xor_byte = 0;
            this->strip(4); // ゲームの版 (major, minor, patch, extra)
            const auto key_pos = this->pos;
            this->strip(1); // XOR キー
            this->xor_byte = this->data[key_pos];

            // 旧 sf_system / sf_when / sf_lives / sf_saves の予約領域 (save-header-reserved.h)
            const auto play_time = this->rd_u32b();
            this->strip(4); // 保存した時刻
            const auto reserved_flags = this->rd_u16b();
            this->strip(2);
            if ((reserved_flags & SAVE_HEADER_HAS_PLAY_TIME) != 0) {
                this->play_time_sec = play_time;
            }

            this->sv_version = this->rd_u32b();
            return true;
        }

        void read_body(CharacterSummary &result)
        {
            this->strip(3); // rd_dummy3()
            result.character_encoding = this->rd_byte(); // rd_system_info()
            this->skip_randomizer();
            this->skip_options();
            this->skip_messages();
            this->skip_lore();
            this->skip_baseitem_records();
            this->skip_town_quest();
            this->skip_artifacts();
            this->read_player_world(result);
        }

        //! info-loader.cpp の rd_randomizer()
        void skip_randomizer()
        {
            this->strip(4);
            this->strip(4 * RAND_DEG); // 乱数状態 RNG_STATE_WORDS 語 + 残りの RAND_DEG - RNG_STATE_WORDS 語
            static_assert(RNG_STATE_WORDS <= RAND_DEG);
        }

        //! option-loader.cpp の rd_options()
        void skip_options()
        {
            this->strip(16);
            this->strip(this->older_than(12) ? 1 : 4); // delay_factor
            this->strip(1 + 1); // hitpoint_warn, mana_warn
            this->strip(2); // cheat 等のフラグ
            this->strip(1 + 1 + 2); // autosave_l, autosave_t, autosave_freq
            this->strip(4 * MAX_WINDOW_ENTITIES * 2); // flag[], mask[]
            this->strip(4 * MAX_WINDOW_ENTITIES * 2); // ウィンドウの flags / masks (各 4 バイト)
        }

        //! info-loader.cpp の rd_messages() / display-messages.cpp の rd_message_history()
        void skip_messages()
        {
            if (!this->older_than(27)) {
                const auto count = this->rd_s32b();
                for (auto i = 0; i < count; i++) {
                    this->rd_string();
                    this->strip(2); // 繰り返し回数
                }

                return;
            }

            const auto count = this->rd_u32b();
            for (auto i = 0U; i < count; i++) {
                this->rd_string();
            }
        }

        //! lore-loader.cpp の load_lore() / rd_lore()
        void skip_lore()
        {
            if (this->older_than(21)) {
                // 旧形式の思い出フラグの移行は版ごとの分岐が多いので読まない (レベル等は不明のまま)
                throw std::runtime_error("SaveReader: lore before savefile version 21 is not supported");
            }

            const auto count = this->rd_u16b();
            for (auto i = 0; i < count; i++) {
                this->strip(2 * 5); // 目撃数, r_deaths, r_pkills, r_akills, r_tkills
                this->strip(1 + 1 + 1); // r_wake, r_ignore, r_can_evolve
                if (this->older_than(8)) {
                    this->strip(1);
                }

                this->strip(1 + 1 + 1 + 1); // r_drop_gold, r_drop_item, 未使用, r_cast_spell
                this->strip(4); // r_blows[4]
                const auto flag_groups = this->older_than(33) ? 9 : 10; // 33 から r_era_flags
                for (auto j = 0; j < flag_groups; j++) {
                    this->skip_flag_group();
                }

                this->strip(this->older_than(26) ? 1 : 2); // mob_num (旧 max_num)
                this->strip(2); // floor_id
                if (!this->older_than(4)) {
                    this->strip(2 + 4); // defeat_level, defeat_time
                }

                this->strip(1);
            }
        }

        //! item-loader-base.cpp の ItemLoaderBase::load_item()
        void skip_baseitem_records()
        {
            this->strip(this->rd_u16b());
        }

        //! load.cpp の load_town_quest() (quest-loader.cpp / world-loader.cpp)
        void skip_town_quest()
        {
            this->strip(2); // load_town(): 町の数
            const auto max_quests = this->rd_u16b();
            const auto max_rquests = this->rd_byte();
            for (auto i = 0; i < max_quests; i++) {
                const auto quest_id = this->older_than(17) ? i : this->rd_s16b();
                const auto status = this->rd_s16b();
                this->strip(2 + 1 + 4); // level, complev, comptime
                auto is_running = (status == QUEST_STATUS_TAKEN) || (status == QUEST_STATUS_COMPLETED);
                is_running |= (quest_id >= MIN_RANDOM_QUEST) && (quest_id <= MIN_RANDOM_QUEST + max_rquests);
                if (!is_running) {
                    continue;
                }

                this->strip(2 * 5 + 1); // cur_num, max_num, type, r_idx, 報酬, flags
                this->strip(1); // dungeon
            }

            this->strip(4 + 4 + 1 + 1); // load_wilderness_info(): x, y, wild_mode, ambush_flag
            const auto wild_x = this->rd_s32b(); // analyze_wilderness()
            const auto wild_y = this->rd_s32b();
            if ((wild_x < 0) || (wild_y < 0) || (wild_x > 10000) || (wild_y > 10000)) {
                throw std::runtime_error("SaveReader: invalid wilderness size");
            }

            this->strip(static_cast<size_t>(wild_x) * static_cast<size_t>(wild_y) * 4);
        }

        //! item-loader-base.cpp の ItemLoaderBase::load_artifact()
        void skip_artifacts()
        {
            const auto count = this->rd_u16b();
            this->strip(static_cast<size_t>(count) * (1 + 2)); // is_generated, floor_id
        }

        //! load.cpp の load_player_world() のうち rd_base_info() の性格まで
        void read_player_world(CharacterSummary &result)
        {
            if (!this->older_than(4)) {
                this->strip(4); // rd_total_play_time(): 全キャラクターの合計 (死亡時にだけ足される)
            }

            if (!this->older_than(9)) {
                this->strip(4); // rd_world_info(): 時空崩壊度
            }

            if (!this->older_than(4)) {
                this->skip_flag_group(); // rd_winner_class(): 勝利した職業
                this->skip_flag_group(); // 引退した職業
            }

            if (!this->older_than(32)) {
                const auto alliance_count = this->rd_u16b(); // rd_alliance_base_power()
                this->strip(static_cast<size_t>(alliance_count) * (2 + 8 + 8));
            }

            // player-info-loader.cpp の rd_base_info()
            if (this->older_than(51)) {
                result.char_name = this->rd_string();
            } else {
                this->read_creature_common(result);
            }

            this->rd_string(); // died_from
            this->rd_string(); // last_message
            this->skip_quick_start();
            for (auto i = 0; i < 4; i++) {
                this->rd_string(); // 生い立ち
            }

            if (this->older_than(54)) {
                result.prace = this->rd_byte();
                result.pclass = this->rd_byte();
            }

            result.ppersonality = this->rd_byte();
        }

        //! creature-common-loader.cpp の rd_creature_common() (セーブ版 51 以降)
        void read_creature_common(CharacterSummary &result)
        {
            result.char_name = this->rd_string();
            this->strip(2 + 2); // y, x
            this->strip(4 * 3 + 4); // hp, maxhp, max_maxhp, dealt_damage
            this->strip(2 * 3); // speed, energy_need, ac
            this->strip(4 + 4); // exp, au
            this->strip(2 + 2); // ht, wt
            this->strip(2 + 2); // target
            if (this->older_than(53)) {
                this->strip(2 * 7);
            } else {
                const auto count = this->older_than(60) ? TIMED_EFFECT_COUNT_UNTIL_V59 : this->rd_u16b();
                this->strip(static_cast<size_t>(count) * 2);
            }

            this->strip(static_cast<size_t>(this->rd_u16b()) * 2); // 材質
            if (this->older_than(52)) {
                return;
            }

            result.level = this->rd_s16b();
            this->strip(2); // age
            this->strip(4 + 4 + 4 + 4); // hp_frac, max_mp, current_mp, current_mp_frac
            this->strip(4 + 4 + 4); // max_exp, max_max_exp, exp_frac
            this->strip(2 * A_MAX * 3); // stat_max, stat_max_max, stat_cur
            if (this->older_than(54)) {
                return;
            }

            result.prace = this->rd_s16b();
            result.pclass = this->rd_s16b();
        }

        //! birth-loader.cpp の load_quick_start()
        void skip_quick_start()
        {
            this->strip(6); // psex, prace, pclass, ppersonality, realm1, realm2
            this->strip(4); // death_count
            this->strip(2 * 4); // age, ht, wt, prestige
            this->strip(4); // au
            this->strip(2 * A_MAX * 2); // stat_max, stat_max_max
            this->strip(2 * (this->older_than(49) ? OLD_PY_MAX_LEVEL : PY_MAX_LEVEL)); // hp_table
            this->strip(2); // patron
            this->strip(2 * 8); // 徳の種類
            for (auto i = 0; i < 4; i++) {
                this->rd_string(); // 生い立ち
            }

            this->strip(1 + 1); // 未使用, quick_ok
        }
    };

} // namespace

CharacterSummary scan_save_data(const std::vector<uint8_t> &data)
{
    return SaveReader(data).scan();
}

std::vector<CharacterSummary> scan_save_files(const std::filesystem::path &save_dir)
{
    std::vector<CharacterSummary> results;
    std::error_code ec;
    if (!std::filesystem::is_directory(save_dir, ec)) {
        return results;
    }

    for (const auto &entry : std::filesystem::directory_iterator(save_dir, ec)) {
        if (!entry.is_regular_file(ec)) {
            continue;
        }

        std::ifstream f(entry.path(), std::ios::binary);
        if (!f) {
            continue;
        }

        // 開けた後の読み込みの失敗 (例外) はそのファイルだけ飛ばし、ほかのセーブデータは一覧に残す
        std::vector<uint8_t> file_data;
        try {
            file_data.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
        } catch (const std::exception &) {
            continue;
        }

        if (f.bad()) {
            continue;
        }

        auto summary = scan_save_data(file_data);
        if (!summary.valid) {
            continue;
        }

        summary.path = entry.path();
        results.push_back(std::move(summary));
    }

    return results;
}

} // namespace hengband_godot
