/*!
 * @file save-file-scanner.cpp
 * @brief lib/save/ 内のセーブファイルを解析してキャラクタ情報を返す独立スキャナ
 *
 * Hengband のロードインフラ (グローバル状態・シングルトン) を一切使わず、
 * バイナリを直接読み出す自己完結実装。
 * 対象は SAVEFILE_VERSION >= 4 のセーブファイル (変愚蛮怒 2.x 以降)。
 */

#include "save-file-scanner.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace hengband_godot {

// ---------------------------------------------------------------------------
// パスエンコーディング変換ユーティリティ
// ---------------------------------------------------------------------------

#ifdef _WIN32
/// Windows UTF-16 wstring → UTF-8 std::string
static std::string wstring_to_utf8(const std::wstring &wstr)
{
    if (wstr.empty()) {
        return {};
    }
    const int len = ::WideCharToMultiByte(
        CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 1) {
        return {};
    }
    std::string result(len - 1, '\0');
    ::WideCharToMultiByte(
        CP_UTF8, 0, wstr.c_str(), -1, result.data(), len, nullptr, nullptr);
    return result;
}
#else
static std::string wstring_to_utf8(const std::wstring &wstr)
{
    // 非 Windows: wchar_t は UTF-32 (Linux) / UTF-16 (Mac)
    // filesystem::path::u8string() が正しく動作する前提
    (void)wstr;
    return {};
}
#endif

namespace {

    // ---------------------------------------------------------------------------
    // 最小 XOR ストリームリーダ
    // ---------------------------------------------------------------------------

    struct SaveReader {
        const std::vector<uint8_t> &data;
        size_t pos = 0;
        uint8_t xor_byte = 0;

        // ゲームバージョン (セーブファイルヘッダから読み取る)
        uint8_t ver_major = 0;
        uint8_t ver_minor = 0;
        uint8_t ver_patch = 0;
        uint8_t ver_extra = 0;
        // セーブファイルフォーマットバージョン (SAVEFILE_VERSION)
        uint32_t sv_version = 0;

        explicit SaveReader(const std::vector<uint8_t> &d)
            : data(d)
        {
        }

        // ---- 基本読み取り ----

        uint8_t raw_byte()
        {
            if (pos >= data.size()) {
                throw std::runtime_error("SaveReader: unexpected EOF");
            }
            return data[pos++];
        }

        uint8_t rd_byte()
        {
            const uint8_t c = raw_byte();
            const uint8_t v = c ^ xor_byte;
            xor_byte = c;
            return v;
        }

        uint16_t rd_u16b()
        {
            const uint16_t lo = rd_byte();
            const uint16_t hi = rd_byte();
            return lo | (hi << 8);
        }

        int16_t rd_s16b()
        {
            return static_cast<int16_t>(rd_u16b());
        }

        uint32_t rd_u32b()
        {
            uint32_t v = rd_byte();
            v |= static_cast<uint32_t>(rd_byte()) << 8;
            v |= static_cast<uint32_t>(rd_byte()) << 16;
            v |= static_cast<uint32_t>(rd_byte()) << 24;
            return v;
        }

        int32_t rd_s32b()
        {
            return static_cast<int32_t>(rd_u32b());
        }

        void strip(int n)
        {
            while (n-- > 0) {
                rd_byte();
            }
        }

        std::string rd_string()
        {
            std::string s;
            for (;;) {
                const auto ch = static_cast<char>(rd_byte());
                if (ch == '\0') {
                    break;
                }
                s.push_back(ch);
            }
            return s;
        }

        // FlagGroup: 先頭 2 バイトがデータサイズ、続いてそのバイト数をスキップ
        void skip_flaggroup()
        {
            const uint16_t sz = rd_byte() | (static_cast<uint16_t>(rd_byte()) << 8);
            strip(sz);
        }

        // ---- バージョン判定ヘルパ ----

        bool sv_older_than(uint32_t v) const
        {
            return sv_version < v;
        }

        bool h_older_than(int maj, int min, int pat, int ext = 0) const
        {
            if (ver_major != maj) {
                return ver_major < maj;
            }
            if (ver_minor != min) {
                return ver_minor < min;
            }
            if (ver_patch != pat) {
                return ver_patch < pat;
            }
            return ver_extra < ext;
        }

        // ---- セクション単位スキップ ----

