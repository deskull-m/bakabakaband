/*!
 * @file godot-audio-manager.cpp
 * @brief BGM・SE管理ノードの実装
 */

#include "godot-audio-manager.h"

#include "main/sound-definitions-table.h"
#include "term/z-term.h"

#include <godot_cpp/classes/audio_stream_mp3.hpp>
#include <godot_cpp/classes/audio_stream_ogg_vorbis.hpp>
#include <godot_cpp/classes/audio_stream_wav.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <random>
#include <sstream>

using namespace godot;
using namespace hengband_godot;

// ---------------------------------------------------------------------------
// [Basic] セクションのキー名 (angband_music_basic_name に対応)
// ---------------------------------------------------------------------------
static const char *const MUSIC_BASIC_KEYS[] = {
    "new_game", "gameover", "exit", "town",
    "field1", "field2", "field3",
    "dun_low", "dun_med", "dun_high",
    "feel1", "feel2", "winner", "build", "wild",
    "quest", "arena", "battle",
    "quest_clear", "final_quest_clear", "ambush",
    "unique", "shadower", "unk_monster", "hl_monster"
};
static constexpr int MUSIC_BASIC_MAX = 25;

// ---------------------------------------------------------------------------
// 音量テーブル (Windows版 VOLUME_TABLE と同値、1000=100%)
// ---------------------------------------------------------------------------
static constexpr int VOLUME_TABLE[10] = { 1000, 800, 600, 450, 350, 250, 170, 100, 50, 20 };

// ---------------------------------------------------------------------------
// グローバルオーディオマネージャポインタ
// ---------------------------------------------------------------------------
static GodotAudioManager *s_audio_manager = nullptr;

void hengband_godot::set_audio_manager(GodotAudioManager *manager)
{
    s_audio_manager = manager;
}

bool hengband_godot::audio_play_music(int type, int val)
{
    if (s_audio_manager) {
        return s_audio_manager->play_music(type, val);
    }
    return false;
}

void hengband_godot::audio_stop_music()
{
    if (s_audio_manager) {
        s_audio_manager->stop_music();
    }
}

void hengband_godot::audio_play_sound(int val)
{
    if (s_audio_manager) {
        s_audio_manager->play_sound(val);
    }
}

void hengband_godot::audio_set_music_paused(bool paused)
{
    if (s_audio_manager) {
        s_audio_manager->set_music_paused(paused);
    }
}

