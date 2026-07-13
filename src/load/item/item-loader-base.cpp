#include "load/item/item-loader-base.h"
#include "artifact/fixed-art-types.h"
#include "load/load-util.h"
#include "system/artifact-type-definition.h"
#include "system/artifact/artifact-record.h"
#include "system/baseitem/baseitem-record.h"
#include "system/baseitem/baseitem-records.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"

/*!
 * @brief アイテムオブジェクトの鑑定情報をロードする.
 */
void ItemLoaderBase::load_item()
{
    auto loading_max_k_idx = rd_u16b();
    BaseitemRecord dummy;
    auto &baseitem_records = BaseitemRecords::get_instance();
    for (uint16_t i = 0; i < loading_max_k_idx; i++) {
        auto &baseitem_record = i < baseitem_records.size() ? baseitem_records.get_record(i) : dummy;
        const auto tmp8u = rd_byte();
        baseitem_record.mark_awareness(any_bits(tmp8u, 0x01));
        baseitem_record.mark_trial(any_bits(tmp8u, 0x02));
    }

    load_note(_("アイテムの記録をロードしました", "Loaded Object Memory"));
}

/*!
 * @brief 固定アーティファクトの出現情報をロードする.
 */
void ItemLoaderBase::load_artifact()
{
    auto loading_max_a_idx = rd_u16b();
    for (auto i = 0U; i < loading_max_a_idx; i++) {
        const auto a_idx = i2enum<FixedArtifactId>(i);
        auto &records = ArtifactRecords::get_instance();
        records.set_generated(a_idx, rd_bool());
        records.set_floor_id(a_idx, rd_s16b());
    }

    load_note(_("伝説のアイテムをロードしました", "Loaded Artifacts"));
}
