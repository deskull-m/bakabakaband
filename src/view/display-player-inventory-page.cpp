/*!
 * @brief c コマンド第 6 ページ「装備＆所持品」表示
 * @details
 *   プレイヤーとモンスター両方が inventory[] を持つ前提のため共通描画。
 *   モンスター閲覧時はモンスターの所持装備が確認できる (提案 13 で
 *   モンスターも装備・所持を持つようになったため有用)。
 */
#include "view/display-player-inventory-page.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "inventory/inventory-describer.h"
#include "inventory/inventory-slot-types.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "system/monrace/extended-slot.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "view/display-messages.h"

namespace {

constexpr int HEADER_ROW = 2;
constexpr int EQUIPMENT_COL = 1;
constexpr int INVENTORY_COL = 41;

void put_section_header(int row, int col, std::string_view title)
{
    c_put_str(TERM_L_BLUE, title, row, col);
}

void display_equipment_section(CreatureEntity &creature)
{
    put_section_header(HEADER_ROW, EQUIPMENT_COL, _("== 装備 ==", "== Equipment =="));
    int row = HEADER_ROW + 1;

    // 通常装備スロット: 体構造的に装備可能な部位のみ表示する (該当なしは消去)
    for (auto slot = static_cast<int>(INVEN_MAIN_HAND); slot < INVEN_TOTAL; ++slot) {
        if (!creature.can_equip_to(slot)) {
            continue;
        }
        const auto &item = *creature.inventory[slot];
        const auto label = format("%-12s :", mention_use(creature, slot));
        c_put_str(TERM_WHITE, label, row, EQUIPMENT_COL);
        if (!item.is_valid()) {
            c_put_str(TERM_L_DARK, _("(なし)", "(empty)"), row, EQUIPMENT_COL + 14);
        } else {
            const auto desc = describe_flavor(creature, item, OD_OMIT_PREFIX | OD_NAME_AND_ENCHANT);
            c_put_str(TERM_L_GREEN, desc.substr(0, 24), row, EQUIPMENT_COL + 14);
        }
        ++row;
    }

    // 拡張装備スロット (尾の指輪・翼装飾 等) を通常枠と統合して同じリストに表示
    const auto slot_count = creature.get_extended_slot_count();
    for (size_t i = 0; i < slot_count; ++i) {
        const auto slot_type = creature.get_extended_slot_type(i);
        const auto name = get_extended_slot_name(slot_type);
        const auto label = format("%-12s :", name.empty() ? "" : name.data());
        c_put_str(TERM_WHITE, label, row, EQUIPMENT_COL);
        if (i >= creature.get_extended_inventory_size() || !creature.get_extended_item(i) || !creature.get_extended_item(i)->is_valid()) {
            c_put_str(TERM_L_DARK, _("(なし)", "(empty)"), row, EQUIPMENT_COL + 14);
        } else {
            const auto desc = describe_flavor(creature, *creature.get_extended_item(i), OD_OMIT_PREFIX | OD_NAME_AND_ENCHANT);
            c_put_str(TERM_L_GREEN, desc.substr(0, 24), row, EQUIPMENT_COL + 14);
        }
        ++row;
    }
}

void display_inventory_section(CreatureEntity &creature)
{
    put_section_header(HEADER_ROW, INVENTORY_COL, _("== 所持品 ==", "== Inventory =="));
    int row = HEADER_ROW + 1;
    int item_count = 0;
    for (auto slot = 0; slot < INVEN_PACK; ++slot, ++row) {
        const auto &item = *creature.inventory[slot];
        if (!item.is_valid()) {
            continue;
        }

        const auto letter = static_cast<char>('a' + slot);
        const auto desc = describe_flavor(creature, item, 0);
        const auto line = format("%c) %s", letter, desc.substr(0, 33).data());
        c_put_str(TERM_L_GREEN, line, row, INVENTORY_COL);
        ++item_count;
    }

    if (item_count == 0) {
        c_put_str(TERM_L_DARK, _("(所持品なし)", "(nothing carried)"), HEADER_ROW + 1, INVENTORY_COL);
    }
}

}

void display_player_inventory_page(CreatureEntity &creature)
{
    put_str(_("装備品とインベントリ", "Equipment and Inventory"), 1, 1);
    const auto cnt_inven = creature.get_inven_cnt();
    const auto cnt_equip = creature.get_equip_cnt();
    const auto summary = format(_(" - 所持: %d / 装備: %d", " - Carried: %d / Equipped: %d"), cnt_inven, cnt_equip);
    c_put_str(TERM_SLATE, summary, 1, 25);

    display_equipment_section(creature);
    display_inventory_section(creature);
}
