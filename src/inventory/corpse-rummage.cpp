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
 * @brief 死体そのものを持ち去る候補を表す番兵 (所持品のスロット番号と重ならない値)
 */
constexpr auto TAKE_WHOLE_CORPSE = -1;

/*!
 * @brief 死体そのもの (死体・骨のアイテム) を持ち去ってプレイヤーの所持品へ移す
 * @return 持ち去ったら true (死体はフロアから無くなる)
 * @details **持ち物が残っている死体は持ち去れない**。中身ごと持ち去れると
 *          床の散乱を減らすために死体の中へ所持品を納めた意味が無くなり、
 *          持ち去った死体アイテムは種族しか覚えないので中身も失われるため。
 */
bool take_whole_corpse(CreatureEntity &creature, short corpse_idx)
{
    auto &corpse = creature.get_floor()->get_corpse(corpse_idx);
    if (corpse.has_belongings()) {
        msg_print(_("{}にはまだ持ち物が残っている。先に取り出さないと持ち去れない。", "{} still holds belongings. Take them out first."), corpse.describe());
        return false;
    }

    const auto remains = corpse.make_remains_item();
    const auto item_name = describe_flavor(creature, remains, 0);
    if (!creature.can_store_item(remains)) {
        msg_print(_("ザックには{}を入れる隙間がない。", "You have no room for {}."), item_name);
        return false;
    }

    (void)creature.store_item(remains);
    remove_corpse(creature, corpse_idx, false);
    msg_print(_("{}を持ち去った。", "You take {}."), item_name);
    return true;
}

/*!
 * @brief 1 つの死体を漁る (中身が尽きるか ESC まで繰り返す)
 * @details 中身の一覧の末尾に「死体そのものを持ち去る」を並べる。中身が尽きた後は
 *          持ち去るかどうかだけを尋ねる。
 */
void rummage_one_corpse(CreatureEntity &creature, short corpse_idx)
{
    auto &floor = *creature.get_floor();

    // 死者の持ち物を剥ぐ行為そのものを悪行として記録する (market/loot-notoriety.h の
    // 系統 5)。品を取り出せたかに依らず、死体を開けた時点で 1 回だけ記録する。
    record_rummaging_corpse(creature, floor.get_corpse(corpse_idx).body.get_r_idx());

    while (true) {
        auto &corpse = floor.get_corpse(corpse_idx);
        auto slots = collect_item_slots(corpse);
        if (slots.empty()) {
            const auto prompt = fmt::format(_("{}には何も残っていない。{}そのものを持ち去りますか？", "There is nothing left in {}. Take {} itself? "),
                corpse.describe(), corpse.describe());
            if (input_check(prompt)) {
                (void)take_whole_corpse(creature, corpse_idx);
            }

            return;
        }

        slots.push_back(TAKE_WHOLE_CORPSE);
        const auto prompt = fmt::format(_("{}から何を取りますか？ ", "Take what from {}? "), corpse.describe());
        CandidateSelector selector(prompt);
        const auto it = selector.select(slots, [&creature, &corpse](int slot) {
            if (slot == TAKE_WHOLE_CORPSE) {
                return fmt::format(_("({}そのものを持ち去る)", "(take {} itself)"), corpse.describe());
            }

            return describe_flavor(creature, *corpse.body.inventory[slot], 0);
        });
        if (it == slots.end()) {
            return;
        }

        if (*it == TAKE_WHOLE_CORPSE) {
            if (take_whole_corpse(creature, corpse_idx)) {
                return;
            }

            continue;
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
