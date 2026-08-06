/*!
 * @brief 巻物を読んだ際の効果処理
 * @date 2022/02/26
 * @author Hourier
 */

#include "object-use/read/read-execution.h"
#include "action/action-limited.h"
#include "avatar/avatar.h"
#include "core/window-redrawer.h"
#include "inventory/inventory-object.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object-use/item-use-checker.h"
#include "object-use/read/read-executor-factory.h"
#include "object/object-info.h"
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player-status/player-energy.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-song.h"
#include "status/experience.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "system/redrawing-flags-updater.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief コンストラクタ
 * @param creature クリーチャーへの参照
 * @param i_idx 読むアイテムのインベントリID
 */
ObjectReadEntity::ObjectReadEntity(CreatureEntity &creature, INVENTORY_IDX i_idx)
    : creature(creature)
    , i_idx(i_idx)
{
}

/*!
 * @brief 巻物を読む
 * @param known 判明済ならばTRUE
 */
void ObjectReadEntity::execute(bool known)
{
    auto item = ref_item(this->creature, this->i_idx);
    PlayerEnergy(this->creature).set_player_turn_energy(100);
    if (!this->can_read()) {
        return;
    }

    if (music_singing_any(this->creature)) {
        stop_singing(this->creature);
    }

    SpellHex spell_hex(this->creature);
    if (spell_hex.is_spelling_any() && ((this->creature.get_level() < 35) || spell_hex.is_casting_full_capacity())) {
        (void)SpellHex(this->creature).stop_all_spells();
    }

    auto executor = ReadExecutorFactory::create(this->creature, item.get(), known);
    auto used_up = executor->read();
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    using Srf = StatusRecalculatingFlag;
    EnumClassFlagGroup<Srf> flags_srf = { Srf::COMBINATION, Srf::REORDER };
    if (rfu.has(Srf::AUTO_DESTRUCTION)) {
        flags_srf.set(Srf::AUTO_DESTRUCTION);
    }

    rfu.reset_flags(flags_srf);
    this->change_virtue_as_read(*item);
    item->mark_as_tried();
    this->gain_exp_from_item_use(item.get(), executor->is_identified());
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::INVENTORY,
        SubWindowRedrawingFlag::EQUIPMENT,
        SubWindowRedrawingFlag::PLAYER,
    };
    rfu.set_flags(flags_swrf);
    rfu.set_flags(flags_srf);
    if (!used_up) {
        return;
    }

    sound(SoundKind::SCROLL);
    this->creature.plus_incident_tree("READ_SCROLL", 1);
    vary_item(this->creature, this->i_idx, -1);
}

bool ObjectReadEntity::can_read() const
{
    if (cmd_limit_time_walk(this->creature)) {
        return false;
    }

    if (CreatureClass(this->creature).equals(PlayerClassType::BERSERKER)) {
        msg_print(_("巻物なんて読めない。", "You cannot read."));
        return false;
    }

    return ItemUseChecker(this->creature).check_stun(_("朦々としていて読めなかった！", "You too stunned to read it!"));
}

void ObjectReadEntity::change_virtue_as_read(ItemEntity &o_ref)
{
    if (o_ref.is_aware()) {
        return;
    }

    chg_virtue(this->creature, Virtue::PATIENCE, -1);
    chg_virtue(this->creature, Virtue::CHANCE, 1);
    chg_virtue(this->creature, Virtue::KNOWLEDGE, -1);
}

void ObjectReadEntity::gain_exp_from_item_use(ItemEntity *o_ptr, bool is_identified)
{
    if (!is_identified || o_ptr->is_aware()) {
        return;
    }

    object_aware(this->creature, *o_ptr);
    const auto item_level = o_ptr->get_baseitem_level();
    gain_exp(this->creature, (item_level + (this->creature.get_level() >> 1)) / this->creature.get_level());
}
