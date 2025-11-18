#include "save/alliance-writer.h"
#include "alliance/alliance.h"
#include "save/save-util.h"
#include "util/enum-converter.h"

/*!
 * @brief アライアンスの基本勢力指数をセーブデータに書き込む
 */
void wr_alliance_base_power()
{
    // アライアンスの数を書き込む
    const auto alliance_count = static_cast<uint16_t>(alliance_list.size());
    wr_u16b(alliance_count);

    // 各アライアンスのIDとbase_power、natural_recoveryを書き込む
    for (const auto &[alliance_type, alliance_ptr] : alliance_list) {
        wr_s16b(enum2i(alliance_type));
        wr_s64b(alliance_ptr->base_power);
        wr_s64b(alliance_ptr->natural_recovery);
    }
}
