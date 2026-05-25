/*!
 * @brief 町の施設処理 / Building commands
 * @date 2013/12/23
 * @author
 * Created by Ken Wigle for Kangband - a variant of Angband 2.8.3
 * -KMW-
 *
 * Rewritten for Kangband 2.8.3i using Kamband's version of
 * building.c as written by Ivan Tkatchev
 *
 * Changed for ZAngband by Robert Ruehlmann
 */

#include "cmd-building/cmd-building.h"
#include "avatar/avatar.h"
#include "cmd-action/cmd-spell.h"
#include "cmd-building/cmd-inn.h"
#include "cmd-io/cmd-dump.h"
#include "core/asking-player.h"
#include "core/scores.h"
#include "core/show-file.h"
#include "core/special-internal-keys.h"
#include "core/stuff-handler.h"
#include "floor/floor-mode-changer.h"
#include "floor/wild.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "main/music-definitions-table.h"
#include "main/sound-of-music.h"
#include "market/arena-entry.h"
#include "market/arena.h"
#include "market/bounty.h"
#include "market/building-actions-table.h"
#include "market/building-craft-armor.h"
#include "market/building-craft-fix.h"
#include "market/building-craft-weapon.h"
#include "market/building-enchanter.h"
#include "market/building-monster.h"
#include "market/building-quest.h"
#include "market/building-recharger.h"
#include "market/building-service.h"
#include "market/building-util.h"
#include "market/melee-arena.h"
#include "market/play-gamble.h"
#include "market/poker.h"
#include "mutation/mutation-flag-types.h"
#include "mutation/mutation-investor-remover.h"
#include "object-hook/hook-armor.h"
#include "object-hook/hook-weapon.h"
#include "object/item-tester-hooker.h"
#include "player-status/player-energy.h"
#include "player/player-personality-types.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-polymorph.h"
#include "spell-kind/spells-world.h"
#include "spell/spells-status.h"
#include "store/cmd-store.h"
#include "store/store-util.h"
#include "system/angband-exceptions.h"
#include "system/angband-system.h"
#include "system/building-type-definition.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain/terrain-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "util/enum-converter.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief 町に関するヘルプを表示する / Display town history
 * @param creature クリーチャーへの参照
 */
static void town_history(CreatureEntity &creature)
{
    screen_save();
    FileDisplayer(creature.name).display(true, _("jbldg.txt", "bldg.txt"), 0, 0);
    screen_load();
}

/*!
 * @brief 施設の処理実行メインルーチン
 * @param creature クリーチャーへの参照
 * @param bldg 施設構造体の参照ポインタ
 * @param i 実行したい施設のサービステーブルの添字
 * @return 施設から別フロアへ移動するか否か (アリーナ/モンスター闘技場のみtrue)
 */
