#include "system/monster-entity.h"
#include "system/enums/monrace/monrace-id.h"

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
}

MonsterEntity MonsterEntity::clone() const
{
    return *this;
}
