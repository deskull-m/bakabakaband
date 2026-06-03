#include "save/creature-common-writer.h"
#include "save/save-util.h"
#include "system/creature-entity.h"
#include "system/material-type-definition.h"
#include "util/enum-converter.h"

/*!
 * @brief CreatureEntity の共通基底フィールドをセーブデータに書き込む
 * @param creature 書き込み対象クリーチャー
 * @details
 * 読み込みは rd_creature_common() と完全対称であること。フィールドの
 * 追加・順序変更を行う場合は両者を同時に修正し、セーブデータバージョンを
 * 更新すること。
 */
void wr_creature_common(const CreatureEntity &creature)
{
    wr_string(creature.name);

    wr_s16b(static_cast<int16_t>(creature.y));
    wr_s16b(static_cast<int16_t>(creature.x));

    wr_s32b(creature.hp);
    wr_s32b(creature.maxhp);
    wr_s32b(creature.max_maxhp);
    wr_u32b(static_cast<uint32_t>(creature.get_dealt_damage()));

    wr_s16b(static_cast<int16_t>(creature.speed));
    wr_s16b(static_cast<int16_t>(creature.energy_need));
    wr_s16b(static_cast<int16_t>(creature.ac));

    wr_u32b(creature.get_exp());
    wr_s32b(creature.get_au());
    wr_s16b(creature.get_ht());
    wr_s16b(creature.get_wt());

    wr_s16b(static_cast<int16_t>(creature.target.y));
    wr_s16b(static_cast<int16_t>(creature.target.x));

    // プレイヤー・モンスター共通の時限効果
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::SLEEP_OR_PARALYSIS));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::ACCELERATION));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::DECELERATION));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::STUN));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::CONFUSION));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::FEAR));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::INVULNERABILITY));

    // 材質 (副種族)
    const auto &materials = creature.get_materials();
    wr_u16b(static_cast<uint16_t>(materials.size()));
    for (const auto material : materials) {
        wr_s16b(static_cast<int16_t>(enum2i(material)));
    }
}
