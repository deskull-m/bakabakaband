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
    wr_s16b(static_cast<int16_t>(creature.get_energy_need()));
    wr_s16b(static_cast<int16_t>(creature.ac));

    wr_u32b(creature.get_exp());
    wr_s32b(creature.get_au());
    wr_s16b(creature.get_ht());
    wr_s16b(creature.get_wt());

    wr_s16b(static_cast<int16_t>(creature.target.y));
    wr_s16b(static_cast<int16_t>(creature.target.x));

    // プレイヤー・モンスター共通の全時限効果を列挙順にダンプする (v53)。
    // これにより約 56 種の時限効果を 1 箇所で保存し、個別保存に伴う
    // 順序不整合 (旧フォーマットに存在) を解消する。
    for (auto i = 0; i < enum2i(CreatureTimedEffect::MAX); i++) {
        wr_s16b(creature.get_timed_effect(i2enum<CreatureTimedEffect>(i)));
    }

    // 材質 (副種族)
    const auto &materials = creature.get_materials();
    wr_u16b(static_cast<uint16_t>(materials.size()));
    for (const auto material : materials) {
        wr_s16b(static_cast<int16_t>(enum2i(material)));
    }

    // --- セーブデータバージョン 52 拡張: CreatureEntity 数値基底の追加分 ---
    // (level / age / 各種 frac / MP / 経験値補助 / 能力値配列)
    wr_s16b(static_cast<int16_t>(creature.get_level()));
    wr_s16b(creature.get_age());
    wr_u32b(creature.hp_frac);
    wr_s32b(creature.get_max_mp());
    wr_s32b(creature.get_current_mp());
    wr_u32b(creature.current_mp_frac);
    wr_u32b(creature.get_max_exp());
    wr_u32b(creature.get_max_max_exp());
    wr_u32b(creature.exp_frac);
    for (auto i = 0; i < A_MAX; i++) {
        wr_s16b(creature.get_stat_max(i));
    }
    for (auto i = 0; i < A_MAX; i++) {
        wr_s16b(creature.get_stat_max_max(i));
    }
    for (auto i = 0; i < A_MAX; i++) {
        wr_s16b(creature.get_stat_cur(i));
    }

    // --- セーブデータバージョン 54 拡張: 種族・職業 ---
    // prace / pclass は CreatureEntity 基底の共通フィールド (C1 でモンスターにも
    // 付与可能)。従来プレイヤー writer (byte) とモンスター writer (s16b) で個別に
    // 保存していたものを共通ブロックへ集約。NONE (-1) を取り得るため符号付き s16b。
    wr_s16b(static_cast<int16_t>(enum2i(creature.prace)));
    wr_s16b(static_cast<int16_t>(enum2i(creature.pclass)));
}
