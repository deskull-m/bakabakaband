/*!
 * @file damage-dispatcher.cpp
 * @brief プレイヤー・モンスター両対応のダメージ処理統一エントリポイント実装（Phase 4）
 */

#include "combat/damage-dispatcher.h"
#include "monster/monster-damage.h"
#include "player/player-damage.h"
#include "system/creature-entity.h"

int apply_damage_to_creature(CreatureEntity &victim, int damage, const DamageContext &ctx)
{
    if (victim.is_player()) {
        return take_hit(victim, ctx.damage_type, damage, ctx.cause, ctx.killer_monrace_id);
    }

    // モンスター経路は加害者を必須とする（MonsterDamageProcessor の第1引数が加害者のため）
    // 環境ダメージによるモンスター殺害は現状 MonsterDamageProcessor を直接呼べないため未対応。
    if (ctx.attacker == nullptr) {
        return 0;
    }

    MonsterDamageProcessor mdp(*ctx.attacker, ctx.victim_m_idx, damage, ctx.fear, ctx.attribute_flags);
    return mdp.mon_take_hit(ctx.cause) ? 1 : 0;
}
