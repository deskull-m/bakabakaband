#include "cmd-item/cmd-equipment.h"
#include "action/weapon-shield.h"
#include "artifact/fixed-art-types.h"
#include "autopick/autopick.h"
#include "avatar/avatar.h"
#include "core/asking-player.h"
#include "core/window-redrawer.h"
#include "dungeon/quest.h" //!< @todo 違和感、何故アイテムを装備するとクエストの成功判定が走るのか？.
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "game-option/birth-options.h"
#include "game-option/input-options.h"
#include "inventory/inventory-describer.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "locale/japanese.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/trc-types.h"
#include "object-hook/hook-armor.h"
#include "object-hook/hook-weapon.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-info.h"
#include "object/object-mark-types.h"
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/equipment-info.h"
#include "player-info/samurai-data-type.h"
#include "player-status/player-energy.h"
#include "player-status/player-hand-types.h"
#include "player/attack-defense-types.h"
#include "player/player-status.h"
#include "player/special-defense-types.h"
#include "racial/racial-android.h"
#include "spell-kind/spells-perception.h"
#include "status/action-setter.h"
#include "status/shape-changer.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "view/display-inventory.h"
#include "view/display-messages.h"
#include <fmt/format.h>
#include <utility>

/*!
 * @brief 装備時にアイテムを呪う処理
 */
static void do_curse_on_equip(OBJECT_IDX slot, ItemEntity &item, CreatureEntity &creature)
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (set_anubis_and_chariot(creature) && ((slot == INVEN_MAIN_HAND) || (slot == INVEN_SUB_HAND))) {

        ItemEntity *anubis = creature.inventory[INVEN_MAIN_HAND].get();
        ItemEntity *chariot = creature.inventory[INVEN_SUB_HAND].get();

        anubis->curse_flags.set(CurseTraitType::PERSISTENT_CURSE);
        anubis->curse_flags.set(CurseTraitType::HEAVY_CURSE);
        chariot->curse_flags.set(CurseTraitType::PERSISTENT_CURSE);
        chariot->curse_flags.set(CurseTraitType::HEAVY_CURSE);
        chariot->curse_flags.set(CurseTraitType::BERS_RAGE);
        chariot->curse_flags.set(CurseTraitType::VUL_CURSE);

        msg_print(_("『銀の戦車』プラス『アヌビス神』二刀流ッ！", "*Silver Chariot* plus *Anubis God* Two Swords!"));
        rfu.set_flag(StatusRecalculatingFlag::BONUS);
        return;
    }

    auto should_curse = item.get_flags().has(TR_PERSISTENT_CURSE) || item.curse_flags.has(CurseTraitType::PERSISTENT_CURSE);
    should_curse &= item.curse_flags.has_not(CurseTraitType::HEAVY_CURSE);
    if (!should_curse) {
        return;
    }

    const auto item_name = describe_flavor(creature, item, (OD_OMIT_PREFIX | OD_NAME_ONLY));
    item.curse_flags.set(CurseTraitType::HEAVY_CURSE);
    msg_print(_("悪意に満ちた黒いオーラが{}をとりまいた...", "There is a malignant black aura surrounding your {}..."), item_name);
    item.feeling = FEEL_NONE;
    rfu.set_flag(StatusRecalculatingFlag::BONUS);
}

/*!
 * @brief 装備一覧を表示するコマンドのメインルーチン / Display equipment
 */
void do_cmd_equip(CreatureEntity &creature)
{
    command_wrk = true;
    if (easy_floor) {
        command_wrk = USE_EQUIP;
    }

    screen_save();
    (void)show_equipment(creature, 0, USE_FULL, AllMatchItemTester());
    auto weight = calc_inventory_weight(creature);
    auto weight_lim = calc_weight_limit(creature);
    constexpr auto fmt = _("装備： 合計 {:3}.{} kg (限界の{}%) コマンド: ", "Equipment: carrying {}.{} pounds ({}% of capacity). Command: ");
#ifdef JP
    const auto out_val = fmt::format(fmt, lb_to_kg_integer(weight), lb_to_kg_fraction(weight), weight * 100 / weight_lim);
#else
    const auto out_val = fmt::format(fmt, weight / 10, weight % 10, weight * 100 / weight_lim);
#endif

    prt(out_val, 0, 0);
    command_new = inkey();
    screen_load();

    if (command_new != ESCAPE) {
        command_see = true;
        return;
    }

    const auto &[wid, hgt] = term_get_size();
    command_new = 0;
    command_gap = wid - 30;
}

