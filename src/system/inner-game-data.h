#pragma once

#include "player-info/class-types.h"
#include "util/flag-group.h"
#include <cstddef>
#include <cstdint>

enum term_color_type : unsigned char;
enum class PlayerRaceType : int;
class InnerGameData {
public:
    InnerGameData(InnerGameData &&) = delete;
    InnerGameData(const InnerGameData &) = delete;
    InnerGameData &operator=(const InnerGameData &) = delete;
    InnerGameData &operator=(InnerGameData &&) = delete;
    ~InnerGameData() = default;
    static InnerGameData &get_instance();

    PlayerRaceType get_start_race() const;
    int get_game_turn_limit() const;
    int get_real_turns(int turns) const;
    void set_start_race(PlayerRaceType race);
    void init_turn_limit();

    uint32_t get_total_play_time() const;
    void set_total_play_time(uint32_t time);
    void add_play_time(uint32_t time);

    bool is_no_score() const;
    uint16_t get_no_score() const;
    void initialize_no_score();
    void add_no_score(uint16_t new_state);

    term_color_type get_birth_class_color(PlayerClassType c) const;

    size_t get_won_classes_count() const;
    bool has_not_won_class(PlayerClassType pc) const;
    const EnumClassFlagGroup<PlayerClassType> &get_won_classes() const;
    void add_winner_class(PlayerClassType c);
    void set_won_classes(const EnumClassFlagGroup<PlayerClassType> &classes);

    bool has_not_retired_class(PlayerClassType pc) const;
    const EnumClassFlagGroup<PlayerClassType> &get_retired_classes() const;
    void add_retired_class(PlayerClassType c);
    void set_retired_classes(const EnumClassFlagGroup<PlayerClassType> &classes);

private:
    InnerGameData();
    static InnerGameData instance;

    PlayerRaceType start_race; //!< ゲーム開始時の種族.
    int game_turn_limit = 0; //!< 最大ゲームターン
    uint32_t total_play_time = 0; //!< このセーブファイルで遊んだ合計のプレイ時間.
    uint16_t noscore = 0; //!< ゲーム内デバッグモードのON／OFF
    EnumClassFlagGroup<PlayerClassType> won_classes{}; //!< このセーブファイルで*勝利*した職業
    EnumClassFlagGroup<PlayerClassType> retired_classes{}; //!< このセーブファイルで引退した職業

    bool is_winner_class(PlayerClassType c) const;
    bool is_retired_class(PlayerClassType c) const;
};
