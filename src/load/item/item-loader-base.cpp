#include "load/item/item-loader-base.h"
#include "artifact/fixed-art-types.h"
#include "load/load-util.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"

/*!
 * @brief アイテムオブジェクトの鑑定情報をロードする.
 */
void ItemLoaderBase::load_item()
{
    auto loading_max_k_idx = rd_u16b();
    BaseitemInfo dummy;
    auto &baseitems = BaseitemList::get_instance();
    for (uint16_t i = 0; i < loading_max_k_idx; i++) {
        auto &baseitem = i < baseitems.size() ? baseitems.get_baseitem(i) : dummy;
        const auto tmp8u = rd_byte();
        baseitem.aware = any_bits(tmp8u, 0x01);
        baseitem.tried = any_bits(tmp8u, 0x02);
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
        auto &artifact = ArtifactList::get_instance().get_artifact(a_idx);
        artifact.is_generated = rd_bool();
        artifact.floor_id = rd_s16b();
    }

    load_note(_("伝説のアイテムをロードしました", "Loaded Artifacts"));
}