        // 14 バイト生ヘッダ読み込み + XOR ストリーム開始設定
        bool read_header()
        {
            // 最低 14 バイト必要
            if (data.size() < 14) {
                return false;
            }

            // Byte 0: variant_length (生バイト, XOR=0)
            const uint8_t vlen = raw_byte();
            if (vlen != 8) { // "Hengband" の長さ
                return false;
            }

            // Bytes 1-8: "Hengband" (生バイト、検証省略)
            for (int i = 0; i < 8; ++i) {
                raw_byte();
            }

            // XOR をリセットして生バイトとしてバージョンを読む
            xor_byte = 0;
            ver_major = rd_byte();
            ver_minor = rd_byte();
            ver_patch = rd_byte();
            ver_extra = rd_byte();

            // Byte 13: XOR キー (rd_byte で読むと xor_byte が更新される)
            rd_byte();

            // data[13] が実際の XOR キー
            xor_byte = data[13];

            // XOR ストリーム開始 (Byte 14 から)
            // sf_system(4) + sf_when(4) + sf_lives(2) + sf_saves(2) = 12 バイト
            strip(12);

            // SAVEFILE_VERSION
            sv_version = rd_u32b();
            return true;
        }

        void skip_dummy3()
        {
            strip(3);
        }

        void skip_randomizer()
        {
            // strip 4: 旧乱数状態の残骸
            strip(4);
            // Xoshiro128StarStar の state (4 × u32 = 16 バイト)
            strip(4 * 4);
            // 残り RAND_DEG(=63) - 4 個を strip (4 × 59 = 236 バイト)
            strip(4 * 59);
        }

        void skip_options()
        {
            strip(16);
            // delay_factor: version >= 9 → s32、それ以前 → byte
            if (!sv_older_than(9)) {
                strip(4);
            } else {
                strip(1);
            }
            strip(1); // hitpoint_warn
            if (!h_older_than(1, 7, 0, 0)) {
                strip(1); // mana_warn
            }
            strip(2); // c (u16、チートフラグ等)
            strip(1 + 1 + 2); // autosave_l, autosave_t, autosave_freq
            strip(8 * 4 + 8 * 4); // flag[8] + mask[8]
            // h_older_than(0,4,5): load_zangband_options → 現代版は不要、スキップなし
            strip(8 * 4); // savefile_window_flags: 8 × FlagGroup_bytes(4)
            strip(8 * 4); // savefile_window_masks: 8 × FlagGroup_bytes(4)
        }

        void skip_messages()
        {
            if (!sv_older_than(22)) {
                // rd_message_history: count(s32) + count × (string + s16)
                const int32_t count = rd_s32b();
                for (int32_t i = 0; i < count; ++i) {
                    rd_string();
                    strip(2); // repeat_count (s16)
                }
            } else {
                // 旧形式: count + count × string
                uint32_t count;
                if (!h_older_than(2, 2, 0, 75)) {
                    count = rd_u32b();
                } else {
                    count = rd_u16b();
                }
                for (uint32_t i = 0; i < count; ++i) {
                    rd_string();
                }
            }
        }

        void skip_system_info()
        {
            strip(1); // loading_character_encoding
            skip_randomizer();
            skip_options();
            skip_messages();
        }

        void skip_one_lore()
        {
            strip(2 + 2 + 2); // r_sights, r_deaths, r_pkills
            if (!h_older_than(1, 7, 0, 5)) {
                strip(2); // r_akills
            }
            strip(2); // r_tkills
            strip(1 + 1); // r_wake, r_ignore
            strip(1); // r_can_evolve
            if (sv_older_than(6)) {
                strip(1); // r_xtra2 (廃止フラグ)
            }
            strip(1 + 1 + 1 + 1); // r_drop_gold, r_drop_item, strip1, r_cast_spell
            strip(4); // r_blows[0..3]

            if (sv_older_than(21)) {
                // 旧フラグマイグレーション経路
                /*const auto r_flags1 =*/rd_u32b();
                /*const auto r_flags2 =*/rd_u32b();
                /*const auto r_flags3 =*/rd_u32b();

                // migrate_old_resistance_and_ability_flags
                if (sv_older_than(3)) {
                    strip(4 + 4 + 4); // f4, f5, f6
                    if (!h_older_than(1, 5, 0, 3)) {
                        strip(4); // r_flagsr
                    }
                } else if (sv_older_than(14)) {
                    strip(4); // r_flagsr
                    skip_flaggroup(); // r_ability_flags
                } else {
                    skip_flaggroup(); // r_resistance_flags
                    skip_flaggroup(); // r_ability_flags
                }

                if (!sv_older_than(10)) {
                    skip_flaggroup();
                } // r_aura_flags
                if (!sv_older_than(11)) {
                    skip_flaggroup();
                } // r_behavior_flags
                if (!sv_older_than(12)) {
                    skip_flaggroup();
                } // r_kind_flags
                if (!sv_older_than(18)) {
                    skip_flaggroup();
                } // r_drop_flags
                if (!sv_older_than(19)) {
                    skip_flaggroup();
                } // r_feature_flags

                // sv_version 20 のみ r_special_flags が存在
                if (!sv_older_than(20)) {
                    skip_flaggroup(); // r_special_flags
                }
                // migrate_old_misc_flags: sv_version >= 20 → FlagGroup 読み込み
                if (!sv_older_than(20)) {
                    skip_flaggroup(); // r_misc_flags
                }
            } else {
                // 現代版 (sv_version >= 21): 9 個の FlagGroup
                for (int i = 0; i < 9; ++i) {
                    skip_flaggroup();
                }
            }

            strip(1 + 2); // max_num, floor_id
            if (!sv_older_than(4)) {
                strip(2 + 4); // defeat_level, defeat_time
            }
            strip(1); // パディング
        }

