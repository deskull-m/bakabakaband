/*!
 * @brief キー入力に応じてゲーム内コマンドを実行する
 * @date 2022/02/20
 * @author Hourier
 */

#include "io/input-key-processor.h"
#include "autopick/autopick-pref-processor.h"
#include "cmd-action/cmd-hissatsu.h"
#include "cmd-action/cmd-mane.h"
#include "cmd-action/cmd-martial-arts.h"
#include "cmd-action/cmd-mind.h"
#include "cmd-action/cmd-move.h"
#include "cmd-action/cmd-open-close.h"
#include "cmd-action/cmd-others.h"
#include "cmd-action/cmd-pet.h"
#include "cmd-action/cmd-racial.h"
#include "cmd-action/cmd-shoot.h"
#include "cmd-action/cmd-spell.h"
#include "cmd-action/cmd-travel.h"
#include "cmd-action/cmd-tunnel.h"
#include "cmd-building/cmd-building.h"
#include "cmd-io/cmd-autopick.h"
#include "cmd-io/cmd-diary.h"
#include "cmd-io/cmd-dump.h"
#include "cmd-io/cmd-floor.h"
#include "cmd-io/cmd-gameoption.h"
#include "cmd-io/cmd-help.h"
#include "cmd-io/cmd-knowledge.h"
#include "cmd-io/cmd-lore.h"
#include "cmd-io/cmd-macro.h"
#include "cmd-io/cmd-process-screen.h"
#include "cmd-io/cmd-save.h"
#include "cmd-io/cmd-text-command.h"
#include "cmd-item/cmd-destroy.h"
#include "cmd-item/cmd-eat.h"
#include "cmd-item/cmd-equipment.h"
#include "cmd-item/cmd-item.h"
#include "cmd-item/cmd-magiceat.h"
#include "cmd-item/cmd-quaff.h"
#include "cmd-item/cmd-read.h"
#include "cmd-item/cmd-refill.h"
#include "cmd-item/cmd-throw.h"
#include "cmd-item/cmd-usestaff.h"
#include "cmd-item/cmd-zaprod.h"
#include "cmd-item/cmd-zapwand.h"
#include "cmd-visual/cmd-draw.h"
#include "cmd-visual/cmd-map.h"
#include "cmd-visual/cmd-visuals.h"
#include "core/asking-player.h"
#include "core/special-internal-keys.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/quest.h" //!< @do_cmd_quest() がある。後で移設する.
#include "effect/spells-effect-util.h"
#include "floor/wild.h"
#include "game-option/birth-options.h"
#include "game-option/disturbance-options.h"
#include "game-option/game-play-options.h"
#include "game-option/input-options.h"
#include "io-dump/random-art-info-dumper.h"
#include "io/command-repeater.h"
#include "io/files-util.h"
#include "io/input-key-requester.h" //!< @todo 相互依存している、後で何とかする.
#include "io/record-play-movie.h"
#include "io/write-diary.h"
#include "knowledge/knowledge-autopick.h"
#include "knowledge/knowledge-quests.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-blue-mage.h"
#include "mind/mind-elementalist.h"
#include "mind/mind-magic-eater.h"
#include "mind/mind-sniper.h"
#include "mind/mind-weaponsmith.h"
#include "mind/snipe-types.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player-info/samurai-data-type.h"
#include "player-info/sniper-data-type.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/digestion-processor.h"
#include "player/player-status.h"
#include "player/special-defense-types.h"
#include "status/action-setter.h"
#include "store/cmd-store.h"
#include "store/home.h"
#include "store/store-util.h"
#include "system/creature-entity.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor/floor-info.h"
#include "system/item-entity.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "window/display-sub-windows.h"
#include "wizard/cmd-wizard.h"
#include "wizard/wizard-mutation.h"
#include "world/world.h"
#include <string>
#include <tl/optional.hpp>

/*!
 * @brief ウィザードモードへの導入処理
 * / Verify use of "wizard" mode
 * @param floor フロアへの参照
 * @return 実際にウィザードモードへ移行したらTRUEを返す。
 */
