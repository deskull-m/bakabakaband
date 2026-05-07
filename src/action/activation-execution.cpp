/*!
 * @file activation-execution.cpp
 * @brief アイテムの発動実行定義
 */

#include "action/activation-execution.h"
#include "action/action-limited.h"
#include "artifact/random-art-effects.h"
#include "core/window-redrawer.h"
#include "effect/attribute-types.h"
#include "effect/spells-effect-util.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/geometry.h"
#include "game-option/disturbance-options.h"
#include "game-option/input-options.h"
#include "inventory/inventory-slot-types.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-info.h"
#include "monster/monster-util.h"
#include "object-activation/activation-switcher.h"
#include "object-activation/activation-util.h"
#include "object-enchant/activation-info-table.h"
#include "object-enchant/object-ego.h"
#include "object/object-info.h"
#include "player-base/player-class.h"
#include "player-status/player-energy.h"
#include "racial/racial-android.h"
#include "specific-object/monster-ball.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-song.h"
#include "spell/spells-object.h"
#include "sv-definition/sv-bow-types.h"
#include "sv-definition/sv-junk-types.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-ring-types.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-key.h"
#include "system/baseitem/baseitem-list.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/item-entity.h"
#include "system/redrawing-flags-updater.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "view/display-messages.h"
#include "world/world.h"

static void decide_chance_fail(CreatureEntity &creature, ae_type *ae_ptr)
{
    ae_ptr->chance = creature.get_skill_device();
    if (creature.is_confused()) {
        ae_ptr->chance = ae_ptr->chance / 2;
    }

    ae_ptr->fail = ae_ptr->lev + 5;
    if (ae_ptr->chance > ae_ptr->fail) {
        ae_ptr->fail -= (ae_ptr->chance - ae_ptr->fail) * 2;
    } else {
        ae_ptr->chance -= (ae_ptr->fail - ae_ptr->chance) * 2;
    }

    if (ae_ptr->fail < USE_DEVICE) {
        ae_ptr->fail = USE_DEVICE;
    }

    if (ae_ptr->chance < USE_DEVICE) {
        ae_ptr->chance = USE_DEVICE;
    }
}

static void decide_activation_success(CreatureEntity &creature, ae_type *ae_ptr)
{
    if (CreatureClass(creature).equals(PlayerClassType::BERSERKER)) {
        ae_ptr->success = false;
        return;
    }

    if (ae_ptr->chance > ae_ptr->fail) {
        ae_ptr->success = randint0(ae_ptr->chance * 2) >= ae_ptr->fail;
        return;
    }

    ae_ptr->success = randint0(ae_ptr->fail * 2) < ae_ptr->chance;
}

static bool check_activation_success(ae_type *ae_ptr)
{
    if (ae_ptr->success) {
        return true;
    }

    if (flush_failure) {
        flush();
    }

    msg_print(_("うまく始動させることができなかった。", "You failed to activate it properly."));
    sound(SoundKind::FAIL);
    return false;
}

static bool check_activation_conditions(CreatureEntity &creature, ae_type *ae_ptr)
{
    if (!check_activation_success(ae_ptr)) {
        return false;
    }

    if (ae_ptr->item->timeout) {
        msg_print(_("それは微かに音を立て、輝き、消えた...", "It whines, glows and fades..."));
        return false;
    }

    if (ae_ptr->item->is_fuel() && (ae_ptr->item->fuel == 0)) {
        msg_print(_("燃料がない。", "It has no fuel."));
        PlayerEnergy(creature).reset_player_turn();
        return false;
    }

    return true;
}

/*!
 * @brief アイテムの発動効果を処理する。
 * @param creature クリーチャーへの参照
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 発動実行の是非を返す。
 */
static bool activate_artifact(CreatureEntity &creature, ItemEntity *o_ptr)
{
    const auto it_activation = o_ptr->find_activation_info();
    if (it_activation == activation_info.end()) {
        msg_print("Activation information is not found.");
        return false;
    }

    const auto item_name = describe_flavor(creature, *o_ptr, OD_NAME_ONLY | OD_OMIT_PREFIX | OD_BASE_NAME);
    if (!switch_activation(creature, &o_ptr, it_activation->index, item_name)) {
        return false;
    }

    if (it_activation->constant) {
        o_ptr->timeout = static_cast<short>(*it_activation->constant);
        if (it_activation->dice > 0) {
            o_ptr->timeout += static_cast<short>(randint1(it_activation->dice));
        }

        return true;
    }

    switch (it_activation->index) {
    case RandomArtActType::BR_FIRE:
        o_ptr->timeout = o_ptr->bi_key == BaseitemKey(ItemKindType::RING, SV_RING_FLAMES) ? 200 : 250;
        return true;
    case RandomArtActType::BR_COLD:
        o_ptr->timeout = o_ptr->bi_key == BaseitemKey(ItemKindType::RING, SV_RING_ICE) ? 200 : 250;
        return true;
    case RandomArtActType::TERROR:
        o_ptr->timeout = 3 * (creature.level + 10);
        return true;
    case RandomArtActType::MURAMASA:
        return true;
    default:
        msg_format("Special timeout is not implemented: %d.", enum2i(it_activation->index));
        return false;
    }
}

