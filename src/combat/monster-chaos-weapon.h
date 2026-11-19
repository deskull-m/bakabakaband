#pragma once

#include "system/angband.h"

class CreatureEntity;
class ItemEntity;

/*!
 * @brief カオス武器効果のうち、打撃ループ途中で実行すると危険なため全打撃終了後へ遅延させる処理の種別
 * @details 混乱・テレポート・吸血は即時適用され、これらは NONE を返す。地震と変身は被害者ポインタ等の
 * 無効化を避けるため呼出側が全打撃終了後に実行する。
 */
enum class ChaosWeaponDeferred {
    NONE,
    EARTHQUAKE,
    POLYMORPH_TARGET,
};

ChaosWeaponDeferred apply_monster_weapon_chaos_effect(CreatureEntity &attacker, const ItemEntity &weapon, CreatureEntity &target,
    CreatureEntity &player, MONSTER_IDX attacker_m_idx, MONSTER_IDX target_m_idx, int weapon_damage);
