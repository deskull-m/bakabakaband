#include "system/monster-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"

MonsterEntity::MonsterEntity()
{
    this->monster_profile.emplace();
    for (const auto mte : MONSTER_TIMED_EFFECT_RANGE) {
        this->get_monster_profile().mtimed[mte] = 0;
    }

    // CreatureEntityの基本メンバーを初期化
    this->r_idx = MonraceId::PLAYER; // デフォルトはプレイヤー（無効な状態）
    this->ap_r_idx = MonraceId::PLAYER;
    this->patron = 0; // パトロンなし
}

void MonsterEntity::wipe()
{
    *this = {};
    this->monster_profile.emplace();
    for (const auto mte : MONSTER_TIMED_EFFECT_RANGE) {
        this->get_monster_profile().mtimed[mte] = 0;
    }
}

MonsterEntity MonsterEntity::clone() const
{
    return *this;
}

bool MonsterEntity::can_ring_boss_call_nazgul() const
{
    auto is_boss = this->r_idx == MonraceId::MORGOTH;
    is_boss |= this->r_idx == MonraceId::SAURON;
    is_boss |= this->r_idx == MonraceId::ANGMAR;
    const auto &nazgul = MonraceList::get_instance().get_monrace(MonraceId::NAZGUL);
    const auto is_nazgul_alive = (nazgul.cur_num + 2) < nazgul.max_num;
    return is_boss && is_nazgul_alive;
}

