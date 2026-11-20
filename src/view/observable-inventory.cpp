#include "view/observable-inventory.h"
#include "flavor/flavor-describer.h"
#include "inventory/inventory-describer.h"
#include "inventory/inventory-slot-types.h"
#include "locale/japanese.h"
#include "object/object-info.h"
#include "player/player-status.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "term/gameterm.h"
#include "term/term-color-types.h"
#include "util/enum-converter.h"
#include "view/display-messages.h"
#include "view/display-symbol.h"
#include <algorithm>
#include <fmt/format.h>

namespace {
std::string format_weight(int weight)
{
    return fmt::format(_("{:3}.{:1} kg", "{:3}.{:1} lb"), _(lb_to_kg_integer(weight), weight / 10), _(lb_to_kg_fraction(weight), weight % 10));
}

/*!
 * @brief 端末版の一覧 (display_inventory()) と同じ規則で 1 品を取り出す
 */
ObservedItem observe_item(CreatureEntity &creature, const ItemEntity &item, int slot)
{
    ObservedItem observed;
    observed.slot = slot;
    observed.label = index_to_label(slot);
    observed.name = describe_flavor(creature, item, 0);
    observed.count = item.number;
    observed.weight = item.weight * item.number;
    observed.weight_text = format_weight(observed.weight);
    observed.tval = enum2i(item.bi_key.tval());
    observed.recharging = item.timeout != 0;
    observed.color = observed.recharging ? static_cast<uint8_t>(TERM_L_DARK) : tval_to_attr[observed.tval % 128];
    const auto symbol = item.get_symbol();
    observed.symbol_color = symbol.color;
    observed.symbol_char = symbol.character;
    return observed;
}
}

/*!
 * @brief 持ち物と装備を取り出す
 * @param creature 持ち主 (プレイヤー)
 */
ObservedInventory observe_inventory(CreatureEntity &creature)
{
    ObservedInventory observed;
    for (const auto i_idx : INVEN_PACK_SLOTS) {
        const auto &item = creature.inventory[i_idx];
        if (!item || !item->is_valid()) {
            continue;
        }

        observed.pack.push_back(observe_item(creature, *item, enum2i(i_idx)));
    }

    for (const auto i_idx : INVEN_WIELDING_SLOTS) {
        const auto slot = enum2i(i_idx);
        ObservedEquipmentSlot equipment;
        equipment.slot = slot;
        equipment.label = index_to_label(slot);
        equipment.slot_name = mention_use(creature, slot);
        equipment.can_equip = creature.can_equip_to(slot);
        const auto &item = creature.inventory[i_idx];
        if (item && item->is_valid()) {
            equipment.has_item = true;
            equipment.item = observe_item(creature, *item, slot);
        }

        observed.equipment.push_back(std::move(equipment));
    }

    observed.total_weight = calc_inventory_weight(creature);
    observed.weight_limit = calc_weight_limit(creature);
    observed.total_weight_text = format_weight(observed.total_weight);
    observed.weight_limit_text = format_weight(observed.weight_limit);
    return observed;
}

std::vector<std::string> observe_recent_messages(int count)
{
    std::vector<std::string> messages;
    const auto available = std::clamp(count, 0, static_cast<int>(message_num()));
    messages.reserve(static_cast<size_t>(available));
    for (auto age = available; age-- > 0;) {
        const auto message = message_str(age);
        messages.push_back(message ? *message : std::string());
    }

    return messages;
}