// ---------------------------------------------------------------------------
// INIパーサ (シンプル実装)
// ---------------------------------------------------------------------------
std::map<std::string, std::map<std::string, std::vector<std::string>>>
GodotAudioManager::parse_ini(const std::string &path)
{
    std::map<std::string, std::map<std::string, std::vector<std::string>>> result;
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        return result;
    }

    std::string current_section;
    std::string line;
    while (std::getline(ifs, line)) {
        // BOM除去
        if (line.size() >= 3 &&
            static_cast<uint8_t>(line[0]) == 0xEF &&
            static_cast<uint8_t>(line[1]) == 0xBB &&
            static_cast<uint8_t>(line[2]) == 0xBF) {
            line = line.substr(3);
        }
        // コメント・空行
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        // セクション
        if (line[0] == '[') {
            const auto end = line.find(']');
            if (end != std::string::npos) {
                current_section = line.substr(1, end - 1);
            }
            continue;
        }
        // key = val1 val2 ...
        const auto eq = line.find('=');
        if (eq == std::string::npos || current_section.empty()) {
            continue;
        }
        std::string key = line.substr(0, eq);
        // trim key
        while (!key.empty() && std::isspace(static_cast<uint8_t>(key.back()))) {
            key.pop_back();
        }
        std::string val_str = line.substr(eq + 1);
        // インラインコメントを除去
        const auto comment = val_str.find('#');
        if (comment != std::string::npos) {
            val_str = val_str.substr(0, comment);
        }

        std::vector<std::string> values;
        std::istringstream ss(val_str);
        std::string token;
        while (ss >> token) {
            values.push_back(token);
        }
        if (!values.empty()) {
            result[current_section][key] = std::move(values);
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// 音量変換
// ---------------------------------------------------------------------------
float GodotAudioManager::volume_level_to_linear(int level)
{
    if (level < 0) {
        level = 0;
    }
    if (level > 9) {
        level = 9;
    }
    return static_cast<float>(VOLUME_TABLE[level]) / 1000.0f;
}

// ---------------------------------------------------------------------------
// オーディオファイルのロード
// ---------------------------------------------------------------------------
Ref<AudioStream> GodotAudioManager::load_audio_file(const std::string &path)
{
    if (path.empty()) {
        return {};
    }

    const String gd_path(path.c_str());

    // res:// または user:// パスはリソースローダーで読む
    if (gd_path.begins_with("res://") || gd_path.begins_with("user://")) {
        return ResourceLoader::get_singleton()->load(gd_path);
    }

    // 絶対パス: FileAccess でバイト列を読んで AudioStream を生成
    Ref<FileAccess> fa = FileAccess::open(gd_path, FileAccess::READ);
    if (!fa.is_valid()) {
        return {};
    }
    PackedByteArray data = fa->get_buffer(fa->get_length());
    fa->close();

    // 拡張子で種別判定
    std::string ext = path;
    const auto dot = ext.rfind('.');
    if (dot != std::string::npos) {
        ext = ext.substr(dot + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(),
            [](unsigned char c) { return std::tolower(c); });
    }

    if (ext == "mp3") {
        Ref<AudioStreamMP3> stream;
        stream.instantiate();
        stream->set_data(data);
        stream->set_loop(false);
        return stream;
    }
    if (ext == "ogg") {
        Ref<AudioStreamOggVorbis> stream = AudioStreamOggVorbis::load_from_buffer(data);
        return stream;
    }
    if (ext == "wav") {
        Ref<AudioStreamWAV> stream;
        stream.instantiate();
        stream->set_data(data);
        return stream;
    }
    return {};
}

// ---------------------------------------------------------------------------
// ファイル選択 (ランダム)
// ---------------------------------------------------------------------------
static std::string pick_random_file(const std::vector<std::string> &files)
{
    if (files.empty()) {
        return {};
    }
    if (files.size() == 1) {
        return files[0];
    }
    static std::mt19937 rng{ std::random_device{}() };
    std::uniform_int_distribution<size_t> dist(0, files.size() - 1);
    return files[dist(rng)];
}

std::string GodotAudioManager::pick_music_file(int type, int val) const
{
    std::string section;
    std::string key;

    switch (type) {
    case TERM_XTRA_MUSIC_BASIC:
        section = "Basic";
        if (val >= 0 && val < MUSIC_BASIC_MAX) {
            key = MUSIC_BASIC_KEYS[val];
        }
        break;
    case TERM_XTRA_MUSIC_DUNGEON:
        section = "Dungeon";
        {
            char buf[16];
            snprintf(buf, sizeof(buf), "dungeon%03d", val);
            key = buf;
        }
        break;
    case TERM_XTRA_MUSIC_QUEST:
        section = "Quest";
        {
            char buf[16];
            snprintf(buf, sizeof(buf), "quest%03d", val);
            key = buf;
        }
        break;
    case TERM_XTRA_MUSIC_TOWN:
        section = "Town";
        {
            char buf[16];
            snprintf(buf, sizeof(buf), "town%03d", val);
            key = buf;
        }
        break;
    case TERM_XTRA_MUSIC_MONSTER:
        section = "Monster";
        {
            char buf[16];
            snprintf(buf, sizeof(buf), "monster%04d", val);
            key = buf;
        }
        break;
    default:
        return {};
    }

    if (key.empty()) {
        return {};
    }
    const auto sit = music_cfg_.find(section);
    if (sit == music_cfg_.end()) {
        return {};
    }
    const auto kit = sit->second.find(key);
    if (kit == sit->second.end()) {
        return {};
    }
    return pick_random_file(kit->second);
}

std::string GodotAudioManager::pick_sound_file(int val) const
{
    const auto it = sound_names.find(static_cast<SoundKind>(val));
    if (it == sound_names.end()) {
        return {};
    }
    const std::string &key = it->second;
    const auto kit = sound_cfg_.find(key);
    if (kit == sound_cfg_.end()) {
        return {};
    }
    return pick_random_file(kit->second);
}

// ---------------------------------------------------------------------------
// _ready: 子ノードとして AudioStreamPlayer を生成
// ---------------------------------------------------------------------------
void GodotAudioManager::_ready()
{
    bgm_player_ = memnew(AudioStreamPlayer);
    bgm_player_->set_name("BGMPlayer");
    add_child(bgm_player_);

    for (int i = 0; i < SE_POOL_SIZE; ++i) {
        se_players_[i] = memnew(AudioStreamPlayer);
        se_players_[i]->set_name(String("SEPlayer") + String::num_int64(i));
        add_child(se_players_[i]);
    }

    set_audio_manager(this);
}

// ---------------------------------------------------------------------------
// 設定
// ---------------------------------------------------------------------------
void GodotAudioManager::set_music_dir(const String &dir)
{
    music_dir_ = dir.utf8().get_data();
}

void GodotAudioManager::set_sound_dir(const String &dir)
{
    sound_dir_ = dir.utf8().get_data();
}

bool GodotAudioManager::load_music_config(const String &cfg_path)
{
    music_cfg_ = parse_ini(cfg_path.utf8().get_data());
    return !music_cfg_.empty();
}

bool GodotAudioManager::load_sound_config(const String &cfg_path)
{
    auto all = parse_ini(cfg_path.utf8().get_data());
    const auto sit = all.find("Sound");
    if (sit == all.end()) {
        return false;
    }
    sound_cfg_ = sit->second;
    return !sound_cfg_.empty();
}

// ---------------------------------------------------------------------------
// BGM再生
// ---------------------------------------------------------------------------
bool GodotAudioManager::play_music(int type, int val)
{
    if (type == TERM_XTRA_MUSIC_MUTE) {
        stop_music();
        return true;
    }
    if (current_music_type_ == type && current_music_val_ == val) {
        return true; // 既に再生中
    }

    const std::string filename = pick_music_file(type, val);
    if (filename.empty()) {
        return false;
    }

    const std::string full_path = music_dir_.empty() ? filename : music_dir_ + "/" + filename;
    Ref<AudioStream> stream = load_audio_file(full_path);
    if (!stream.is_valid()) {
        return false;
    }

    bgm_player_->stop();
    bgm_player_->set_stream(stream);
    bgm_player_->set_volume_db(
        UtilityFunctions::linear_to_db(volume_level_to_linear(music_volume_level_)));
    bgm_player_->play();

    current_music_type_ = type;
    current_music_val_ = val;
    return true;
}

void GodotAudioManager::stop_music()
{
    if (bgm_player_) {
        bgm_player_->stop();
    }
    current_music_type_ = -1;
    current_music_val_ = -1;
}

// ---------------------------------------------------------------------------
// SE再生
// ---------------------------------------------------------------------------
void GodotAudioManager::play_sound(int val)
{
    const std::string filename = pick_sound_file(val);
    if (filename.empty()) {
        return;
    }

    const std::string full_path = sound_dir_.empty() ? filename : sound_dir_ + "/" + filename;
    Ref<AudioStream> stream = load_audio_file(full_path);
    if (!stream.is_valid()) {
        return;
    }

    AudioStreamPlayer *player = se_players_[se_pool_idx_];
    se_pool_idx_ = (se_pool_idx_ + 1) % SE_POOL_SIZE;

    player->stop();
    player->set_stream(stream);
    player->set_volume_db(
        UtilityFunctions::linear_to_db(volume_level_to_linear(sound_volume_level_)));
    player->play();
}

// ---------------------------------------------------------------------------
// 音量
// ---------------------------------------------------------------------------
void GodotAudioManager::set_music_volume(int level)
{
    music_volume_level_ = std::clamp(level, 0, 9);
    if (bgm_player_ && bgm_player_->is_playing()) {
        bgm_player_->set_volume_db(
            UtilityFunctions::linear_to_db(volume_level_to_linear(music_volume_level_)));
    }
}

void GodotAudioManager::set_sound_volume(int level)
{
    sound_volume_level_ = std::clamp(level, 0, 9);
}

void GodotAudioManager::set_music_paused(bool paused)
{
    if (bgm_player_) {
        bgm_player_->set_stream_paused(paused);
    }
}

// ---------------------------------------------------------------------------
// _bind_methods
// ---------------------------------------------------------------------------
void GodotAudioManager::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_music_dir", "dir"),
        &GodotAudioManager::set_music_dir);
    ClassDB::bind_method(D_METHOD("set_sound_dir", "dir"),
        &GodotAudioManager::set_sound_dir);
    ClassDB::bind_method(D_METHOD("load_music_config", "cfg_path"),
        &GodotAudioManager::load_music_config);
    ClassDB::bind_method(D_METHOD("load_sound_config", "cfg_path"),
        &GodotAudioManager::load_sound_config);
    ClassDB::bind_method(D_METHOD("play_music", "type", "val"),
        &GodotAudioManager::play_music);
    ClassDB::bind_method(D_METHOD("stop_music"),
        &GodotAudioManager::stop_music);
    ClassDB::bind_method(D_METHOD("play_sound", "val"),
        &GodotAudioManager::play_sound);
    ClassDB::bind_method(D_METHOD("set_music_volume", "level"),
        &GodotAudioManager::set_music_volume);
    ClassDB::bind_method(D_METHOD("set_sound_volume", "level"),
        &GodotAudioManager::set_sound_volume);
    ClassDB::bind_method(D_METHOD("set_music_paused", "paused"),
        &GodotAudioManager::set_music_paused);
}
