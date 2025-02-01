#include "player-info/magic-eater-data-type.h"
#include "io/command-repeater.h"
#include "sv-definition/sv-rod-types.h"
#include "sv-definition/sv-staff-types.h"
#include "sv-definition/sv-wand-types.h"
#include "system/baseitem-info.h"

magic_eater_data_type::magic_eater_data_type()
    : staves(SV_STAFF_MAX)
    , wands(SV_WAND_MAX)
    , rods(SV_ROD_MAX)
{
}

/*!
 * @brief 指定した種類の取り込んだ魔道具のデータの配列を取得する
 *
 * @param tval 魔道具の種類(TV_STAFF/TV_WAND/TV_ROD)を指定する
 * @return tvalで指定した種類の取り込んだ魔道具のデータ配列の参照
 */
std::vector<magic_eater_data_type::magic_type> &magic_eater_data_type::get_item_group(ItemKindType tval)
{
    switch (tval) {
    case ItemKindType::STAFF:
        return this->staves;
    case ItemKindType::WAND:
        return this->wands;
    case ItemKindType::ROD:
        return this->rods;
    default:
        // ダミーデータ。通常使用されることはない。
        return magic_eater_data_type::none;
    }
}

std::optional<BaseitemKey> magic_eater_data_type::check_magic_eater_spell_repeat() const
{
    short sn;
    if (!repeat_pull(&sn)) {
        return std::nullopt;
    }

    auto tval = ItemKindType::NONE;
    if (EATER_STAFF_BASE <= sn && sn < EATER_STAFF_BASE + EATER_ITEM_GROUP_SIZE) {
        tval = ItemKindType::STAFF;
    } else if (EATER_WAND_BASE <= sn && sn < EATER_WAND_BASE + EATER_ITEM_GROUP_SIZE) {
        tval = ItemKindType::WAND;
    } else if (EATER_ROD_BASE <= sn && sn < EATER_ROD_BASE + EATER_ITEM_GROUP_SIZE) {
        tval = ItemKindType::ROD;
    }

    const auto &item_group = this->get_item_group(tval);
    auto sval = sn % EATER_ITEM_GROUP_SIZE;
    if (sval >= static_cast<int>(item_group.size())) {
        return std::nullopt;
    }

    auto &item = item_group[sval];
    /* Verify the spell */
    switch (tval) {
    case ItemKindType::ROD: {
        const auto &baseitems = BaseitemList::get_instance();
        const auto &baseitem = baseitems.lookup_baseitem({ ItemKindType::ROD, sval });
        if (item.charge <= baseitem.pval * (item.count - 1) * EATER_ROD_CHARGE) {
            return BaseitemKey(tval, sval);
        }

        return std::nullopt;
    }
    case ItemKindType::STAFF:
    case ItemKindType::WAND:
        if (item.charge >= EATER_CHARGE) {
            return BaseitemKey(tval, sval);
        }

        return std::nullopt;
    default:
        return std::nullopt;
    }
}

const std::vector<magic_eater_data_type::magic_type> &magic_eater_data_type::get_item_group(ItemKindType tval) const
{
    switch (tval) {
    case ItemKindType::STAFF:
        return this->staves;
    case ItemKindType::WAND:
        return this->wands;
    case ItemKindType::ROD:
        return this->rods;
    default:
        // ダミーデータ。通常使用されることはない。
        return magic_eater_data_type::none;
    }
}
