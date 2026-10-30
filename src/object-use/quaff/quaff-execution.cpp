/*!
 * @brief 薬を飲んだ時の各種効果処理
 * @date 2022/03/10
 * @author Hourier
 */

#include "object-use/quaff/quaff-execution.h"
#include "avatar/avatar.h"
#include "core/window-redrawer.h"
#include "game-option/disturbance-options.h"
#include "inventory/inventory-object.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object-use/item-use-checker.h"
#include "object-use/quaff/quaff-effects.h"
#include "object/object-broken.h"
#include "object/object-info.h"
#include "perception/object-perception.h"
#include "player-base/player-race.h"
#include "player-info/mimic-info-table.h"
#include "player-status/player-energy.h"
#include "player/digestion-processor.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-song.h"
#include "status/experience.h"
#include "system/creature-entity.h"
#include "system/deferred-update-requests.h"
#include "system/item-entity.h"
#include "term/screen-processor.h"
#include "view/display-messages.h"
#include <algorithm>

/*!
 * @brief 水分を栄養にする種族が薬を飲んだ後の満腹度を計算する
 * @param food 飲む前の満腹度
 * @param pval 薬の滋養
 * @return 飲んだ後の満腹度 (0以上 PY_FOOD_MAX - 1 以下)
 * @details 滋養の10倍や和を short で計算すると回り込むので、int で計算してから抑える。
 * 滋養が負なら10倍の項は足さない。
 */
short calc_moistened_food(short food, short pval)
{
    const auto value = food + pval + std::max(0, pval * 10) + 2000;
    return static_cast<short>(std::clamp(value, 0, PY_FOOD_MAX - 1));
}

/*!
 * @brief コンストラクタ
 * @param creature クリーチャーへの参照
 */
ObjectQuaffEntity::ObjectQuaffEntity(CreatureEntity &creature)
    : creature(creature)
{
}

/*!
 * @brief 薬を飲む.
 * @param i_idx 薬のインベントリID
 * @details
 * 効果発動のあと、食料タイプによって空腹度を少し充足する。
 * 但し骸骨は除く
 */
void ObjectQuaffEntity::execute(INVENTORY_IDX i_idx, bool is_rectal)
{
    if (!this->can_influence()) {
        return;
    }

    auto item = this->copy_object(i_idx);
    vary_item(this->creature, i_idx, -1);
    sound(SoundKind::QUAFF);
    this->creature.plus_incident_tree("QUAFF", 1);
    auto ident = QuaffEffects(this->creature).influence(item, is_rectal);
    if (CreatureRace(&this->creature).equals(PlayerRaceType::SKELETON)) {
        msg_print(_("液体の一部はあなたのアゴを素通りして落ちた！", "Some of the fluid falls through your jaws!"));
        (void)potion_smash_effect(this->creature, 0, this->creature.y, this->creature.x, item.bi_id);
    }

    static constexpr auto flags_srf = {
        InventoryArrangementFlag::COMBINATION,
        InventoryArrangementFlag::REORDER,
    };
    auto &dur = DeferredUpdateRequests::get_instance();
    dur.set_flags(flags_srf);
    this->change_virtue_as_quaff(item);
    item.mark_as_tried();
    if (ident && !item.is_aware()) {
        object_aware(this->creature, item);
        gain_exp(this->creature, (item.get_baseitem_level() + (this->creature.get_level() >> 1)) / this->creature.get_level());
    }

    dur.set_item_related_sub_window_flags();
    if (CreatureRace(&this->creature).equals(PlayerRaceType::SKELETON)) {
        return;
    }

    this->moisten(item);
}

bool ObjectQuaffEntity::can_influence()
{
    PlayerEnergy(this->creature).set_player_turn_energy(100);
    if (!this->can_quaff()) {
        return false;
    }

    if (music_singing_any(this->creature)) {
        stop_singing(this->creature);
    }

    SpellHex spell_hex(this->creature);
    if (spell_hex.is_spelling_any() && !spell_hex.is_spelling_specific(HEX_INHALE)) {
        (void)SpellHex(this->creature).stop_all_spells();
    }

    return true;
}

bool ObjectQuaffEntity::can_quaff()
{
    if (this->creature.is_timewalking()) {
        if (flush_failure) {
            flush();
        }

        msg_print(_("瓶から水が流れ出てこない！", "The potion doesn't flow out from the bottle."));
        sound(SoundKind::FAIL);
        return false;
    }

    return ItemUseChecker(this->creature).check_stun(_("朦朧としていて瓶の蓋を開けられなかった！", "You are too stunned to quaff it!"));
}

ItemEntity ObjectQuaffEntity::copy_object(const INVENTORY_IDX i_idx)
{
    auto o_val = ref_item(this->creature, i_idx)->clone();
    o_val.number = 1;
    return o_val;
}

void ObjectQuaffEntity::moisten(const ItemEntity &o_ref)
{
    switch (CreatureRace(&this->creature).food()) {
    case PlayerRaceFoodType::WATER:
        msg_print(_("水分を取り込んだ。", "You are moistened."));
        set_food(this->creature, calc_moistened_food(this->creature.get_food(), o_ref.pval));
        return;
    case PlayerRaceFoodType::OIL:
        if (o_ref.bi_key.tval() != ItemKindType::FLASK) {
            set_food(this->creature, this->creature.get_food() + ((o_ref.pval) / 20));
            return;
        }

        msg_print(_("オイルを補給した。", "You replenish yourself with the oil."));
        set_food(this->creature, this->creature.get_food() + 5000);
        return;
    case PlayerRaceFoodType::BLOOD:
        (void)set_food(this->creature, this->creature.get_food() + (o_ref.pval / 10));
        return;
    case PlayerRaceFoodType::MANA:
    case PlayerRaceFoodType::MONSTER_REMAINS:
        set_food(this->creature, this->creature.get_food() + ((o_ref.pval) / 20));
        return;
    default:
        (void)set_food(this->creature, this->creature.get_food() + o_ref.pval);
        return;
    }
}

void ObjectQuaffEntity::change_virtue_as_quaff(const ItemEntity &o_ref)
{
    if (o_ref.is_aware()) {
        return;
    }

    chg_virtue(this->creature, Virtue::PATIENCE, -1);
    chg_virtue(this->creature, Virtue::CHANCE, 1);
    chg_virtue(this->creature, Virtue::KNOWLEDGE, -1);
}
