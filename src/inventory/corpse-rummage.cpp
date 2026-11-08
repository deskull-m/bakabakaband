#include "inventory/corpse-rummage.h"
#include "core/asking-player.h"
#include "core/disturbance.h"
#include "dungeon/quest.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "inventory/inventory-slot-types.h"
#include "locale/language-switcher.h"
#include "market/loot-notoriety.h"
#include "monster-floor/monster-corpse-leaver.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/floor/monster-corpse.h"
#include "system/item-entity.h"
#include "util/candidate-selector.h"
#include "view/display-messages.h"
#include <fmt/format.h>
#include <vector>

namespace {

/*!
 * @brief 死体の中の有効なアイテムが入っているスロットを集める
 * @details 装備枠 (INVEN_MAIN_HAND 以降) も対象にする。生前に装備していた品を
 *          剥ぎ取る操作であるため、パックと装備の区別はしない。
 */
std::vector<int> collect_item_slots(const MonsterCorpse &corpse)
{
    std::vector<int> slots;
    for (auto slot = 0; slot < INVEN_TOTAL; slot++) {
        const auto &item_ptr = corpse.body.inventory[slot];
        if (item_ptr && item_ptr->is_valid()) {
            slots.push_back(slot);
        }
    }

    return slots;
}

/*!
 * @brief 死体から 1 品を取り出してプレイヤーの所持品へ移す
 * @return 取り出せたらtrue
 */
bool take_item_from_corpse(CreatureEntity &creature, MonsterCorpse &corpse, int slot_in_corpse)
{
    auto &held = *corpse.body.inventory[slot_in_corpse];
    const auto taken = held.clone();
    const auto item_name = describe_flavor(creature, taken, 0);
    if (!creature.can_store_item(taken)) {
        msg_print(_("ザックには{}を入れる隙間がない。", "You have no room for {}."), item_name);
        return false;
    }

    const auto slot = creature.store_item(taken);
    held.wipe();
    // 「死体」が二重に出るのを避けるため品名だけを述べる
    // (品そのものが「〜の死体」になりうる)
    msg_print(_("{}を取り出した。", "You take {}."), item_name);

    // 他者の正当な所有物を手に入れたことを記録し、社会的問題を発生させる。
    // 床を経由しなくなったぶん process_player_pickup_item() が通らないので、
    // ここで同じ記録を行う (market/loot-notoriety.h 参照)。
    record_looted_rightful_property(creature, taken, taken.number);
    record_rummaged_corpse_item(creature, taken.number);
    check_find_art_quest_completion(creature, creature.inventory[slot].get());
    return true;
}

/*!
 * @brief 1 つの死体を漁る (中身が尽きるか ESC まで繰り返す)
 */
void rummage_one_corpse(CreatureEntity &creature, short corpse_idx)
{
    auto &floor = *creature.get_floor();

    // 死者の持ち物を剥ぐ行為そのものを悪行として記録する (market/loot-notoriety.h の
    // 系統 5)。品を取り出せたかに依らず、死体を開けた時点で 1 回だけ記録する。
    record_rummaging_corpse(creature, floor.get_corpse(corpse_idx).body.get_r_idx());

    while (true) {
        auto &corpse = floor.get_corpse(corpse_idx);
        const auto slots = collect_item_slots(corpse);
        if (slots.empty()) {
            msg_print(_("{}には何も残っていない。", "There is nothing left in {}."), corpse.describe());
            return;
        }

        const auto prompt = fmt::format(_("{}から何を取りますか？ ", "Take what from {}? "), corpse.describe());
        CandidateSelector selector(prompt);
        const auto it = selector.select(slots, [&creature, &corpse](int slot) {
            return describe_flavor(creature, *corpse.body.inventory[slot], 0);
        });
        if (it == slots.end()) {
            return;
        }

        if (!take_item_from_corpse(creature, corpse, *it)) {
            return;
        }
    }
}
}

/*!
 * @brief 足元に遺された死体を漁る
 * @param pickup 拾う操作なら true。false のときは死体があることだけを伝える
 * @details 床のアイテムを拾う処理 (carry()) から呼ばれる。死体が複数積まれて
 *          いる場合はまずどの死体を漁るかを選ぶ。
 */
void rummage_corpses_here(CreatureEntity &creature, bool pickup)
{
    auto &floor = *creature.get_floor();
    const auto p_pos = creature.get_position();
    const auto corpse_indices = collect_corpses_at(floor, p_pos);
    if (corpse_indices.empty()) {
        return;
    }

    if (!pickup) {
        for (const auto corpse_idx : corpse_indices) {
            msg_print(_("{}がある。", "You see {}."), floor.get_corpse(corpse_idx).describe());
        }

        return;
    }

    disturb(creature, false, false);
    if (corpse_indices.size() == 1) {
        const auto corpse_idx = corpse_indices.front();
        const auto prompt = fmt::format(_("{}を漁りますか？", "Search {}? "), floor.get_corpse(corpse_idx).describe());
        if (!input_check(prompt)) {
            return;
        }

        rummage_one_corpse(creature, corpse_idx);
        return;
    }

    CandidateSelector selector(_("どの死体を漁りますか？ ", "Search which corpse? "));
    const auto it = selector.select(corpse_indices, [&floor](short corpse_idx) {
        return floor.get_corpse(corpse_idx).describe();
    });
    if (it == corpse_indices.end()) {
        return;
    }

    rummage_one_corpse(creature, *it);
}
