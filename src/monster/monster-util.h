#pragma once

#include "system/angband.h"
#include "system/enums/monrace/monrace-hook-types.h"
#include "util/point-2d.h"
#include <functional>
#include <tl/optional.hpp>

enum summon_type : int;
enum class MonraceId : short;
class CreatureEntity;
void get_mon_num_prep_enum(CreatureEntity &creature, MonraceHook hook1 = MonraceHook::NONE, MonraceHookTerrain hook2 = MonraceHookTerrain::NONE);
void get_mon_num_prep_escort(CreatureEntity &creature, MonraceId escorted_monrace_id, short m_idx, MonraceHookTerrain hook2);

class SummonCondition {
public:
    SummonCondition(summon_type type, BIT_FLAGS mode, const tl::optional<short> &summoner_m_idx, MonraceHookTerrain hook)
        : type(type)
        , mode(mode)
        , summoner_m_idx(summoner_m_idx)
        , hook(hook)
    {
    }

    summon_type type;
    BIT_FLAGS mode;
    tl::optional<short> summoner_m_idx;
    MonraceHookTerrain hook;
};

void get_mon_num_prep_summon(CreatureEntity &creature, const SummonCondition &condition);

class ChameleonTransformation {
public:
    /*!
     * @brief カメレオンの変身対象フィルタ
     * @param m_idx カメレオンのフロア内インデックス
     * @param terrain_id カメレオンの足元にある地形のID
     * @param is_unique ユニークであるか否か (実質、カメレオンの王であるか否か)
     * @param summoner_m_idx モンスターの召喚による場合、召喚者のモンスターID
     */
    ChameleonTransformation(short m_idx, short terrain_id, bool is_unique, const tl::optional<short> &summoner_m_idx)
        : m_idx(m_idx)
        , terrain_id(terrain_id)
        , is_unique(is_unique)
        , summoner_m_idx(summoner_m_idx)
    {
    }

    short m_idx;
    short terrain_id;
    bool is_unique;
    tl::optional<short> summoner_m_idx;
};
void get_mon_num_prep_chameleon(CreatureEntity &creature, const ChameleonTransformation &ct);
void get_mon_num_prep_bounty(CreatureEntity &creature);
void mark_monsters_present(CreatureEntity &creature);
bool is_player(MONSTER_IDX m_idx);
bool is_monster(MONSTER_IDX m_idx);
void move_monster_to(CreatureEntity &creature, CreatureEntity &monster, const Pos2D &pos_to);
