#pragma once

class CreatureEntity;
class WorldTurnProcessor {
public:
    WorldTurnProcessor(CreatureEntity &creature);
    WorldTurnProcessor &operator=(const WorldTurnProcessor &) = delete;
    WorldTurnProcessor &operator=(WorldTurnProcessor &&) = delete;
    virtual ~WorldTurnProcessor() = default;
    void process_world();
    void print_time();
    void print_world_collapse();
    void print_cheat_position();

private:
    CreatureEntity &creature;
    int hour = 0;
    int min = 0;

    void process_downward();
    void process_monster_arena();
    void process_monster_arena_winner(int win_m_idx);
    void process_monster_arena_draw();
    void decide_auto_save();
    void process_change_daytime_night();
    void process_world_monsters();
    void decide_alloc_monster();
    void ring_nightmare_bell(int prev_min);
};
