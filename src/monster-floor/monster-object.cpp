/*!
 * @brief モンスターが移動した結果、床のアイテムに重なった時の処理と、モンスターがアイテムを落とす処理
 * @date 2020/03/07
 * @author Hourier
 */

#include "monster-floor/monster-object.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "floor/floor-object.h"
#include "floor/geometry.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-resistance-mask.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "monster/monster-processor-util.h"
#include "monster/smart-learn-types.h"
#include "object-enchant/tr-types.h"
#include "object/object-info.h"
#include "object/object-mark-types.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/redrawing-flags-updater.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include <string_view>

/*!
 * @brief オブジェクトのフラグを更新する
 */
static void update_object_flags(const TrFlags &flags, EnumClassFlagGroup<MonsterKindType> &flg_monster_kind, EnumClassFlagGroup<MonsterResistanceType> &flgr)
{
    if (flags.has(TR_SLAY_DRAGON)) {
        flg_monster_kind.set(MonsterKindType::DRAGON);
    }
    if (flags.has(TR_KILL_DRAGON)) {
        flg_monster_kind.set(MonsterKindType::DRAGON);
    }
    if (flags.has(TR_SLAY_TROLL)) {
        flg_monster_kind.set(MonsterKindType::TROLL);
    }
    if (flags.has(TR_KILL_TROLL)) {
        flg_monster_kind.set(MonsterKindType::TROLL);
    }
    if (flags.has(TR_SLAY_GIANT)) {
        flg_monster_kind.set(MonsterKindType::GIANT);
    }
    if (flags.has(TR_KILL_GIANT)) {
        flg_monster_kind.set(MonsterKindType::GIANT);
    }
    if (flags.has(TR_SLAY_ORC)) {
        flg_monster_kind.set(MonsterKindType::ORC);
    }
    if (flags.has(TR_KILL_ORC)) {
        flg_monster_kind.set(MonsterKindType::ORC);
    }
    if (flags.has(TR_SLAY_DEMON)) {
        flg_monster_kind.set(MonsterKindType::DEMON);
    }
    if (flags.has(TR_KILL_DEMON)) {
        flg_monster_kind.set(MonsterKindType::DEMON);
    }
    if (flags.has(TR_SLAY_UNDEAD)) {
        flg_monster_kind.set(MonsterKindType::UNDEAD);
    }
    if (flags.has(TR_KILL_UNDEAD)) {
        flg_monster_kind.set(MonsterKindType::UNDEAD);
    }
    if (flags.has(TR_SLAY_ANIMAL)) {
        flg_monster_kind.set(MonsterKindType::ANIMAL);
        flg_monster_kind.set(MonsterKindType::WEREWOLF);
    }
    if (flags.has(TR_KILL_ANIMAL)) {
        flg_monster_kind.set(MonsterKindType::ANIMAL);
        flg_monster_kind.set(MonsterKindType::WEREWOLF);
    }
    if (flags.has(TR_SLAY_EVIL)) {
        flg_monster_kind.set(MonsterKindType::EVIL);
    }
    if (flags.has(TR_KILL_EVIL)) {
        flg_monster_kind.set(MonsterKindType::EVIL);
    }
    if (flags.has(TR_SLAY_GOOD)) {
        flg_monster_kind.set(MonsterKindType::GOOD);
    }
    if (flags.has(TR_KILL_GOOD)) {
        flg_monster_kind.set(MonsterKindType::GOOD);
    }
    if (flags.has(TR_SLAY_HUMAN)) {
        flg_monster_kind.set(MonsterKindType::HUMAN);
    }
    if (flags.has(TR_KILL_HUMAN)) {
        flg_monster_kind.set(MonsterKindType::HUMAN);
    }
    if (flags.has(TR_BRAND_ACID)) {
        flgr.set(MonsterResistanceType::IMMUNE_ACID);
    }
    if (flags.has(TR_BRAND_ELEC)) {
        flgr.set(MonsterResistanceType::IMMUNE_ELEC);
    }
    if (flags.has(TR_BRAND_FIRE)) {
        flgr.set(MonsterResistanceType::IMMUNE_FIRE);
    }
    if (flags.has(TR_BRAND_COLD)) {
        flgr.set(MonsterResistanceType::IMMUNE_COLD);
    }
    if (flags.has(TR_BRAND_POIS)) {
        flgr.set(MonsterResistanceType::IMMUNE_POISON);
    }
}

/*!
 * @brief モンスターがアイテムを拾うか壊す処理
 * @param creature クリーチャーへの参照
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_idx モンスターID
 * @param o_ptr オブジェクトへの参照ポインタ
 * @param is_unpickable_object モンスターが拾えないアイテム (アーティファクト等)であればTRUE
 * @param ny 移動後の、モンスターのY座標
 * @param nx 移動後の、モンスターのX座標
 * @param m_name モンスター名
 * @param o_name アイテム名
 * @param this_o_idx モンスターが乗ったオブジェクトID
 */
