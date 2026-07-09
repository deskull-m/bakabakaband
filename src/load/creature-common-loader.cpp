#include "load/creature-common-loader.h"
#include "load/load-util.h"
#include "player-info/class-types.h"
#include "player-info/race-types.h"
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
    creature.set_energy_need(rd_s16b());
    creature.ac = rd_s16b();

    creature.set_exp(rd_u32b());
    creature.set_au(rd_s32b());
    creature.set_ht(rd_s16b());
    creature.set_wt(rd_s16b());

    creature.target.y = rd_s16b();
    creature.target.x = rd_s16b();

    if (loading_savefile_version_is_older_than(53)) {
        // v52 以前: 共通時限効果 7 種のみ個別保存。残りはプレイヤー固有経路が読む。
        creature.set_timed_effect(CreatureTimedEffect::SLEEP_OR_PARALYSIS, rd_s16b());
        creature.set_timed_effect(CreatureTimedEffect::ACCELERATION, rd_s16b());
        creature.set_timed_effect(CreatureTimedEffect::DECELERATION, rd_s16b());
        creature.set_timed_effect(CreatureTimedEffect::STUN, rd_s16b());
        creature.set_timed_effect(CreatureTimedEffect::CONFUSION, rd_s16b());
        creature.set_timed_effect(CreatureTimedEffect::FEAR, rd_s16b());
        creature.set_timed_effect(CreatureTimedEffect::INVULNERABILITY, rd_s16b());
    } else {
        // v53 以降: 全時限効果を列挙順にダンプ (wr_creature_common と対称)。
        for (auto i = 0; i < enum2i(CreatureTimedEffect::MAX); i++) {
            creature.set_timed_effect(i2enum<CreatureTimedEffect>(i), rd_s16b());
        }
    }

    const auto material_count = rd_u16b();
    std::vector<CreatureMaterialType> materials;
    materials.reserve(material_count);
    for (auto i = 0; i < material_count; i++) {
        materials.push_back(i2enum<CreatureMaterialType>(rd_s16b()));
    }
    creature.set_materials(materials);

    // --- セーブデータバージョン 52 拡張 ---
    // v51 以前のセーブには存在しないため読み飛ばす (各経路はこの分岐で自動対応)。
    if (loading_savefile_version_is_older_than(52)) {
        return;
    }
    creature.set_level(rd_s16b());
    creature.set_age(rd_s16b());
    creature.hp_frac = rd_u32b();
    creature.set_max_mp(rd_s32b());
    creature.set_current_mp(rd_s32b());
    creature.current_mp_frac = rd_u32b();
    creature.set_max_exp(rd_u32b());
    creature.set_max_max_exp(rd_u32b());
    creature.exp_frac = rd_u32b();
    for (auto i = 0; i < A_MAX; i++) {
        creature.set_stat_max(i, rd_s16b());
    }
    for (auto i = 0; i < A_MAX; i++) {
        creature.set_stat_max_max(i, rd_s16b());
    }
    for (auto i = 0; i < A_MAX; i++) {
        creature.set_stat_cur(i, rd_s16b());
    }

    // 派生値 stat_use はセーブしない (プレイヤーは calc_bonuses() で再計算する)。
    // モンスターは calc_bonuses() を呼ばないため、生成時の get_stats() と同様に
    // stat_max と同値へ復元する。これによりロード後にモンスターの能力値が 0 へ
    // 落ちる問題を解消する (プレイヤーはこの後の calc_bonuses() で上書きされる)。
    for (auto i = 0; i < A_MAX; i++) {
        creature.set_stat_use(i, creature.get_stat_max(i));
    }

    // --- セーブデータバージョン 54 拡張: 種族・職業 ---
    // v53 以前は prace/pclass をプレイヤー固有経路 (byte) / モンスター固有経路 (s16b)
    // で読むため、ここでは読まない。
    if (loading_savefile_version_is_older_than(54)) {
        return;
    }
    creature.prace = i2enum<PlayerRaceType>(rd_s16b());
    creature.pclass = i2enum<PlayerClassType>(rd_s16b());
}
