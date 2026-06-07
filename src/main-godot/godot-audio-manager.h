#pragma once
/*!
 * @file godot-audio-manager.h
 * @brief BGM・SE管理ノード (Godot AudioStreamPlayer バックエンド)
 *
 * music.cfg / sound.cfg を読み込み、TERM_XTRA_SOUND / TERM_XTRA_MUSIC_* を処理する。
 * - BGM: AudioStreamPlayer 1台 (ループ再生)
 * - SE: AudioStreamPlayer 8台のプール (ポリフォニー)
 */

#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/classes/node.hpp>

#include <array>
#include <map>
#include <string>
#include <vector>

namespace hengband_godot {

class GodotAudioManager : public godot::Node {
    GDCLASS(GodotAudioManager, godot::Node)

public:
    GodotAudioManager() = default;
    ~GodotAudioManager() override = default;

    void _ready() override;

    /// BGMアセットディレクトリを設定する (絶対パスまたは res:// パス)
    void set_music_dir(const godot::String &dir);
    /// SEアセットディレクトリを設定する
    void set_sound_dir(const godot::String &dir);

    /// music.cfg を読み込む
    bool load_music_config(const godot::String &cfg_path);
    /// sound.cfg を読み込む
    bool load_sound_config(const godot::String &cfg_path);

    /// BGMを再生する (TERM_XTRA_MUSIC_* から呼ばれる)
    bool play_music(int type, int val);
    /// BGMを停止する (TERM_XTRA_MUSIC_MUTE から呼ばれる)
    void stop_music();
    /// SEを再生する (TERM_XTRA_SOUND から呼ばれる)
    void play_sound(int val);

    /// BGM音量を設定する (level: 0=100%, 9=最小)
    void set_music_volume(int level);
    /// SE音量を設定する
    void set_sound_volume(int level);

    /// BGMを一時停止/再開する (ウィンドウ非アクティブ時)
    void set_music_paused(bool paused);

protected:
    static void _bind_methods();

private:
    static constexpr int SE_POOL_SIZE = 8;

    godot::AudioStreamPlayer *bgm_player_{ nullptr };
    std::array<godot::AudioStreamPlayer *, SE_POOL_SIZE> se_players_{};
    int se_pool_idx_{ 0 };

    std::string music_dir_;
    std::string sound_dir_;

    /// music.cfg: section -> (key -> filenames)
    std::map<std::string, std::map<std::string, std::vector<std::string>>> music_cfg_;
    /// sound.cfg: key -> filenames
    std::map<std::string, std::vector<std::string>> sound_cfg_;

    int current_music_type_{ -1 };
    int current_music_val_{ -1 };
    int music_volume_level_{ 0 };
    int sound_volume_level_{ 0 };

    /// INIファイルを解析して section -> (key -> values) を返す
    static std::map<std::string, std::map<std::string, std::vector<std::string>>>
    parse_ini(const std::string &path);

    /// 音量レベル(0-9)を線形値(0.0-1.0)に変換する
    static float volume_level_to_linear(int level);

    /// ファイルパスから AudioStream を読み込む (mp3/ogg/wav 対応)
    static godot::Ref<godot::AudioStream> load_audio_file(const std::string &path);

    /// (type, val) に対応するファイル名をランダムに選択する
    std::string pick_music_file(int type, int val) const;
    /// val(SoundKind) に対応するファイル名をランダムに選択する
    std::string pick_sound_file(int val) const;
};

/// GodotAudioManager をグローバル参照として登録する
void set_audio_manager(GodotAudioManager *manager);

/// TERM_XTRA_SOUND / MUSIC_* 用のグローバルアクセス関数
bool audio_play_music(int type, int val);
void audio_stop_music();
void audio_play_sound(int val);
void audio_set_music_paused(bool paused);

} // namespace hengband_godot