/*!
 * @brief 装備するコマンドのメインルーチン / Wield or wear a single item from the pack or floor
 * @param creature クリーチャーへの参照
 */
void do_cmd_wield(CreatureEntity &creature)
{
    OBJECT_IDX need_switch_wielding = 0;
    CreatureClass(creature).break_samurai_stance({ SamuraiStanceType::MUSOU });

    constexpr auto selection_q = _("どれを装備しますか? ", "Wear/Wield which item? ");
    constexpr auto selection_s = _("装備可能なアイテムがない。", "You have nothing you can wear or wield.");
    const auto &[item_chosen, i_idx] = choose_item(creature, selection_q, selection_s, (USE_INVEN | USE_FLOOR), FuncItemTester(item_tester_hook_wear, creature));
    if (!item_chosen) {
        return;
    }

    auto slot = wield_slot(creature, *item_chosen);

    // 肛門破壊チェック
    if (slot == INVEN_ASSHOLE && creature.get_mutations().has(PlayerMutationType::DESTROYED_ASSHOLE)) {
        msg_print(_("あなたの肛門は完全に破壊されており、何も装備できない！", "Your asshole is completely destroyed and cannot equip anything!"));
        return;
    }

    // 頭部失失チェック
    if (slot == INVEN_HEAD && creature.get_mutations().has(PlayerMutationType::LOST_HEAD)) {
        msg_print(_("あなたは頭がないので、頭に何も装備できない！", "You have no head and cannot equip anything on your head!"));
        return;
    }
    const auto o_ptr_mh = creature.inventory[INVEN_MAIN_HAND].get();
    const auto o_ptr_sh = creature.inventory[INVEN_SUB_HAND].get();
    const auto tval = item_chosen->bi_key.tval();
    switch (tval) {
    case ItemKindType::CAPTURE:
    case ItemKindType::SHIELD:
    case ItemKindType::CARD:
        if (has_melee_weapon(creature, INVEN_MAIN_HAND) && has_melee_weapon(creature, INVEN_SUB_HAND)) {
            constexpr auto q = _("どちらの武器と取り替えますか?", "Replace which weapon? ");
            constexpr auto s = _("おっと。", "Oops.");
            const auto &[item_replace, slot_replace] = choose_item(creature, q, s, (USE_EQUIP | IGNORE_BOTHHAND_SLOT), FuncItemTester(&ItemEntity::is_melee_weapon));
            if (!item_replace) {
                return;
            }

            slot = slot_replace;
            if (slot == INVEN_MAIN_HAND) {
                need_switch_wielding = INVEN_SUB_HAND;
            }
        } else if (has_melee_weapon(creature, INVEN_SUB_HAND)) {
            slot = INVEN_MAIN_HAND;
        } else if (o_ptr_mh->is_valid() && o_ptr_sh->is_valid() &&
                   ((tval == ItemKindType::CAPTURE) || (!o_ptr_mh->is_melee_weapon() && !o_ptr_sh->is_melee_weapon()))) {
            constexpr auto q = _("どちらの手に装備しますか?", "Equip which hand? ");
            constexpr auto s = _("おっと。", "Oops.");
            const auto &[item_replace, slot_new] = choose_item(creature, q, s, (USE_EQUIP), FuncItemTester(&ItemEntity::is_wieldable_in_etheir_hand));
            if (!item_replace) {
                return;
            }

            slot = slot_new;
        }

        break;
    case ItemKindType::DIGGING:
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::SWORD:
        if (slot == INVEN_SUB_HAND) {
            if (!input_check(_("二刀流で戦いますか？", "Dual wielding? "))) {
                slot = INVEN_MAIN_HAND;
            }
        } else if (!o_ptr_mh->is_valid() && has_melee_weapon(creature, INVEN_SUB_HAND)) {
            if (!input_check(_("二刀流で戦いますか？", "Dual wielding? "))) {
                slot = INVEN_SUB_HAND;
            }
        } else if (o_ptr_mh->is_valid() && o_ptr_sh->is_valid()) {
            constexpr auto q = _("どちらの手に装備しますか?", "Equip which hand? ");
            constexpr auto s = _("おっと。", "Oops.");
            const auto &[item_new, slot_new] = choose_item(creature, q, s, (USE_EQUIP), FuncItemTester(&ItemEntity::is_wieldable_in_etheir_hand));
            if (!item_new) {
                return;
            }

            slot = slot_new;
            if ((slot == INVEN_SUB_HAND) && !has_melee_weapon(creature, INVEN_MAIN_HAND)) {
                need_switch_wielding = INVEN_MAIN_HAND;
            }
        }

        break;
    case ItemKindType::RING: {
        std::string q;
        if (creature.inventory[INVEN_SUB_RING]->is_valid() && creature.inventory[INVEN_MAIN_RING]->is_valid()) {
            q = _("どちらの指輪と取り替えますか?", "Replace which ring? ");
        } else {
            q = _("どちらの手に装備しますか?", "Equip which hand? ");
        }

        constexpr auto s = _("おっと。", "Oops.");
        creature.set_select_ring_slot(true);
        const auto &[item_replace, slot_replace] = choose_item(creature, q, s, (USE_EQUIP | IGNORE_BOTHHAND_SLOT));
        if (!item_replace) {
            creature.set_select_ring_slot(false);
            return;
        }

        slot = slot_replace;
        creature.set_select_ring_slot(false);
        break;
    }
    default:
        break;
    }

    if (creature.inventory[slot]->is_cursed()) {
        const auto item_name = describe_flavor(creature, *creature.inventory[slot], OD_OMIT_PREFIX | OD_NAME_ONLY);
#ifdef JP
        msg_print("{}{}は呪われているようだ。", describe_use(creature, slot), item_name);
#else
        msg_print("The {} you are {} appears to be cursed.", item_name, describe_use(creature, slot));
#endif
        return;
    }

    auto should_equip_cursed = item_chosen->is_cursed() && item_chosen->is_known();
    should_equip_cursed |= item_chosen->ident.has(IdentificationFlag::SENSE) && is_cursed_feeling(item_chosen->feeling);
    should_equip_cursed &= confirm_wear;
    if (should_equip_cursed) {
        const auto item_name = describe_flavor(creature, *item_chosen, (OD_OMIT_PREFIX | OD_NAME_ONLY));
        if (!input_check(fmt::format(_("本当に{}{{呪われている}}を使いますか？", "Really use the {} {{cursed}}? "), item_name))) {
            return;
        }
    }

    CreatureRace pr(&creature);
    auto should_change_vampire = item_chosen->is_specific_artifact(FixedArtifactId::STONEMASK);
    should_change_vampire &= item_chosen->is_known();
    should_change_vampire &= !pr.equals(PlayerRaceType::VAMPIRE);
    should_change_vampire &= !pr.equals(PlayerRaceType::ANDROID);
    if (should_change_vampire) {
        const auto item_name = describe_flavor(creature, *item_chosen, (OD_OMIT_PREFIX | OD_NAME_ONLY));
        constexpr auto mes = _("{}を装備すると吸血鬼になります。よろしいですか？", "{} will transform you into a vampire permanently when equipped. Do you become a vampire? ");
        if (!input_check(fmt::format(mes, item_name))) {
            return;
        }
    }

    sound(SoundKind::WIELD);
    if (need_switch_wielding && !creature.inventory[need_switch_wielding]->is_cursed()) {
        auto &slot_item = *creature.inventory[slot];
        auto &switch_item = *creature.inventory[need_switch_wielding];
        const auto item_name = describe_flavor(creature, switch_item, (OD_OMIT_PREFIX | OD_NAME_ONLY));
        std::swap(switch_item, slot_item);
        msg_print(_("{}を{}に構えなおした。", "You wield {} at {} hand."), item_name, (slot == INVEN_MAIN_HAND) ? (left_hander ? _("左手", "left") : _("右手", "right")) : (left_hander ? _("右手", "right") : _("左手", "left")));
        slot = need_switch_wielding;
    }

    check_find_art_quest_completion(creature, item_chosen.get());
    if (creature.ppersonality == PERSONALITY_MUNCHKIN) {
        identify_item(creature, item_chosen.get());
        autopick_alter_item(creature, i_idx, false);
    }

    PlayerEnergy(creature).set_player_turn_energy(100);
    auto item = item_chosen->clone();
    item.number = 1;
    if (i_idx >= 0) {
        inven_item_increase(creature, i_idx, -1);
        inven_item_optimize(creature, i_idx);
    } else {
        floor_item_increase(creature, 0 - i_idx, -1);
        floor_item_optimize(creature, 0 - i_idx);
    }

    auto &wield_slot_item = *creature.inventory[slot];
    if (wield_slot_item.is_valid()) {
        (void)inven_takeoff(creature, slot, 255);
    }

    wield_slot_item = std::move(item);
    wield_slot_item.marked.set(OmType::TOUCHED);

    using WieldMessageFormat = fmt::format_string<std::string_view, char>;
    constexpr WieldMessageFormat wield_hand_right = _("{}({:c})を右手に装備した。", "You are wielding {} ({:c}) in your right hand.");
    constexpr WieldMessageFormat wield_hand_left = _("{}({:c})を左手に装備した。", "You are wielding {} ({:c}) in your left hand.");
    constexpr WieldMessageFormat wield_hands_two = _("{}({:c})を両手で構えた。", "You are wielding {} ({:c}) with both hands.");
    WieldMessageFormat act = _("{}({:c})を装備した。", "You are wearing {} ({:c}).");

    switch (slot) {
    case INVEN_MAIN_HAND:
        if (wield_slot_item.allow_two_hands_wielding() && (empty_hands(creature, false) == EMPTY_HAND_SUB) && can_two_hands_wielding(creature)) {
            act = wield_hands_two;
        } else if (left_hander) {
            act = wield_hand_left;
        } else {
            act = wield_hand_right;
        }

        break;
    case INVEN_SUB_HAND:
        if (wield_slot_item.allow_two_hands_wielding() && (empty_hands(creature, false) == EMPTY_HAND_MAIN) && can_two_hands_wielding(creature)) {
            act = wield_hands_two;
        } else if (left_hander) {
            act = wield_hand_right;
        } else {
            act = wield_hand_left;
        }

        break;
    case INVEN_BOW:
        act = _("{}({:c})を射撃用に装備した。", "You are shooting with {} ({:c}).");
        break;
    case INVEN_LITE:
        act = _("{}({:c})を光源にした。", "Your light source is {} ({:c}).");
        break;
    default:
        break;
    }

    const auto item_name = describe_flavor(creature, wield_slot_item, 0);
    msg_print(act, std::string_view(item_name), index_to_label(slot));
    if (wield_slot_item.is_cursed()) {
        msg_print(_("うわ！ すさまじく冷たい！", "Oops! It feels deathly cold!"));
        chg_virtue(creature, Virtue::HARMONY, -1);
        wield_slot_item.ident.set(IdentificationFlag::SENSE);
    }

    do_curse_on_equip(slot, wield_slot_item, creature);
    if (wield_slot_item.is_specific_artifact(FixedArtifactId::STONEMASK)) {
        auto is_specific_race = pr.equals(PlayerRaceType::VAMPIRE);
        is_specific_race |= pr.equals(PlayerRaceType::ANDROID);
        if (!is_specific_race) {
            change_race(creature, PlayerRaceType::VAMPIRE, "");
        }
    }

    calc_android_exp(creature);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.request_equipment_change_updates();
    rfu.set_flag(MainWindowRedrawingFlag::EQUIPPY);
    rfu.set_flag(SubWindowRedrawingFlag::PLAYER);
}

