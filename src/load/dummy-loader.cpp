#include "load/dummy-loader.h"
#include "load/load-util.h"
#include "load/monster/monster-loader-factory.h"
#include "load/old/monster-loader-savefile50.h"
#include "system/floor-type-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"

/*!
 * @brief ダミーバイトを読み込む
 * @param なし
 * @details もはや何に使われていたのか不明
 */
void rd_dummy1(void)
{
    auto tmp16s = rd_s16b();
    strip_bytes(2 * tmp16s);
}

/*!
 * @brief ダミーバイトを読み込む
 * @param なし
 * @details もはや何に使われていたのか不明
 */
void rd_dummy2(void)
{
    strip_bytes(48);
    strip_bytes(12);
}

/*!
 * @brief 馬鹿馬鹿蛮怒 v1.5.0より大きなバージョンにおいて、ダミーでモンスターを読み込む
 * @param player_ptr プレーヤーへの参照ポインタ
 * @details もはや何に使われていたのか不明
 */
void rd_dummy_monsters()
{
    auto tmp16s = rd_s16b();
    MonsterEntity dummy_mon;
    auto monster_loader = MonsterLoaderFactory::create_loader();
    for (int i = 0; i < tmp16s; i++) {
        monster_loader->rd_monster(&dummy_mon);
    }
}

/*!
 * @brief ダミー情報スキップ / Strip the "ghost" info
 * @details
 * This is such a nasty hack it hurts.
 */
void rd_ghost(void)
{
    (void)rd_string();
    strip_bytes(60);
}

void rd_dummy3(void)
{
    strip_bytes(2);
    strip_bytes(1);
}