        void skip_lore()
        {
            const uint16_t count = rd_u16b();
            for (uint16_t i = 0; i < count; ++i) {
                skip_one_lore();
            }
        }

        void skip_items()
        {
            const uint16_t count = rd_u16b();
            strip(count); // aware/tried フラグ 1 バイト × count
        }

        void skip_quests()
        {
            if (h_older_than(2, 1, 3)) {
                return;
            }

            rd_u16b(); // load_town: max_towns_load

            const uint16_t max_quests = rd_u16b();
            uint8_t max_rquests;
            if (!h_older_than(1, 0, 7)) {
                max_rquests = rd_byte();
            } else {
                max_rquests = 10;
            }

            // MIN_RANDOM_QUEST = 40 (QuestId::RANDOM_QUEST1)
            constexpr int MIN_RANDOM_QUEST = 40;

            for (uint16_t i = 0; i < max_quests; ++i) {
                uint16_t quest_id_val;
                if (!sv_older_than(17)) {
                    quest_id_val = static_cast<uint16_t>(rd_s16b());
                } else {
                    quest_id_val = i;
                }

                // load_quest_completion
                const int16_t status = rd_s16b();
                strip(2); // level
                if (!h_older_than(1, 0, 6)) {
                    strip(1);
                } // complev
                if (!h_older_than(2, 1, 2, 2)) {
                    strip(4);
                } // comptime

                // クエスト進行中かどうか判定 (QuestStatusType: TAKEN=1, COMPLETED=2)
                bool is_running = (status == 1); // TAKEN
                is_running |= (!h_older_than(0, 3, 14) && (status == 2)); // COMPLETED
                is_running |= (!h_older_than(1, 0, 7) && quest_id_val >= MIN_RANDOM_QUEST && quest_id_val <= static_cast<uint16_t>(MIN_RANDOM_QUEST + max_rquests));

                if (!is_running) {
                    continue;
                }

                // load_quest_details
                strip(2 + 2 + 2 + 2 + 2 + 1); // cur_num, max_num, type, r_idx, reward_fa_id, flags
                if (!h_older_than(0, 3, 11)) {
                    strip(1); // dungeon
                }
            }

            // load_wilderness_info
            strip(4 + 4); // player x, y
            if (!h_older_than(0, 3, 7)) {
                strip(1 + 1); // wild_mode, ambush_flag
            }

            // analyze_wilderness
            const int32_t wild_x = rd_s32b();
            const int32_t wild_y = rd_s32b();
            if (wild_x > 0 && wild_y > 0 && wild_x < 10000 && wild_y < 10000) {
                strip(wild_x * wild_y * 4); // seeds
            }
        }

        void skip_artifacts()
        {
            const uint16_t count = rd_u16b();
            for (uint16_t i = 0; i < count; ++i) {
                strip(1); // is_generated (bool)
                if (!h_older_than(1, 5, 0, 0)) {
                    strip(2); // floor_id
                } else {
                    strip(3); // 旧形式
                }
            }
        }

