#include "system/monster-entity.h"
#include "alliance/alliance.h"
#include "core/speed-table.h"
#include "monster-race/race-kind-flags.h"
#include "monster/monster-status.h"
#include "system/angband-system.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include <algorithm>

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

/*!
 * @brief モンスターの個体加速を設定する / Get initial monster speed
 * @param force_fixed_speed 速度を固定にする(個体差を適用しない)か否か
 */
void MonsterEntity::set_individual_speed(bool force_fixed_speed)
{
    const auto &monrace = this->get_monrace();
    auto speed = monrace.speed;
    if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE) && !force_fixed_speed) {
        /* Allow some small variation per monster */
        int i = speed_to_energy(monrace.speed) / (one_in_(4) ? 3 : 10);
        if (i) {
            speed += static_cast<uint8_t>(rand_spread(0, i));
        }
    }

    if (speed > STANDARD_SPEED + 99) {
        speed = STANDARD_SPEED + 99;
    }

    this->speed = speed;
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

int MonsterEntity::get_level() const
{
    // 個体レベルが設定されていればそれを使用、未設定なら種族レベルを使用
    if (this->level > 0) {
        return this->level;
    }
    return this->get_monrace().level / 2;
}
