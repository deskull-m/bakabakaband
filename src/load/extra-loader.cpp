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
#include "system/player-type-definition.h"
#include "world/world.h"

/*!
 * @brief その他の情報を読み込む / Read the "extra" information
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void rd_extra(PlayerType *player_ptr)
{
    player_ptr->ride_monster(rd_s16b());
    player_ptr->floor_id = rd_s16b();
    rd_dummy_monsters();

    auto &world = AngbandWorld::get_instance();
    world.play_time = ElapsedTime(rd_u32b());

    player_ptr->visit = rd_u32b();
    player_ptr->count = rd_u32b();
}
