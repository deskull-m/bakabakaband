/*!
 * @file damage-dispatcher.cpp
 * @brief プレイヤー・モンスター両対応のダメージ処理統一エントリポイント実装（Phase 4）
 */

#include "combat/damage-dispatcher.h"
#include "combat/damage-dispatcher-internal.h"
#include "monster/monster-damage.h"
#include "system/creature-entity.h"

int apply_damage_to_creature(CreatureEntity &victim, int damage, const DamageContext &ctx)
{
    if (victim.is_player()) {
        // プレイヤー経路は無敵・多重影分身・幽体化等の mitigation 後の
        // 実適用ダメージ量を返す。
        return apply_damage_to_player_impl(victim, ctx.damage_type, damage, ctx.cause, ctx.killer_monrace_id);
    }

    // モンスター経路は加害者を必須とする（MonsterDamageProcessor の第1引数が加害者のため）
    // 環境ダメージによるモンスター殺害は現状 MonsterDamageProcessor を直接呼べないため未対応。
    if (ctx.attacker == nullptr) {
        return 0;
    }

    // モンスター経路でも「適用したダメージ量」を返すことでプレイヤー経路と
    // 戻り値セマンティクスを統一する。mon_take_hit() は呼び出し時点で
    // 既に耐性・防御計算が済んだ damage を受け取る設計のため、そのまま
    // damage を返してよい（生死判定が必要な場合は victim.is_dead() を参照）。
    MonsterDamageProcessor mdp(*ctx.attacker, ctx.victim_m_idx, damage, ctx.fear, ctx.attribute_flags);
    mdp.mon_take_hit(ctx.cause);
    return damage;
}

int apply_damage_to_creature(CreatureEntity &victim, int damage_type, int damage, std::string_view cause, MonraceId killer_monrace_id)
{
    DamageContext ctx;
    ctx.cause = cause;
    ctx.damage_type = damage_type;
    ctx.killer_monrace_id = killer_monrace_id;
    return apply_damage_to_creature(victim, damage, ctx);
}
