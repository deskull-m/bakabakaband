#include "load/creature-common-loader.h"
#include "load/load-util.h"
#include "system/creature-entity.h"
#include "system/material-type-definition.h"
#include "util/enum-converter.h"
#include <vector>

/*!
 * @brief CreatureEntity の共通基底フィールドをセーブデータから読み込む
 * @param creature 復元対象クリーチャー
 * @details wr_creature_common() と完全対称であること。
 */
void rd_creature_common(CreatureEntity &creature)
{
    creature.name = rd_string();

    creature.y = rd_s16b();
    creature.x = rd_s16b();

    creature.hp = rd_s32b();
    creature.maxhp = rd_s32b();
    creature.max_maxhp = rd_s32b();
    creature.set_dealt_damage(static_cast<int>(rd_u32b()));

    creature.speed = rd_s16b();
    creature.energy_need = rd_s16b();
    creature.ac = rd_s16b();

    creature.set_exp(rd_u32b());
    creature.set_au(rd_s32b());
    creature.set_ht(rd_s16b());
    creature.set_wt(rd_s16b());

    creature.target.y = rd_s16b();
    creature.target.x = rd_s16b();

    creature.set_timed_effect(CreatureTimedEffect::SLEEP_OR_PARALYSIS, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::ACCELERATION, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::DECELERATION, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::STUN, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::CONFUSION, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::FEAR, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::INVULNERABILITY, rd_s16b());

    const auto material_count = rd_u16b();
    std::vector<CreatureMaterialType> materials;
    materials.reserve(material_count);
    for (auto i = 0; i < material_count; i++) {
        materials.push_back(i2enum<CreatureMaterialType>(rd_s16b()));
    }
    creature.set_materials(materials);
}