static bool bldg_process_command(CreatureEntity &creature, const building_type &bldg, int i)
{
    auto &world = AngbandWorld::get_instance();
    msg_flag = false;
    msg_erase();
    const auto can_be_owner = is_owner(creature, bldg);
    const auto building_cost = can_be_owner ? bldg.member_costs[i] : bldg.other_costs[i];
    if (((bldg.action_restr[i] == 1) && !is_member(creature, bldg)) || ((bldg.action_restr[i] == 2) && !can_be_owner)) {
        msg_print(_("それを選択する権利はありません！", "You have no right to choose that!"));
        return false;
    }

    const auto building_action = bldg.actions[i];
    if ((building_action != BACT_RECHARGE) && (((bldg.member_costs[i] > creature.get_au()) && can_be_owner) || ((bldg.other_costs[i] > creature.get_au()) && !can_be_owner))) {
        msg_print(_("お金が足りません！", "You do not have the gold!"));
        return false;
    }

    switch (building_action) {
    case BACT_NOTHING:
        /* Do nothing */
        return false;
    case BACT_RESEARCH_ITEM:
        if (identify_fully(creature, false)) {
            creature.sub_au(building_cost);
        }

        return false;
    case BACT_TOWN_HISTORY:
        town_history(creature);
        return false;
    case BACT_RACE_LEGENDS:
        race_legends(creature);
        return false;
    case BACT_QUEST:
        castle_quest(creature);
        return false;
    case BACT_KING_LEGENDS:
    case BACT_ARENA_LEGENDS:
    case BACT_LEGENDS:
        show_highclass(creature);
        return false;
    case BACT_POSTER:
    case BACT_ARENA_RULES:
    case BACT_ARENA:
        return arena_comm(creature, building_action);
    case BACT_IN_BETWEEN:
    case BACT_CRAPS:
    case BACT_SPIN_WHEEL:
    case BACT_DICE_SLOTS:
    case BACT_GAMBLE_RULES:
    case BACT_POKER:
        gamble_comm(creature, building_action);
        return false;
    case BACT_REST:
    case BACT_RUMORS:
    case BACT_FOOD:
        if (inn_comm(creature, building_action)) {
            creature.sub_au(building_cost);
        }

        return false;
    case BACT_RESEARCH_MONSTER:
        if (research_mon(creature)) {
            creature.sub_au(building_cost);
        }

        return false;
    case BACT_COMPARE_WEAPONS:
        creature.sub_au(compare_weapons(creature, building_cost));
        return false;
    case BACT_ENCHANT_WEAPON:
        enchant_item(creature, building_cost, 1, 1, 0, FuncItemTester(&ItemEntity::allow_enchant_melee_weapon));
        return false;
    case BACT_ENCHANT_ARMOR:
        enchant_item(creature, building_cost, 0, 0, 1, FuncItemTester(&ItemEntity::is_protector));
        return false;
    case BACT_RECHARGE:
        building_recharge(creature);
        return false;
    case BACT_RECHARGE_ALL:
        building_recharge_all(creature);
        return false;
    case BACT_IDENTS:
        if (!input_check(_("持ち物を全て鑑定してよろしいですか？", "Do you pay to identify all your possession? "))) {
            return false;
        }

        identify_pack(creature);
        msg_print(_(" 持ち物全てが鑑定されました。", "Your possessions have been identified."));
        creature.sub_au(building_cost);
        return false;
    case BACT_IDENT_ONE:
        if (ident_spell(creature, false)) {
            creature.sub_au(building_cost);
        }

        return false;
    case BACT_LEARN:
        do_cmd_study(creature);
        return false;
    case BACT_HEALING:
        if (cure_critical_wounds(creature, 200)) {
            creature.sub_au(building_cost);
        }

        return false;
    case BACT_RESTORE:
        if (restore_all_status(creature)) {
            creature.sub_au(building_cost);
        }

        return false;
    case BACT_ENCHANT_ARROWS:
        enchant_item(creature, building_cost, 1, 1, 0, FuncItemTester(&ItemEntity::is_ammo));
        return false;
    case BACT_ENCHANT_BOW:
        enchant_item(creature, building_cost, 1, 1, 0, TvalItemTester(ItemKindType::BOW));
        return false;
    case BACT_RECALL:
        if (recall_player(creature, 1)) {
            creature.sub_au(building_cost);
        }

        return false;
    case BACT_TELEPORT_LEVEL:
        clear_bldg(4, 20);
        if (free_level_recall(creature)) {
            creature.sub_au(building_cost);
        }

        screen_load();
        return false;
    case BACT_LOSE_MUTATION: {
        auto muta = creature.muta;
        if (creature.ppersonality == PERSONALITY_LUCKY) {
            // ラッキーマンの白オーラは突然変異治療の対象外
            muta.reset(PlayerMutationType::GOOD_LUCK);
        }

        if (muta.any()) {
            while (!lose_mutation(creature, 0)) {
                ;
            }

            creature.sub_au(building_cost);
            return false;
        }

        msg_print(_("治すべき突然変異が無い。", "You have no mutations."));
        msg_erase();
        return false;
    }
    case BACT_MELEE_ARENA:
        return melee_arena_comm(creature);
    case BACT_TSUCHINOKO:
        tsuchinoko();
        return false;
    case BACT_BOUNTY:
        show_bounty();
        return false;
    case BACT_TARGET:
        today_target();
        return false;
    case BACT_KANKIN:
        exchange_cash(creature);
        return false;
    case BACT_HEIKOUKA:
        msg_print(_("平衡化の儀式を行なった。", "You received an equalization ritual."));
        set_virtue(creature, Virtue::COMPASSION, 0);
        set_virtue(creature, Virtue::HONOUR, 0);
        set_virtue(creature, Virtue::JUSTICE, 0);
        set_virtue(creature, Virtue::SACRIFICE, 0);
        set_virtue(creature, Virtue::KNOWLEDGE, 0);
        set_virtue(creature, Virtue::FAITH, 0);
        set_virtue(creature, Virtue::ENLIGHTEN, 0);
        set_virtue(creature, Virtue::ENCHANT, 0);
        set_virtue(creature, Virtue::CHANCE, 0);
        set_virtue(creature, Virtue::NATURE, 0);
        set_virtue(creature, Virtue::HARMONY, 0);
        set_virtue(creature, Virtue::VITALITY, 0);
        set_virtue(creature, Virtue::UNLIFE, 0);
        set_virtue(creature, Virtue::PATIENCE, 0);
        set_virtue(creature, Virtue::TEMPERANCE, 0);
        set_virtue(creature, Virtue::DILIGENCE, 0);
        set_virtue(creature, Virtue::VALOUR, 0);
        set_virtue(creature, Virtue::INDIVIDUALISM, 0);
        initialize_virtues(creature);
        creature.sub_au(building_cost);
        return false;
    case BACT_TELE_TOWN:
        if (!tele_town(creature)) {
            return false;
        }

        creature.sub_au(building_cost);
        return true;
    case BACT_EVAL_AC:
        if (eval_ac(creature.get_dis_ac() + creature.get_dis_to_a())) {
            creature.sub_au(building_cost);
        }

        return false;
    case BACT_BROKEN_WEAPON:
        creature.sub_au(repair_broken_weapon(creature, building_cost));
        return false;

    case BACT_TRANS_SEX:
        if (trans_sex(creature)) {
            creature.sub_au(building_cost);
        }
        return false;

    case BACT_SHOP_GENERAL:
        do_cmd_store(creature, StoreSaleType::GENERAL);
        world.character_icky_depth = 1;
        return false;
    case BACT_SHOP_ARMOURY:
        do_cmd_store(creature, StoreSaleType::ARMOURY);
        world.character_icky_depth = 1;
        return false;
    case BACT_SHOP_WEAPON:
        do_cmd_store(creature, StoreSaleType::WEAPON);
        world.character_icky_depth = 1;
        return false;
    case BACT_SHOP_TEMPLE:
        do_cmd_store(creature, StoreSaleType::TEMPLE);
        world.character_icky_depth = 1;
        return false;
    case BACT_SHOP_ALCHEMIST:
        do_cmd_store(creature, StoreSaleType::ALCHEMIST);
        world.character_icky_depth = 1;
        return false;
    case BACT_SHOP_MAGIC:
        do_cmd_store(creature, StoreSaleType::MAGIC);
        world.character_icky_depth = 1;
        return false;
    case BACT_SHOP_BLACK:
        do_cmd_store(creature, StoreSaleType::BLACK);
        world.character_icky_depth = 1;
        return false;
    case BACT_SHOP_HOME:
        do_cmd_store(creature, StoreSaleType::HOME);
        world.character_icky_depth = 1;
        return false;
    case BACT_SHOP_BOOK:
        do_cmd_store(creature, StoreSaleType::BOOK);
        world.character_icky_depth = 1;
        return false;
    case BACT_SHOP_MUSEUM:
        do_cmd_store(creature, StoreSaleType::MUSEUM);
        world.character_icky_depth = 1;
        return false;
    case BACT_SHOP_MELINLAITO_POTION:
        do_cmd_store(creature, StoreSaleType::MELINLAITO);
        world.character_icky_depth = 1;
        return false;
    case BACT_SHOP_HUNAHYANDA_WEAPON:
        do_cmd_store(creature, StoreSaleType::HUNAHYANDA);
        world.character_icky_depth = 1;
        return false;
    default:
        THROW_EXCEPTION(std::logic_error, "Invalid building action is specified!");
    }
}

