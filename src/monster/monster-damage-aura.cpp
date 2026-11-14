/*!
 * @brief モンスターが周囲へ継続的にダメージを与えるオーラの処理 (Issue #1676)
 * @details MtG「苦花」由来。monrace の `aura_damage` 指定を持つ個体が、自身を
 * 中心とした半径内のプレイヤー・モンスターへ一定周期で属性ダメージを与え続ける。
 * 指定の無い種族 (実データの大多数) では何も起きないため既定バランスは不変。
 */

#include "monster/monster-damage-aura.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/gamevalue.h"
#include "system/monrace/monrace-definition.h"
#include "world/world.h"
#include <vector>

namespace {
/*!
 * @brief 今回の tick がオーラの発動タイミングかどうか
 * @param interval 発動周期 (TURNS_PER_TICK 単位)
 * @details セーブデータへ状態を持たせないよう、ゲームターンから算出する。
 * 本関数の呼出元 process_world() 自体が TURNS_PER_TICK 周期でゲートされている。
 */
bool is_aura_tick(int interval)
{
    if (interval <= 1) {
        return true;
    }

    const auto tick = AngbandWorld::get_instance().game_turn / TURNS_PER_TICK;
    return (tick % interval) == 0;
}
}

/*!
 * @brief 周囲継続ダメージのオーラを持つ全モンスターのオーラを発動させる
 * @param creature プレイヤーへの参照
 */
void process_monster_damage_auras(CreatureEntity &creature)
{
    auto &floor = *creature.get_floor();

    // オーラは他個体を殺し得るので m_list が書き換わる。対象を先に確定させてから撃つ。
    std::vector<MONSTER_IDX> aura_monsters;
    for (MONSTER_IDX m_idx = 1; m_idx < floor.m_max; m_idx++) {
        const auto &monster = floor.m_list[m_idx];
        if (!monster.is_valid()) {
            continue;
        }

        const auto &aura = monster.get_monrace().aura_damage;
        if (!aura || !is_aura_tick(aura->interval)) {
            continue;
        }

        aura_monsters.push_back(m_idx);
    }

    for (const auto m_idx : aura_monsters) {
        if (creature.is_dead()) {
            return;
        }

        auto &monster = floor.m_list[m_idx];
        if (!monster.is_valid()) {
            continue; // 先に撃った別個体のオーラで死亡した
        }

        const auto &aura = monster.get_monrace().aura_damage;
        if (!aura) {
            continue;
        }

        // PROJECT_GRID / PROJECT_ITEM は意図的に立てない。毎 tick 地形を変えたり
        // 床のアイテムを壊し続けるのは過剰なため、効果はクリーチャーのみに限る。
        // 自身はボールの中心だが is_affective() が発生源を除外するので当たらない。
        constexpr BIT_FLAGS flags = PROJECT_KILL | PROJECT_JUMP;
        (void)project(creature, m_idx, aura->radius, monster.y, monster.x, aura->damage_dice.roll(), aura->attribute, flags);
    }
}
