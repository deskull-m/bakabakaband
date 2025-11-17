#include "load/alliance-loader.h"
#include "alliance/alliance.h"
#include "load/load-util.h"
#include "util/enum-converter.h"
#include "view/display-messages.h"

/*!
 * @brief アライアンスの基本勢力指数をロードする
 */
void rd_alliance_base_power()
{
    // セーブファイルのバージョンチェック（新しいバージョンからのみ対応）
    if (loading_savefile_version_is_older_than(32)) {
        return;
    }

    // アライアンスの数を読み込む
    const auto alliance_count = rd_u16b();

    // 各アライアンスのbase_powerを読み込む
    for (uint16_t i = 0; i < alliance_count; i++) {
        const auto alliance_type_value = rd_s16b();
        const auto base_power = rd_s64b();

        const auto alliance_type = i2enum<AllianceType>(alliance_type_value);

        // alliance_listからアライアンスを検索して更新
        auto it = alliance_list.find(alliance_type);
        if (it != alliance_list.end()) {
            it->second->base_power = base_power;
        }
    }

    load_note(_("アライアンス情報をロードしました", "Loaded alliance information"));
}
