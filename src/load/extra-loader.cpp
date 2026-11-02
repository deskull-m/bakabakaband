/*!
 * @brief その他の情報を読み込む処理
 * @date 2020/07/05
 * @author Hourier
 * @todo 「その他」が雑多すぎて肥大化している。今後の課題として分割を検討する
 */

#include "load/extra-loader.h"
#include "load/dummy-loader.h"
#include "load/load-util.h"
#include "load/world-loader.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/town-records.h"
#include "target/target.h"
#include "util/enum-converter.h"
#include "world/world.h"

/*!
 * @brief 最後にターゲットしたものを読み込む
 * @param creature クリーチャーへの参照
 * @details モンスターの番号は、この後で読み込む今の階のモンスターを指す
 */
static void rd_last_target(CreatureEntity &creature)
{
    switch (rd_byte()) {
    case 1: {
        const auto y = rd_s16b();
        const auto x = rd_s16b();
        Target::set_last_target(Target::create_grid_target(creature, { y, x }));
        return;
    }
    case 2:
        Target::set_last_target(Target::create_monster_target(creature, rd_s16b()));
        return;
    default:
        Target::clear_last_target();
        return;
    }
}

/*!
 * @brief その他の情報を読み込む / Read the "extra" information
 * @param creature クリーチャーへの参照
 */
void rd_extra(CreatureEntity &creature)
{
    creature.ride_monster(rd_s16b());
    creature.floor_id = rd_s16b();
    rd_dummy_monsters();

    auto &world = AngbandWorld::get_instance();
    world.play_time = ElapsedTime(rd_u32b());

    // 訪問済みの町情報。旧来の creature.visit ビットマスク (u32) を TownRecords に移行する。
    // セーブフォーマットは互換のため u32 ビットマスクのまま (bit N = TownId N)。
    const auto visit_flags = rd_u32b();
    EnumClassFlagGroup<TownId> visited_towns;
    for (auto i = 0; i < enum2i(TownId::MAX); i++) {
        if ((visit_flags & (1U << i)) != 0) {
            visited_towns.set(i2enum<TownId>(i));
        }
    }

    TownRecords::get_instance().set_ids(visited_towns);
    creature.set_count(rd_u32b());

    // [モンスタープレイヤー] プレイヤーがモンスター化している場合の種族 ID を復元する。
    // セーブファイルバージョン 48 以降で保存される。旧データは PLAYER のまま。
    if (!loading_savefile_version_is_older_than(48)) {
        const auto r_idx = i2enum<MonraceId>(rd_s16b());
        const auto ap_r_idx = i2enum<MonraceId>(rd_s16b());
        creature.set_r_idx(r_idx);
        creature.set_ap_r_idx(ap_r_idx);
    }

    // ペット・乗馬の攻撃対象と、最後にターゲットしたもの。セーブファイルバージョン 58 以降。
    if (!loading_savefile_version_is_older_than(58)) {
        creature.set_pet_t_m_idx(rd_s16b());
        creature.set_riding_t_m_idx(rd_s16b());
        rd_last_target(creature);
    } else {
        creature.set_pet_t_m_idx(0);
        creature.set_riding_t_m_idx(0);
        Target::clear_last_target();
    }
}