bool enter_wizard_mode(const FloorType &floor)
{
    auto &world = AngbandWorld::get_instance();
    if (!world.noscore) {
        if (!allow_debug_opts) {
            msg_print(_("ウィザードモードは許可されていません。 ", "Wizard mode is not permitted."));
            return false;
        }

        msg_print(_("ウィザードモードはデバッグと実験のためのモードです。 ", "Wizard mode is for debugging and experimenting."));
        msg_print(_("一度ウィザードモードに入るとスコアは記録されません。", "The game will not be scored if you enter wizard mode."));
        msg_erase();
        if (!input_check(_("本当にウィザードモードに入りたいのですか? ", "Are you sure you want to enter wizard mode? "))) {
            return false;
        }

        constexpr auto mes = _("ウィザードモードに突入してスコアを残せなくなった。", "gave up recording score to enter wizard mode.");
        exe_write_diary(floor, DiaryKind::DESCRIPTION, 0, mes);
        world.noscore |= 0x0002;
    }

    return true;
}

/*!
 * @brief デバッグコマンドへの導入処理
 * / Verify use of "debug" commands
 * @param creature クリーチャーへの参照
 * @return 実際にデバッグコマンドへ移行したらTRUEを返す。
 */
static bool enter_debug_mode(const FloorType &floor)
{
    auto &world = AngbandWorld::get_instance();
    if (!world.noscore) {
        if (!allow_debug_opts) {
            msg_print(_("デバッグコマンドは許可されていません。 ", "Use of debug command is not permitted."));
            return false;
        }

        msg_print(_("デバッグ・コマンドはデバッグと実験のためのコマンドです。 ", "The debug commands are for debugging and experimenting."));
        msg_print(_("デバッグ・コマンドを使うとスコアは記録されません。", "The game will not be scored if you use debug commands."));
        msg_erase();
        if (!input_check(_("本当にデバッグ・コマンドを使いますか? ", "Are you sure you want to use debug commands? "))) {
            return false;
        }

        constexpr auto mes = _("デバッグモードに突入してスコアを残せなくなった。", "gave up sending score to use debug commands.");
        exe_write_diary(floor, DiaryKind::DESCRIPTION, 0, mes);
        world.noscore |= 0x0008;
    }

    return true;
}

/*!
 * @brief プレイヤーから受けた入力コマンドの分岐処理。
 * / Parse and execute the current command Give "Warning" on illegal commands.
 * @todo Make some "blocks"
 */