static void monster_pickup_object(CreatureEntity &creature, turn_flags *turn_flags_ptr, const MONSTER_IDX m_idx, ItemEntity *o_ptr, const bool is_unpickable_object,
    const POSITION ny, const POSITION nx, std::string_view m_name, std::string_view o_name, const OBJECT_IDX this_o_idx)
{
    auto &floor = *creature.get_floor();
    auto &monster = floor.get_monster(m_idx);
    const auto &monrace = monster.get_monrace();
    if (is_unpickable_object) {
        if (turn_flags_ptr->do_take && monrace.behavior_flags.has(MonsterBehaviorType::STUPID)) {
            turn_flags_ptr->did_take_item = true;
            if (monster.is_visible_on_map() && player_can_see_bold(creature, ny, nx)) {
                msg_format(_("%s^は%sを拾おうとしたが、だめだった。", "%s^ tries to pick up %s, but fails."), m_name.data(), o_name.data());
            }
        }

        return;
    }

    if (turn_flags_ptr->do_take) {
        turn_flags_ptr->did_take_item = true;
        if (player_can_see_bold(creature, ny, nx)) {
            msg_format(_("%s^が%sを拾った。", "%s^ picks up %s."), m_name.data(), o_name.data());
        }

        // [フェーズ A-5] inventory[] に格納し、floor.o_list 側のエントリは
        // delete_object_idx で破棄する (旧 hold_o_idx_list / held_m_idx 経路は廃止)
        auto picked = o_ptr->clone();
        picked.marked.clear().set(OmType::TOUCHED);
        picked.iy = picked.ix = 0;
        picked.held_m_idx = 0;
        delete_object_idx(creature, this_o_idx);

        // [フェーズ B-1] 装備可能なアイテムは空きスロットがあれば自動装備
        // (1 個のみ; スタックは pack に格納)
        const auto eq_slot = wield_slot(monster, picked);
        const bool can_auto_equip = (eq_slot >= INVEN_MAIN_HAND) && (eq_slot < INVEN_TOTAL) && !monster.inventory[eq_slot]->is_valid() && (picked.number == 1);
        if (can_auto_equip) {
            *monster.inventory[eq_slot] = picked;
            monster.equip_cnt++;
            if (player_can_see_bold(creature, ny, nx)) {
                const auto eq_name = describe_flavor(creature, *monster.inventory[eq_slot], 0);
                msg_format(_("%s^は%sを装備した。", "%s^ wields %s."), m_name.data(), eq_name.data());
            }
        } else {
            (void)monster.store_item(picked);
        }

        RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::FOUND_ITEMS);
        return;
    }

    if (monster.is_pet()) {
        return;
    }

    turn_flags_ptr->did_kill_item = true;
    if (floor.has_los_at({ ny, nx })) {
        msg_format(_("%s^が%sを破壊した。", "%s^ destroys %s."), m_name.data(), o_name.data());
    }

    delete_object_idx(creature, this_o_idx);
}

/*!
 * @brief モンスターの移動に伴うオブジェクト処理 (アイテム破壊等)
 * @param creature クリーチャーへの参照
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_idx モンスターID
 * @param ny 移動後の、モンスターのY座標
 * @param nx 移動後の、モンスターのX座標
 */
void update_object_by_monster_movement(CreatureEntity &creature, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, POSITION ny, POSITION nx)
{
    const auto &monster = creature.get_floor()->get_monster(m_idx);
    const auto &monrace = monster.get_monrace();
    const auto &grid = creature.get_floor()->grid_array[ny][nx];
    turn_flags_ptr->do_take = monrace.behavior_flags.has(MonsterBehaviorType::TAKE_ITEM);
    for (auto it = grid.o_idx_list.begin(); it != grid.o_idx_list.end();) {
        EnumClassFlagGroup<MonsterKindType> flg_monster_kind;
        EnumClassFlagGroup<MonsterResistanceType> flgr;
        OBJECT_IDX this_o_idx = *it++;
        auto &item = *creature.get_floor()->o_list[this_o_idx];
        if (turn_flags_ptr->do_take) {
            const auto tval = item.bi_key.tval();
            if (tval == ItemKindType::GOLD || (tval == ItemKindType::MONSTER_REMAINS) || (tval == ItemKindType::STATUE)) {
                continue;
            }
        }

        const auto flags = item.get_flags();
        const auto item_name = describe_flavor(creature, item, 0);
        const auto m_name = monster_desc(creature, monster, MD_INDEF_HIDDEN);
        update_object_flags(flags, flg_monster_kind, flgr);

        auto is_unpickable_object = item.is_fixed_or_random_artifact();
        is_unpickable_object |= monrace.kind_flags.has_any_of(flg_monster_kind);
        is_unpickable_object |= !monrace.resistance_flags.has_all_of(flgr) && monrace.resistance_flags.has_not(MonsterResistanceType::RESIST_ALL);
        monster_pickup_object(creature, turn_flags_ptr, m_idx, &item, is_unpickable_object, ny, nx, m_name, item_name, this_o_idx);
    }
}

/*!
 * @brief モンスターが盗みや拾いで確保していたアイテムを全てドロップさせる / Drop all items carried by a monster
 * @param creature クリーチャーへの参照 (フロア・UI 文脈)
 * @param target ドロップ元クリーチャー (モンスター)
 * @details
 * フェーズ A-2 で inventory[] からのドロップに切替済み、フェーズ A-4b で
 * レガシー hold_o_idx_list 経路を完全削除。フェーズ A-5 で
 * `target.drop_all_inventory(creature)` に集約。
 */
void monster_drop_carried_objects(CreatureEntity &creature, CreatureEntity &target)
{
    if (!target.has_monster_profile()) {
        return;
    }
    target.drop_all_inventory(creature);
}
