/*!
 * @brief モンスターが装備したカオス武器 (TR_CHAOTIC) の追加効果処理
 * @details
 * プレイヤーの select_chaotic_effect() / change_monster_stat() と同じ確率・効果で、
 * カオス武器を装備したモンスターの近接打撃に吸血・地震・混乱・テレポートの追加効果を与える。
 * 対プレイヤー・対モンスターのいずれの標的でも、それぞれに適した既存プリミティブへ振り分ける。
 * 変身 (CE_POLYMORPH) は、打撃ループ途中で対象モンスターを delete/再生成し得て被害者ポインタを
 * 無効化する危険があるため本段では保留する (何も起こさない)。
 */

#include "combat/monster-chaos-weapon.h"
#include "combat/slaying.h"
#include "monster-race/race-misc-flags.h"
#include "monster-race/race-resistance-mask.h"
#include "monster/monster-status-setter.h"
#include "object-enchant/tr-types.h"
#include "spell-kind/spells-teleport.h"
#include "spell/spells-util.h"
#include "status/bad-status-setter.h"
#include "system/creature-entity.h"
#include "system/creature-timed-effect-types.h"
#include "system/floor/floor-info.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "term/z-rand.h"
#include "tracking/health-bar-tracker.h"

namespace {
constexpr auto CHAOS_TELEPORT_DISTANCE = 50;

/*!
 * @brief カオス武器による混乱を対象へ与える (プレイヤー・モンスター両対応)
 */
void inflict_chaos_confusion(CreatureEntity &attacker, CreatureEntity &target, MONSTER_IDX target_m_idx)
{
    const auto duration = 10 + randint0(attacker.get_level()) / 5;
    if (target.is_player()) {
        BadStatusSetter(target).mod_confusion(static_cast<TIME_EFFECT>(duration));
        return;
    }

    const auto current = target.get_timed_effect(CreatureTimedEffect::CONFUSION);
    set_monster_confused(*attacker.get_floor(), target_m_idx, current + duration);
}

/*!
 * @brief カオス武器によるテレポートアウェイを対象へ与える (プレイヤー・モンスター両対応)
 */
void inflict_chaos_teleport(CreatureEntity &target, CreatureEntity &player, MONSTER_IDX attacker_m_idx, MONSTER_IDX target_m_idx)
{
    if (target.is_player()) {
        teleport_player_away(attacker_m_idx, target, CHAOS_TELEPORT_DISTANCE, false);
        return;
    }

    teleport_away(player, target_m_idx, CHAOS_TELEPORT_DISTANCE, TELEPORT_PASSIVE);
}

/*!
 * @brief カオス武器による変身の対象になり得るか (プレイヤーの attack_polymorph と同じ除外条件)
 * @details UNIQUE 扱い (プレイヤー・UNIQUE モンスター) / クエスト対象 / カオス耐性は変身しない。
 * さらに change_monster_stat と同じ `randint1(90) > level` の抽選を課す。
 */
bool can_chaos_polymorph_target(const CreatureEntity &target)
{
    if (target.is_unique()) {
        return false;
    }

    const auto &monrace = target.get_monrace();
    if (monrace.misc_flags.has(MonsterMiscType::QUESTOR) || monrace.resistance_flags.has_any_of(RFR_EFF_RESIST_CHAOS_MASK)) {
        return false;
    }

    return randint1(90) > monrace.level;
}
}

/*!
 * @brief モンスターの装備カオス武器による近接打撃の追加効果を判定・適用する
 * @param attacker 攻撃側モンスター
 * @param weapon 使用した近接武器
 * @param target 攻撃対象クリーチャー (プレイヤー・モンスターいずれも可)
 * @param player プレイヤーへの参照 (フロア・行為者コンテキスト用)
 * @param attacker_m_idx 攻撃側モンスターのインデックス
 * @param target_m_idx 攻撃対象モンスターのインデックス (プレイヤーが標的なら 0)
 * @param weapon_damage calc_weapon_melee_damage() が返した当該打撃の武器ダメージ (吸血量算出用)
 * @return 全打撃終了後に遅延実行すべき効果 (地震 / 変身)。即時効果のみなら NONE
 * @details 確率・効果はプレイヤーの select_chaotic_effect() と一致させる。地震・変身は
 * 打撃ループ途中の実行が危険なため遅延させ、呼出側が全打撃終了後に実行する。
 */
ChaosWeaponDeferred apply_monster_weapon_chaos_effect(CreatureEntity &attacker, const ItemEntity &weapon, CreatureEntity &target,
    CreatureEntity &player, MONSTER_IDX attacker_m_idx, MONSTER_IDX target_m_idx, int weapon_damage)
{
    if (weapon.get_flags().has_not(TR_CHAOTIC) || one_in_(2)) {
        return ChaosWeaponDeferred::NONE;
    }

    // CE_VAMPIRIC: 吸血 (武器の TR_VAMPIRIC 有無に依らず発火)
    if (randint1(5) < 3) {
        const auto drained = drain_life_to_attacker(attacker, target, weapon_damage);
        if (drained > 0) {
            HealthBarTracker::get_instance().set_flag_if_tracking(attacker_m_idx);
        }

        return ChaosWeaponDeferred::NONE;
    }

    // CE_QUAKE: 地震 (実発生は全打撃終了後に呼出側が行う)
    if (one_in_(250)) {
        return ChaosWeaponDeferred::EARTHQUAKE;
    }

    // CE_CONFUSION: 混乱
    if (!one_in_(10)) {
        inflict_chaos_confusion(attacker, target, target_m_idx);
        return ChaosWeaponDeferred::NONE;
    }

    // CE_TELE_AWAY: テレポートアウェイ (即時) / CE_POLYMORPH: 変身 (遅延)
    if (one_in_(2)) {
        inflict_chaos_teleport(target, player, attacker_m_idx, target_m_idx);
        return ChaosWeaponDeferred::NONE;
    }

    // CE_POLYMORPH: 変身。polymorph_monster は対象を delete/再生成して被害者ポインタを無効化するため、
    // 実行は全打撃終了後に呼出側へ委ねる。プレイヤーは is_unique により対象外。
    if (can_chaos_polymorph_target(target)) {
        return ChaosWeaponDeferred::POLYMORPH_TARGET;
    }

    return ChaosWeaponDeferred::NONE;
}