void process_command(CreatureEntity &creature)
{
    COMMAND_CODE old_now_message = now_message;
    repeat_check();
    now_message = 0;
    auto sniper_data = CreatureClass(creature).get_specific_data<SniperData>();
    if (sniper_data && sniper_data->concent > 0) {
        sniper_data->reset_concent = true;
    }

    auto &world = AngbandWorld::get_instance();
    const auto is_wild_mode = world.is_wild_mode();
    const auto &floor = *creature.get_floor();
    switch (command_cmd) {
    case ESCAPE:
    case ' ':
    case '\r':
    case '\n': {
        /* Ignore */
        break;
    }
    case KTRL('W'): {
        if (world.wizard) {
            world.wizard = false;
            msg_print(_("ウィザードモード解除。", "Wizard mode off."));
        } else if (enter_wizard_mode(floor)) {
            world.wizard = true;
            msg_print(_("ウィザードモード突入。", "Wizard mode on."));
        }

        auto &rfu = RedrawingFlagsUpdater::get_instance();
        rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
        rfu.set_flag(MainWindowRedrawingFlag::TITLE);
        break;
    }
    case KTRL('A'): {
        if (enter_debug_mode(floor)) {
            do_cmd_debug(creature);
        }

        break;
    }
    case KTRL('Y'): {
        if (enter_debug_mode(floor)) {
            wiz_mutation_menu(creature);
        }

        break;
    }
    case 'w': {
        if (!is_wild_mode) {
            do_cmd_wield(creature);
        }

        break;
    }
    case 't': {
        if (!is_wild_mode) {
            do_cmd_takeoff(creature);
        }

        break;
    }
    case 'd': {
        if (!is_wild_mode) {
            do_cmd_drop(creature);
        }

        break;
    }
    case 'k': {
        do_cmd_destroy(creature);
        break;
    }
    case 'e': {
        do_cmd_equip(creature);
        break;
    }
    case 'i': {
        do_cmd_inven(creature);
        break;
    }
    case 'I': {
        do_cmd_observe(creature);
        break;
    }

    case KTRL('I'): {
        toggle_inventory_equipment();
        break;
    }
    case '+': {
        if (!is_wild_mode) {
            do_cmd_alter(creature);
        }

        break;
    }
    case 'T': {
        if (!is_wild_mode) {
            do_cmd_tunnel(creature);
        }

        break;
    }
    case ';': {
        do_cmd_walk(creature, false);
        break;
    }
    case '-': {
        do_cmd_walk(creature, true);
        break;
    }
    case '.': {
        if (!is_wild_mode) {
            do_cmd_run(creature);
        }

        break;
    }
    case ',': {
        do_cmd_stay(creature, always_pickup);
        break;
    }
    case 'g': {
        do_cmd_stay(creature, !always_pickup);
        break;
    }
    case 'R': {
        do_cmd_rest(creature);
        break;
    }
    case 's': {
        do_cmd_search(creature);
        break;
    }
    case 'S': {
        if (creature.action == ACTION_SEARCH) {
            set_action(creature, ACTION_NONE);
        } else {
            set_action(creature, ACTION_SEARCH);
        }

        break;
    }
    case SPECIAL_KEY_STORE: {
        do_cmd_store(creature);
        break;
    }
    case SPECIAL_KEY_BUILDING: {
        do_cmd_building(creature);
        break;
    }
    case SPECIAL_KEY_QUEST: {
        do_cmd_quest(creature);
        break;
    }
    case '<': {
        if (!is_wild_mode && !floor.is_underground() && !floor.inside_arena && !floor.is_in_quest()) {
            if (vanilla_town) {
                break;
            }

            if (creature.get_ambush_flag()) {
                msg_print(_("襲撃から逃げるにはマップの端まで移動しなければならない。", "To flee the ambush you have to reach the edge of the map."));
                break;
            }

            if (creature.get_food() < PY_FOOD_WEAK) {
                msg_print(_("その前に食事をとらないと。", "You must eat something here."));
                break;
            }

            change_wild_mode(creature, false);
        } else {
            do_cmd_go_up(creature);
        }

        break;
    }
    case '>': {
        if (is_wild_mode) {
            change_wild_mode(creature, false);
        } else {
            do_cmd_go_down(creature);
        }

        break;
    }
    case 'o': {
        do_cmd_open(creature);
        break;
    }
    case 'c': {
        do_cmd_close(creature);
        break;
    }
    case 'j': {
        do_cmd_spike(creature);
        break;
    }
    case 'B': {
        do_cmd_bash(creature);
        break;
    }
    case 'D': {
        do_cmd_disarm(creature);
        break;
    }
    case 'G': {
        CreatureClass pc(creature);
        if (pc.is_every_magic() || pc.equals(PlayerClassType::ELEMENTALIST)) {
            msg_print(_("呪文を学習する必要はない！", "You don't have to learn spells!"));
        } else if (pc.equals(PlayerClassType::SAMURAI)) {
            do_cmd_gain_hissatsu(creature);
        } else if (pc.equals(PlayerClassType::MAGIC_EATER)) {
            import_magic_device(creature);
        } else {
            do_cmd_study(creature);
        }

        break;
    }
    case 'X': {
        do_cmd_martial_arts_style(creature);
        break;
    }
    case 'b': {
        CreatureClass pc(creature);
        if (pc.can_browse()) {
            do_cmd_mind_browse(creature);
        } else if (pc.equals(PlayerClassType::ELEMENTALIST)) {
            do_cmd_element_browse(creature);
        } else if (pc.equals(PlayerClassType::SMITH)) {
            do_cmd_kaji(creature, true);
        } else if (pc.equals(PlayerClassType::MAGIC_EATER)) {
            do_cmd_magic_eater(creature, true, false);
        } else if (pc.equals(PlayerClassType::SNIPER)) {
            do_cmd_snipe_browse(creature);
        } else {
            do_cmd_browse(creature);
        }

        break;
    }
    case 'm': {
        if (is_wild_mode) {
            break;
        }

        CreatureClass pc(creature);
        if (pc.equals(PlayerClassType::WARRIOR) || pc.equals(PlayerClassType::ARCHER) || pc.equals(PlayerClassType::CAVALRY)) {
            msg_print(_("呪文を唱えられない！", "You cannot cast spells!"));
            break;
        }

        const auto &dungeon = floor.get_dungeon_definition();
        auto non_magic_class = pc.equals(PlayerClassType::BERSERKER);
        non_magic_class |= pc.equals(PlayerClassType::SMITH);
        if (floor.is_underground() && dungeon.flags.has(DungeonFeatureType::NO_MAGIC) && !non_magic_class) {
            msg_print(_("ダンジョンが魔法を吸収した！", "The dungeon absorbs all attempted magic!"));
            msg_erase();
            break;
        }

        if (creature.has_anti_magic() && !non_magic_class) {
            concptr which_power = _("魔法", "magic");
            switch (creature.pclass) {
            case PlayerClassType::MINDCRAFTER:
                which_power = _("超能力", "psionic powers");
                break;
            case PlayerClassType::IMITATOR:
                which_power = _("ものまね", "imitation");
                break;
            case PlayerClassType::SAMURAI:
                which_power = _("必殺剣", "hissatsu");
                break;
            case PlayerClassType::MIRROR_MASTER:
                which_power = _("鏡魔法", "mirror magic");
                break;
            case PlayerClassType::NINJA:
                which_power = _("忍術", "ninjutsu");
                break;
            case PlayerClassType::ELEMENTALIST:
                which_power = _("元素魔法", "magic");
                break;
            default:
                if (mp_ptr->spell_book == ItemKindType::LIFE_BOOK) {
                    which_power = _("祈り", "prayer");
                }
                break;
            }

            msg_format(_("反魔法バリアが%sを邪魔した！", "An anti-magic shell disrupts your %s!"), which_power);
            PlayerEnergy(creature).reset_player_turn();
            break;
        }

        if (creature.is_shero() && !pc.equals(PlayerClassType::BERSERKER)) {
            msg_format(_("狂戦士化していて頭が回らない！", "You cannot think directly!"));
            PlayerEnergy(creature).reset_player_turn();
            break;
        }

        if (pc.can_browse()) {
            do_cmd_mind(creature);
        } else if (pc.equals(PlayerClassType::ELEMENTALIST)) {
            do_cmd_element(creature);
        } else if (pc.equals(PlayerClassType::IMITATOR)) {
            do_cmd_mane(creature, false);
        } else if (pc.equals(PlayerClassType::MAGIC_EATER)) {
            do_cmd_magic_eater(creature, false, false);
        } else if (pc.equals(PlayerClassType::SAMURAI)) {
            do_cmd_hissatsu(creature);
        } else if (pc.equals(PlayerClassType::BLUE_MAGE)) {
            do_cmd_cast_learned(creature);
        } else if (pc.equals(PlayerClassType::SMITH)) {
            do_cmd_kaji(creature, false);
        } else if (pc.equals(PlayerClassType::SNIPER)) {
            do_cmd_snipe(creature);
        } else {
            (void)do_cmd_cast(creature);
        }

        break;
    }
    case 'p': {
        do_cmd_pet(creature);
        break;
    }
    case '{': {
        do_cmd_inscribe(creature);
        break;
    }
    case '}': {
        do_cmd_uninscribe(creature);
        break;
    }
    case 'A': {
        do_cmd_activate(creature);
        break;
    }
    case 'E': {
        do_cmd_eat_food(creature);
        break;
    }
    case 'F': {
        do_cmd_refill(creature);
        break;
    }
    case 'f': {
        do_cmd_fire(creature, SP_NONE);
        break;
    }
    case 'v': {
        (void)ThrowCommand(creature).do_cmd_throw(1, false, -1);
        break;
    }
    case 'a': {
        do_cmd_aim_wand(creature);
        break;
    }
    case 'z': {
        if (use_command && rogue_like_commands) {
            do_cmd_use(creature);
        } else {
            do_cmd_zap_rod(creature);
        }

        break;
    }
    case 'q': {
        do_cmd_quaff_potion(creature);
        break;
    }
    case KTRL('Z'): {
        do_cmd_rectal_absorption(creature);
        break;
    }
    case 'r': {
        do_cmd_read_scroll(creature);
        break;
    }
    case 'u': {
        if (use_command && !rogue_like_commands) {
            do_cmd_use(creature);
        } else {
            do_cmd_use_staff(creature);
        }

        break;
    }
    case 'U': {
        do_cmd_racial_power(creature);
        break;
    }
    case 'M': {
        do_cmd_view_map(creature);
        break;
    }
    case 'L': {
        do_cmd_locate(creature);
        break;
    }
    case 'l': {
        do_cmd_look(creature);
        break;
    }
    case '*': {
        do_cmd_target(creature);
        break;
    }
    case '?': {
        do_cmd_help(creature);
        break;
    }
    case '/': {
        do_cmd_query_symbol(creature);
        break;
    }
    case 'C': {
        do_cmd_player_status(creature);
        break;
    }
    case '!':
        term_user();
        break;
    case '"': {
        do_cmd_pref(creature);
        break;
    }
    case '$': {
        do_cmd_reload_autopick(creature);
        break;
    }
    case '_': {
        do_cmd_edit_autopick(creature);
        break;
    }
    case '@': {
        do_cmd_macros(creature);
        break;
    }
    case '%': {
        do_cmd_visuals(creature);
        do_cmd_redraw(creature);
        break;
    }
    case '&': {
        do_cmd_colors(creature);
        do_cmd_redraw(creature);
        break;
    }
    case '=': {
        do_cmd_options(creature);
        (void)combine_and_reorder_home(creature, StoreSaleType::HOME);
        do_cmd_redraw(creature);
        break;
    }
    case ':': {
        do_cmd_note();
        break;
    }
    case 'V': {
        do_cmd_version();
        break;
    }
    case KTRL('F'): {
        do_cmd_feeling(creature);
        break;
    }
    case KTRL('O'): {
        do_cmd_message_one();
        break;
    }
    case KTRL('P'): {
        do_cmd_messages(old_now_message);
        break;
    }
    case KTRL('Q'): {
        do_cmd_checkquest(creature);
        break;
    }
    case KTRL('R'): {
        now_message = old_now_message;
        do_cmd_redraw(creature);
        break;
    }
    case KTRL('S'): {
        do_cmd_save_game(creature, false);
        break;
    }
    case KTRL('T'): {
        do_cmd_time(creature);
        break;
    }
    case KTRL('X'):
    case SPECIAL_KEY_QUIT: {
        do_cmd_save_and_exit(creature);
        break;
    }
    case 'Q': {
        do_cmd_suicide(creature);
        break;
    }
    case '|': {
        do_cmd_diary(creature);
        break;
    }
    case '~': {
        do_cmd_knowledge(creature);
        break;
    }
    case '(': {
        do_cmd_load_screen();
        break;
    }
    case ')': {
        do_cmd_save_screen(creature);
        break;
    }
    case ']': {
        prepare_movie_hooks(creature);
        break;
    }
    case KTRL('V'): {
        spoil_random_artifact(creature);
        break;
    }
    case KTRL('C'): {
        do_cmd_inscribe_terrain(creature);
        break;
    }
    case KTRL('E'): {
        do_cmd_text_command(creature);
        break;
    }
    case '`': {
        if (!is_wild_mode) {
            do_cmd_travel(creature);
        }
        CreatureClass(creature).break_samurai_stance({ SamuraiStanceType::MUSOU });

        break;
    }
    default: {
        if (flush_failure) {
            flush();
        }
        prt(_("存在しないコマンドです。'?' でヘルプが表示されます。", "Command not found. Type '?' for help."), 0, 0);

        break;
    }
    }

    if (!creature.energy_use && !now_message) {
        now_message = old_now_message;
    }
}
