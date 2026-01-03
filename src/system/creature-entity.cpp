#include "system/creature-entity.h"
#include "inventory/inventory-slot-types.h"
#include "system/item-entity.h"
#include "timed-effect/timed-effects.h"
#include <range/v3/algorithm.hpp>

/*!
 * @brief 指定した固定アーティファクトを装備しているかどうか調べる
 *
 * @param fa_id 固定アーティファクトのID
 * @return 装備していればtrue、そうでなければfalse
 */
bool CreatureEntity::is_wielding(FixedArtifactId fa_id) const
{
    return ranges::any_of(INVEN_WIELDING_SLOTS,
        [&](auto slot) {
            const auto &item = this->inventory[slot];
            return item->is_valid() && item->is_specific_artifact(fa_id);
        });
}

/*!
 * @brief 時限効果管理オブジェクトを取得
 * @return 時限効果管理オブジェクトへの共有ポインタ
 */
std::shared_ptr<TimedEffects> CreatureEntity::effects() const
{
    return this->timed_effects;
}
