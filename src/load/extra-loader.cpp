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
#include "util/enum-converter.h"
#include "world/world.h"

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
}