/*!
 * @brief 装備を外すコマンドのメインルーチン / Take off an item
 */
void do_cmd_takeoff(CreatureEntity &creature)
{
    CreatureClass pc(creature);
    pc.break_samurai_stance({ SamuraiStanceType::MUSOU });

    constexpr auto q = _("どれを装備からはずしますか? ", "Take off which item? ");
    constexpr auto s = _("はずせる装備がない。", "You are not wearing anything to take off.");
    const auto &[item, i_idx] = choose_item(creature, q, s, (USE_EQUIP | IGNORE_BOTHHAND_SLOT));
    if (!item) {
        return;
    }

    PlayerEnergy energy(creature);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (item->is_cursed()) {
        if (item->curse_flags.has(CurseTraitType::PERMA_CURSE) || !pc.equals(PlayerClassType::BERSERKER)) {
            msg_print(_("ふーむ、どうやら呪われているようだ。", "Hmmm, it seems to be cursed."));
            return;
        }

        if ((item->curse_flags.has(CurseTraitType::HEAVY_CURSE) && one_in_(7)) || one_in_(4)) {
            msg_print(_("呪われた装備を力づくで剥がした！", "You tore off a piece of cursed equipment by sheer strength!"));
            item->remove_curses();
            rfu.set_flag(StatusRecalculatingFlag::BONUS);
            msg_print(_("呪いを打ち破った。", "You break the curse."));
        } else {
            msg_print(_("装備を外せなかった。", "You couldn't remove the equipment."));
            energy.set_player_turn_energy(50);
            return;
        }
    }

    sound(SoundKind::TAKE_OFF);
    energy.set_player_turn_energy(50);
    (void)inven_takeoff(creature, i_idx, 255);
    verify_equip_slot(creature, i_idx);
    calc_android_exp(creature);
    rfu.request_equipment_change_updates();
    rfu.set_flag(MainWindowRedrawingFlag::EQUIPPY);
    rfu.set_flag(SubWindowRedrawingFlag::PLAYER);
}