        void skip_load_quick_start()
        {
            if (h_older_than(1, 0, 13)) {
                return;
            }
            strip(1 + 1 + 1 + 1 + 1 + 1); // psex, prace, pclass, ppersonality, realm1, realm2
            strip(2 + 2 + 2 + 2); // age, ht, wt, sc (s16 各)
            strip(4); // au (s32)
            strip(6 * 2); // stat_max[A_MAX=6] (s16 各)
            strip(6 * 2); // stat_max_max[A_MAX=6]
            strip(50 * 2); // player_hp[PY_MAX_LEVEL=50] (s16 各)
            strip(2); // chaos_patron
            strip(8 * 2); // vir_types[8]
            for (int i = 0; i < 4; ++i) {
                rd_string(); // history[4] (可変長)
            }
            strip(1 + 1); // strip1 パディング + quick_ok
        }

        CharacterSummary scan()
        {
            CharacterSummary result;

            if (!read_header()) {
                return result;
            }

            skip_dummy3();
            skip_system_info();
            skip_lore();
            skip_items();
            skip_quests();
            skip_artifacts();

            // rd_total_play_time (sv_version >= 4)
            if (!sv_older_than(4)) {
                strip(4);
            }

            // rd_winner_class (sv_version >= 4)
            if (!sv_older_than(4)) {
                skip_flaggroup(); // sf_winner
                skip_flaggroup(); // sf_retired
            }

            // rd_base_info
            result.char_name = rd_string(); // キャラクタ名
            rd_string(); // died_from
            if (!h_older_than(1, 7, 0, 1)) {
                rd_string(); // last_message
            }
            skip_load_quick_start();
            for (int i = 0; i < 4; ++i) {
                rd_string(); // history[4]
            }

            result.prace = rd_byte();
            result.pclass = rd_byte();
            result.ppersonality = rd_byte();
            strip(1); // psex

            // rd_realms: 常に 2 バイト (クラス問わず)
            strip(2);
            strip(1); // strip_bytes(1) in rd_base_info

            // rd_base_info 残り: hit_dice(1) + expfact(2) + age(2) + ht(2) + wt(2)
            strip(1 + 2 + 2 + 2 + 2);

            // rd_player_info → rd_player_status → rd_base_status + strip(24) + au
            // rd_base_status: 3 × A_MAX(=6) × 2 = 36 バイト
            strip(36);
            strip(24); // strip_bytes(24)
            strip(4); // au (s32)

            // rd_experience: max_exp(4) + max_max_exp(4) + exp(4) + exp_frac(4 or 2) + lev(2)
            strip(4); // max_exp
            if (!h_older_than(1, 5, 4, 1)) {
                strip(4); // max_max_exp
            }
            strip(4); // exp
            if (!h_older_than(1, 7, 0, 3)) {
                strip(4); // exp_frac (u32)
            } else {
                strip(2); // exp_frac 旧形式 (u16)
            }
            result.level = rd_s16b(); // lev

            result.valid = true;
            return result;
        }
    };

} // anonymous namespace

// ---------------------------------------------------------------------------
// 公開インタフェース
// ---------------------------------------------------------------------------

std::vector<CharacterSummary> scan_save_files(const std::filesystem::path &save_dir)
{
    std::vector<CharacterSummary> results;

    std::error_code ec;
    if (!std::filesystem::exists(save_dir, ec) || !std::filesystem::is_directory(save_dir, ec)) {
        return results;
    }

    for (const auto &entry : std::filesystem::directory_iterator(save_dir, ec)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        const auto &path = entry.path();

        // ファイル内容をメモリに読み込む
        std::ifstream f(path, std::ios::binary);
        if (!f) {
            continue;
        }
        const std::vector<uint8_t> file_data(
            (std::istreambuf_iterator<char>(f)),
            std::istreambuf_iterator<char>());
        f.close();

        try {
            SaveReader reader(file_data);
            auto summary = reader.scan();
            if (summary.valid) {
                summary.path = path;
                // ファイル名を UTF-8 に変換してキャラクタ名に使う
                // (Win32 API 経由で正確に UTF-16 → UTF-8 変換)
#ifdef _WIN32
                summary.char_name = wstring_to_utf8(path.stem().wstring());
#else
                const auto stem_u8 = path.stem().u8string();
                summary.char_name = std::string(
                    reinterpret_cast<const char *>(stem_u8.c_str()),
                    stem_u8.size());
#endif
                results.push_back(std::move(summary));
            }
        } catch (...) {
            // 解析失敗は無視して次のファイルへ
        }
    }

    // レベル降順でソート
    std::sort(results.begin(), results.end(), [](const CharacterSummary &a, const CharacterSummary &b) {
        return a.level > b.level;
    });

    return results;
}

} // namespace hengband_godot