static bool activate_whistle(CreatureEntity &user, ae_type *ae_ptr)
{
    if (ae_ptr->item->bi_key.tval() != ItemKindType::WHISTLE) {
        return false;
    }

    if (music_singing_any(user)) {
        stop_singing(user);
    }

    if (SpellHex(user).is_spelling_any()) {
        (void)SpellHex(user).stop_all_spells();
    }

    const auto &floor = *user.get_floor();
    std::vector<short> pet_index;
    for (short pet_indice = floor.m_max - 1; pet_indice >= 1; pet_indice--) {
        const auto &monster = floor.get_monster(pet_indice);
        if (monster.is_pet() && (user.riding != pet_indice)) {
            pet_index.push_back(pet_indice);
        }
    }

    std::stable_sort(pet_index.begin(), pet_index.end(), [&floor](auto x, auto y) { return floor.order_pet_whistle(x, y); });
    for (auto pet_indice : pet_index) {
        teleport_monster_to(user, pet_indice, user.y, user.x, 100, TELEPORT_PASSIVE);
    }

    ae_ptr->item->timeout = 100 + randint1(100);
    return true;
}

static bool scouter_probing(CreatureEntity &user, ae_type *ae_ptr)
{
    if (ae_ptr->item->bi_key.tval() != ItemKindType::HELM || ae_ptr->item->bi_key.sval() != SV_SCOUTER) {
        return false;
    }

    probing(user);
    return true;
}

static bool activate_firethrowing(CreatureEntity &creature, ae_type *ae_ptr)
{
    if (ae_ptr->item->bi_key.tval() != ItemKindType::BOW || ae_ptr->item->bi_key.sval() != SV_FLAMETHROWER) {
        return false;
    }

    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    msg_print(_("汚物は消毒だあ！", "The filth must be disinfected!"));
    fire_breath(creature, AttributeType::FIRE, dir, 20 + creature.level * 5, 2);
    return true;
}

static bool activate_rosmarinus(CreatureEntity &creature, ae_type *ae_ptr)
{
    if (ae_ptr->item->bi_key.tval() != ItemKindType::BOW || ae_ptr->item->bi_key.sval() != SV_ROSMARINUS) {
        return false;
    }

    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    fire_breath(creature, AttributeType::MISSILE, dir, 20 + creature.level * 5, 2);
    return true;
}

static bool activate_stungun(CreatureEntity &creature, ae_type *ae_ptr)
{
    if (ae_ptr->item->bi_key.tval() != ItemKindType::JUNK || ae_ptr->item->bi_key.sval() != SV_STUNGUN) {
        return false;
    }

    project_length = 1;
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    msg_print(_("『バチィ』", "'bzzt'"));
    fire_ball(creature, AttributeType::STUNGUN, dir, creature.level, 0);
    project_length = 0;
    return true;
}

static bool activate_raygun(CreatureEntity &creature, ae_type *ae_ptr)
{
    if (ae_ptr->item->bi_key.tval() != ItemKindType::BOW || ae_ptr->item->bi_key.sval() != SV_RAYGUN) {
        return false;
    }

    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    msg_print(_("『ビィーム！』", "'ZAP! ZAP!'"));
    fire_bolt(creature, AttributeType::MISSILE, dir, 10 + creature.level * 2);
    return true;
}

/*!
 * @brief 装備を発動するコマンドのサブルーチン
 * @param creature クリーチャーへの参照
 * @param i_idx 発動するオブジェクトの所持品ID
 */
void exe_activate(CreatureEntity &creature, INVENTORY_IDX i_idx)
{
    bool activated = false;
    if (i_idx <= INVEN_PACK && BaseitemList::get_instance().get_baseitem(creature.inventory[i_idx]->bi_id).flags.has_not(TR_INVEN_ACTIVATE)) {
        msg_print(_("このアイテムは装備しないと始動できない。", "That object must be activated by equipment."));
        return;
    }

    ae_type ae(creature, i_idx);
    auto *ae_ptr = &ae;

    if (ae_ptr->item->ego_idx == EgoType::SHATTERED || ae_ptr->item->ego_idx == EgoType::BLASTED) {
        msg_print(_("このアイテムはもう壊れていて始動できない。", "That broken object can't be activated."));
        return;
    }

    PlayerEnergy(creature).set_player_turn_energy(100);
    ae_ptr->decide_activation_level();
    decide_chance_fail(creature, ae_ptr);
    if (cmd_limit_time_walk(creature)) {
        return;
    }

    decide_activation_success(creature, ae_ptr);
    if (!check_activation_conditions(creature, ae_ptr)) {
        return;
    }

    msg_print(_("始動させた...", "You activate it..."));
    sound(SoundKind::ZAP);
    if (ae_ptr->item->has_activation()) {
        (void)activate_artifact(creature, ae_ptr->item.get());
        static constexpr auto flags = {
            SubWindowRedrawingFlag::INVENTORY,
            SubWindowRedrawingFlag::EQUIPMENT,
        };
        RedrawingFlagsUpdater::get_instance().set_flags(flags);
        return;
    }

    if (activate_whistle(creature, ae_ptr)) {
        activated = true;
    } else if (scouter_probing(creature, ae_ptr)) {
        activated = true;
    } else if (exe_monster_capture(creature, *ae_ptr->item)) {
        activated = true;
    } else if (activate_firethrowing(creature, ae_ptr)) {
        activated = true;
    } else if (activate_rosmarinus(creature, ae_ptr)) {
        activated = true;
    } else if (activate_stungun(creature, ae_ptr)) {
        activated = true;
    } else if (activate_raygun(creature, ae_ptr)) {
        activated = true;
    }

    if (!activated) {
        msg_print(_("おっと、このアイテムは始動できない。", "Oops.  That object cannot be activated."));
        return;
    }

    if (randint1(100) <= ae_ptr->broken) {
        std::string o_name = describe_flavor(creature, *ae_ptr->item, OD_OMIT_PREFIX);
        msg_format(_("%sは壊れた！", "%s is destroyed!"), o_name.data());
        curse_weapon_object(creature, true, *ae_ptr->item);
    }
}
