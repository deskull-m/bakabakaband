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
#include "monster/monster-status-setter.h"
#include "object-enchant/tr-types.h"
#include "spell-kind/spells-teleport.h"
#include "spell/spells-util.h"
#include "status/bad-status-setter.h"
#include "system/creature-entity.h"
#include "system/creature-timed-effect-types.h"
#include "system/floor/floor-info.h"
#include "system/item-entity.h"
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
 * @return CE_QUAKE を引いた場合は true (呼出側が全打撃終了後の地震を予約する)。それ以外は false
 * @details 確率・効果はプレイヤーの select_chaotic_effect() と一致させる。
 */
bool apply_monster_weapon_chaos_effect(CreatureEntity &attacker, const ItemEntity &weapon, CreatureEntity &target,
    CreatureEntity &player, MONSTER_IDX attacker_m_idx, MONSTER_IDX target_m_idx, int weapon_damage)
{
    if (weapon.get_flags().has_not(TR_CHAOTIC) || one_in_(2)) {
        return false;
    }

    // CE_VAMPIRIC: 吸血 (武器の TR_VAMPIRIC 有無に依らず発火)
    if (randint1(5) < 3) {
        const auto drained = drain_life_to_attacker(attacker, target, weapon_damage);
        if (drained > 0) {
            HealthBarTracker::get_instance().set_flag_if_tracking(attacker_m_idx);
        }

        return false;
    }

    // CE_QUAKE: 地震 (実発生は全打撃終了後に呼出側が行う)
    if (one_in_(250)) {
        return true;
    }

    // CE_CONFUSION: 混乱
    if (!one_in_(10)) {
        inflict_chaos_confusion(attacker, target, target_m_idx);
        return false;
    }

    // CE_TELE_AWAY: テレポートアウェイ / CE_POLYMORPH: 変身 (保留)
    if (one_in_(2)) {
        inflict_chaos_teleport(target, player, attacker_m_idx, target_m_idx);
    }

    return false;
}