/*!
 * @brief 施設入り口にプレイヤーが乗った際の処理 / Do building commands
 * @param creature クリーチャーへの参照
 */
void do_cmd_building(CreatureEntity &creature)
{
    if (AngbandWorld::get_instance().is_wild_mode()) {
        return;
    }

    PlayerEnergy energy(creature);
    energy.set_player_turn_energy(100);
    const auto p_pos = creature.get_position();
    auto &floor = *creature.get_floor();
    if (!floor.has_terrain_characteristics(p_pos, TerrainCharacteristics::BLDG)) {
        msg_print(_("ここには建物はない。", "You see no building here."));
        return;
    }

    const auto which = enum2i(floor.get_grid(p_pos).get_terrain().building_type);

    auto &bldg = buildings[which];
    reinit_wilderness = false;
    const auto &entries = ArenaEntryList::get_instance();
    if ((which == 2) && entries.get_defeated_entry() && !entries.is_player_true_victor()) {
        msg_print(_("「敗者に用はない。」", "'There's no place here for a LOSER like you!'"));
        return;
    }

    auto &fcms = FloorChangeModesStore::get_instace();
    auto &world = AngbandWorld::get_instance();
    if ((which == 2) && floor.inside_arena) {
        if (!world.get_arena() && floor.m_cnt > 0) {
            prt(_("ゲートは閉まっている。モンスターがあなたを待っている！", "The gates are closed.  The monster awaits!"), 0, 0);
        } else {
            fcms->set({ FloorChangeMode::SAVE_FLOORS, FloorChangeMode::NO_RETURN });
            floor.inside_arena = false;
            creature.leaving = true;
            command_new = SPECIAL_KEY_BUILDING;
            energy.reset_player_turn();
        }

        return;
    }

    TermCenteredOffsetSetter tcos(MAIN_TERM_MIN_COLS, MAIN_TERM_MIN_ROWS);

    auto &system = AngbandSystem::get_instance();
    if (system.is_phase_out()) {
        fcms->set({ FloorChangeMode::SAVE_FLOORS, FloorChangeMode::NO_RETURN });
        creature.leaving = true;
        system.set_phase_out(false);
        command_new = SPECIAL_KEY_BUILDING;
        energy.reset_player_turn();
        return;
    }

    creature.oldpy = creature.y;
    creature.oldpx = creature.x;
    floor.forget_lite();
    floor.forget_view();
    world.character_icky_depth++;

    command_arg = 0;
    command_rep = 0;
    command_new = 0;

    play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_BUILD);
    while (true) {
        display_building_service(creature, bldg);
        prt("", 1, 0);
        building_prt_gold(creature.get_au());
        const auto command = inkey();
        if (command == ESCAPE) {
            floor.inside_arena = false;
            system.set_phase_out(false);
            break;
        }

        auto is_valid_command = false;
        int i;
        for (i = 0; i < 8; i++) {
            if (bldg.letters[i] && (bldg.letters[i] == command)) {
                is_valid_command = true;
                break;
            }
        }

        const auto should_leave = is_valid_command ? bldg_process_command(creature, bldg, i) : false;
        handle_stuff(creature);
        if (should_leave) {
            break;
        }
    }

    select_floor_music(creature);

    msg_flag = false;
    msg_erase();

    if (reinit_wilderness) {
        creature.leaving = true;
    }

    world.character_icky_depth--;
    term_clear();

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::VIEW,
        StatusRecalculatingFlag::MONSTER_STATUSES,
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::LITE,
        StatusRecalculatingFlag::MONSTER_LITE,
    };
    rfu.set_flags(flags_srf);
    static constexpr auto flags_mwrf = {
        MainWindowRedrawingFlag::BASIC,
        MainWindowRedrawingFlag::EXTRA,
        MainWindowRedrawingFlag::EQUIPPY,
        MainWindowRedrawingFlag::MAP,
    };
    rfu.set_flags(flags_mwrf);
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::OVERHEAD,
        SubWindowRedrawingFlag::DUNGEON,
    };
    rfu.set_flags(flags_swrf);
}
