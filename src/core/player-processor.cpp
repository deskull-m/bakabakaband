#include "core/player-processor.h"
#include "action/run-execution.h"
#include "action/travel-execution.h"
#include "bot/bot-json-output.h"
#include "core/disturbance.h"
#include "core/special-internal-keys.h"
#include "core/speed-table.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "floor/floor-save-util.h"
#include "floor/floor-util.h"
#include "floor/geometry.h"
#include "game-option/cheat-options.h"
#include "game-option/disturbance-options.h"
#include "game-option/map-screen-options.h"
#include "grid/grid.h"
#include "inventory/pack-overflow.h"
#include "io/cursor.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-processor.h"
#include "io/input-key-requester.h"
#include "mind/mind-force-trainer.h"
#include "mind/mind-sniper.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race-hook.h"
#include "monster/monster-describer.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-list.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "monster/monster-util.h"
#include "mutation/mutation-investor-remover.h"
#include "player-base/player-class.h"
#include "player-info/bluemage-data-type.h"
#include "player-info/mane-data-type.h"
#include "player-info/samurai-data-type.h"
#include "player-info/sniper-data-type.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/eldritch-horror.h"
#include "player/player-skill.h"
#include "player/special-defense-types.h"
#include "spell-kind/spells-random.h"
#include "spell-realm/spells-crusade.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-song.h"
#include "status/action-setter.h"
#include "system/angband-system.h"
#include "system/creature-entity.h"
#include "system/creature-timed-effect-types.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor/floor-info.h"
#include "system/floor/wilderness-grid.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "timed-effect/player-stun.h"
#include "tracking/health-bar-tracker.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "window/display-sub-windows.h"
#include "world/world-turn-processor.h"

bool load = true;
bool can_save = false;

static void process_fishing(CreatureEntity &creature)
{
    term_xtra(TERM_XTRA_DELAY, 10);
    if (one_in_(1000)) {
        bool success = false;
        get_mon_num_prep_enum(creature, MonraceHook::FISHING);
        const auto &floor = *creature.get_floor();
        const auto wild_level = WildernessGrids::get_instance().get_player_grid().get_level();
        const auto level = floor.is_underground() ? floor.dun_level : wild_level;
        const auto r_idx = get_mon_num(creature, 0, level, PM_NONE);
        msg_erase();
        if (MonraceList::is_valid(r_idx) && one_in_(2)) {
            const auto pos = creature.get_neighbor(creature.get_fishing_dir());
            if (auto m_idx = place_specific_monster(creature, pos.y, pos.x, r_idx, PM_NO_KAGE)) {
                const auto m_name = monster_desc(creature, floor.m_list[*m_idx], 0);
                msg_print(_(format("%sが釣れた！", m_name.data()), "You have a good catch!"));
                success = true;
            }
        }

        if (!success) {
            msg_print(_("餌だけ食われてしまった！くっそ～！", "Damn!  The fish stole your bait!"));
        }

        disturb(creature, false, true);
    }
}

bool continuous_action_running(CreatureEntity &creature)
{
    return creature.get_running() || Travel::get_instance().is_ongoing() || command_rep || (creature.get_action() == ACTION_REST) || (creature.get_action() == ACTION_FISH);
}

/*!
 * @brief プレイヤーの行動処理 / Process the player
 * @note
 * Notice the annoying code to handle "pack overflow", which\n
 * must come first just in case somebody manages to corrupt\n
 * the savefiles by clever use of menu commands or something.\n
 */
void process_player(CreatureEntity &creature)
{
    if (creature.is_hack_mutation()) {
        msg_print(_("何か変わった気がする！", "You feel different!"));
        (void)gain_mutation(creature, 0);
        creature.set_hack_mutation(false);
    }

    if (creature.is_invoking_midnight_curse()) {
        int count = 0;
        mark_monsters_present(creature);
        activate_ty_curse(creature, false, &count);
        creature.set_invoking_midnight_curse(false);
    }

    const auto &system = AngbandSystem::get_instance();
    if (system.is_phase_out()) {
        for (MONSTER_IDX m_idx = 1; m_idx < creature.get_floor()->m_max; m_idx++) {
            auto &monster = creature.get_floor()->m_list[m_idx];
            if (!monster.is_valid()) {
                continue;
            }

            monster.set_constant_flags({ MonsterConstantFlagType::MARK, MonsterConstantFlagType::SHOW });
            update_monster(creature, m_idx, false);
        }

        WorldTurnProcessor(creature).print_time();
        WorldTurnProcessor(creature).print_world_collapse();
        WorldTurnProcessor(creature).print_cheat_position();

    } else if (!(load && creature.get_energy_need() <= 0)) {
        creature.consume_energy_by_speed(creature.get_speed());
    }

    if (creature.get_energy_need() > 0) {
        return;
    }
    if (!command_rep) {
        WorldTurnProcessor(creature).print_time();
        WorldTurnProcessor(creature).print_world_collapse();
        WorldTurnProcessor(creature).print_cheat_position();
    }

    if (fresh_once && (continuous_action_running(creature) || !command_rep)) {
        stop_term_fresh();
    }

    if (creature.get_resting() < 0) {
        if (creature.get_resting() == COMMAND_ARG_REST_FULL_HEALING) {
            if ((creature.hp == creature.maxhp) && (creature.get_current_mp() >= creature.get_max_mp())) {
                set_action(creature, ACTION_NONE);
            }
        } else if (creature.get_resting() == COMMAND_ARG_REST_UNTIL_DONE) {
            if (creature.is_fully_healthy()) {
                set_action(creature, ACTION_NONE);
            }
        }
    }

    if (creature.get_action() == ACTION_FISH) {
        process_fishing(creature);
    }

    if (check_abort) {
        if (continuous_action_running(creature)) {
            inkey_scan = true;
            if (inkey()) {
                flush();
                disturb(creature, false, true);
                msg_print(_("中断しました。", "Canceled."));
            }
        }
    }

    if (creature.get_riding() && !creature.is_confused() && !creature.is_blind()) {
        const auto &monster = creature.get_floor()->m_list[creature.get_riding()];
        const auto &monrace = monster.get_monrace();
        if (monster.is_asleep()) {
            const auto m_name = monster_desc(creature, monster, 0);
            (void)set_monster_csleep(*creature.get_floor(), creature.get_riding(), 0);
            msg_format(_("%s^を起こした。", "You have woken %s up."), m_name.data());
        }

        if (monster.is_stunned()) {
            if (set_monster_stunned(*creature.get_floor(), creature.get_riding(),
                    (randint0(monrace.level) < creature.get_skill_exp(PlayerSkillKindType::RIDING)) ? 0 : (monster.get_remaining_stun() - 1))) {
                const auto m_name = monster_desc(creature, monster, 0);
                msg_format(_("%s^を朦朧状態から立ち直らせた。", "%s^ is no longer stunned."), m_name.data());
            }
        }

        if (monster.is_confused()) {
            if (set_monster_confused(*creature.get_floor(), creature.get_riding(),
                    (randint0(monrace.level) < creature.get_skill_exp(PlayerSkillKindType::RIDING)) ? 0 : (monster.get_remaining_confusion() - 1))) {
                const auto m_name = monster_desc(creature, monster, 0);
                msg_format(_("%s^を混乱状態から立ち直らせた。", "%s^ is no longer confused."), m_name.data());
            }
        }

        if (monster.is_fearful()) {
            if (set_monster_monfear(*creature.get_floor(), creature.get_riding(),
                    (randint0(monrace.level) < creature.get_skill_exp(PlayerSkillKindType::RIDING)) ? 0 : (monster.get_remaining_fear() - 1))) {
                const auto m_name = monster_desc(creature, monster, 0);
                msg_format(_("%s^を恐怖から立ち直らせた。", "%s^ is no longer fearful."), m_name.data());
            }
        }

        handle_stuff(creature);
    }

    load = false;
    if (creature.get_timed_effect(CreatureTimedEffect::LIGHTSPEED)) {
        set_lightspeed(creature, creature.get_timed_effect(CreatureTimedEffect::LIGHTSPEED) - 1, true);
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (CreatureClass(creature).equals(PlayerClassType::FORCETRAINER) && get_current_ki(creature)) {
        if (get_current_ki(creature) < 40) {
            set_current_ki(creature, true, 0);
        } else {
            set_current_ki(creature, false, -40);
        }
        rfu.set_flag(StatusRecalculatingFlag::BONUS);
    }

    if (creature.get_action() == ACTION_LEARN) {
        int32_t cost = 0L;
        uint32_t cost_frac = (creature.get_max_mp() + 30L) * 256L;
        s64b_lshift(&cost, &cost_frac, 16);
        if (s64b_cmp(creature.get_current_mp(), creature.current_mp_frac, cost, cost_frac) < 0) {
            creature.set_current_mp(0);
            creature.current_mp_frac = 0;
            set_action(creature, ACTION_NONE);
        } else {
            creature.sub_current_mp_with_frac(cost, cost_frac);
        }

        rfu.set_flag(MainWindowRedrawingFlag::MP);
    }

    if (CreatureClass(creature).samurai_stance_is(SamuraiStanceType::MUSOU)) {
        if (creature.get_current_mp() < 3) {
            set_action(creature, ACTION_NONE);
        } else {
            creature.sub_current_mp(2);
            rfu.set_flag(MainWindowRedrawingFlag::MP);
        }
    }

    /*** Handle actual user input ***/
    while (creature.get_energy_need() <= 0) {
        rfu.set_flag(SubWindowRedrawingFlag::PLAYER);
        creature.set_sutemi(false);
        creature.set_counter(false);
        creature.set_now_damaged(false);

        update_monsters(creature, false);
        handle_stuff(creature);
        move_cursor_relative(creature.y, creature.x);
        if (fresh_before) {
            term_fresh_force();
        }

        pack_overflow(creature);
        if (!command_new) {
            command_see = false;
        }

        PlayerEnergy energy(creature);
        energy.reset_player_turn();
        const auto is_knocked_out = PlayerStun::is_knocked_out(creature.get_timed_effect(CreatureTimedEffect::STUN));
        const auto is_paralyzed = creature.is_paralyzed();
        if (system.is_phase_out()) {
            move_cursor_relative(creature.y, creature.x);
            command_cmd = SPECIAL_KEY_BUILDING;
            process_command(creature);
        } else if ((is_paralyzed || is_knocked_out) && !cheat_immortal) {
            energy.set_player_turn_energy(100);
        } else if (creature.get_action() == ACTION_REST) {
            if (creature.get_resting() > 0) {
                creature.set_resting(creature.get_resting() - 1);
                if (!creature.get_resting()) {
                    set_action(creature, ACTION_NONE);
                }

                rfu.set_flag(MainWindowRedrawingFlag::ACTION);
            }

            energy.set_player_turn_energy(100);
        } else if (creature.get_action() == ACTION_FISH) {
            energy.set_player_turn_energy(100);
        } else if (creature.get_running()) {
            run_step(creature, Direction::none());
        } else if (auto &travel = Travel::get_instance(); travel.is_ongoing()) {
            travel.step(creature);
        } else if (command_rep) {
            command_rep--;
            rfu.set_flag(MainWindowRedrawingFlag::ACTION);
            handle_stuff(creature);
            msg_flag = false;
            prt("", 0, 0);
            mark_monsters_present(creature);
            process_command(creature);
        } else {
            move_cursor_relative(creature.y, creature.x);

            static constexpr auto flags = {
                SubWindowRedrawingFlag::SIGHT_MONSTERS,
                SubWindowRedrawingFlag::PETS,
            };
            rfu.set_flags(flags);
            window_stuff(creature);

            can_save = true;
            output_bot_json_snapshot(creature);
            InputKeyRequestor(creature, false).request_command();
            can_save = false;
            mark_monsters_present(creature);
            process_command(creature);
        }

        pack_overflow(creature);
        if (creature.get_energy_use()) {
            if (creature.is_timewalking() || creature.get_energy_use() > 400) {
                creature.add_energy_need(creature.get_energy_use() * TURNS_PER_TICK / 10);
            } else {
                creature.add_energy_need((int16_t)((int32_t)creature.get_energy_use() * ENERGY_NEED() / 100L));
            }

            if (creature.is_hallucinated()) {
                rfu.set_flag(MainWindowRedrawingFlag::MAP);
            }

            for (MONSTER_IDX m_idx = 1; m_idx < creature.get_floor()->m_max; m_idx++) {
                auto &monster = creature.get_floor()->m_list[m_idx];
                if (!monster.is_valid()) {
                    continue;
                }

                const auto &monrace = monster.get_apparent_monrace();

                // モンスターのシンボル/カラーの更新
                if (monster.is_visible_on_map() && monrace.visual_flags.has_any_of({ MonsterVisualType::MULTI_COLOR, MonsterVisualType::SHAPECHANGER })) {
                    lite_spot(creature, monster.get_position());
                }

                // 出現して即魔法を使わないようにするフラグを落とす処理
                if (monster.has_temporary_flag(MonsterTemporaryFlagType::PREVENT_MAGIC)) {
                    monster.reset_temporary_flag(MonsterTemporaryFlagType::PREVENT_MAGIC);
                }

                if (monster.has_temporary_flag(MonsterTemporaryFlagType::SANITY_BLAST)) {
                    monster.reset_temporary_flag(MonsterTemporaryFlagType::SANITY_BLAST);
                    sanity_blast(creature, m_idx);
                }

                // 感知中のモンスターのフラグを落とす処理
                // 感知したターンはMFLAG2_SHOWを落とし、次のターンに感知中フラグのMFLAG2_MARKを落とす
                if (monster.has_constant_flag(MonsterConstantFlagType::MARK)) {
                    if (monster.has_constant_flag(MonsterConstantFlagType::SHOW)) {
                        monster.reset_constant_flag(MonsterConstantFlagType::SHOW);
                    } else {
                        monster.reset_constant_flag(MonsterConstantFlagType::MARK);
                        monster.set_visible_on_map(false);
                        update_monster(creature, m_idx, false);
                        HealthBarTracker::get_instance().set_flag_if_tracking(m_idx);
                        if (monster.is_riding()) {
                            rfu.set_flag(MainWindowRedrawingFlag::UHEALTH);
                        }

                        lite_spot(creature, monster.get_position());
                    }
                }
            }

            if (CreatureClass(creature).equals(PlayerClassType::IMITATOR)) {
                auto mane_data = CreatureClass(creature).get_specific_data<mane_data_type>();
                if (static_cast<int>(mane_data->mane_list.size()) > (creature.get_level() > 44 ? 3 : creature.get_level() > 29 ? 2
                                                                                                                               : 1)) {
                    mane_data->mane_list.pop_front();
                }

                mane_data->new_mane = false;
                rfu.set_flag(MainWindowRedrawingFlag::IMITATION);
            }

            if (creature.get_action() == ACTION_LEARN) {
                auto mane_data = CreatureClass(creature).get_specific_data<bluemage_data_type>();
                mane_data->new_magic_learned = false;
                rfu.set_flag(MainWindowRedrawingFlag::ACTION);
            }

            if (creature.is_timewalking() && (creature.get_energy_need() > -1000)) {
                rfu.set_flag(MainWindowRedrawingFlag::MAP);
                rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
                static constexpr auto flags_swrf = {
                    SubWindowRedrawingFlag::OVERHEAD,
                    SubWindowRedrawingFlag::DUNGEON,
                };
                rfu.set_flags(flags_swrf);
                msg_print(_("「時は動きだす…」", "You feel time flowing around you once more."));
                msg_erase();
                creature.set_timewalking(false);
                creature.set_energy_need(ENERGY_NEED());

                handle_stuff(creature);
            }
        }

        if (!creature.is_playing() || creature.is_dead()) {
            creature.set_timewalking(false);
            break;
        }

        auto sniper_data = CreatureClass(creature).get_specific_data<SniperData>();
        if (creature.get_energy_use() && sniper_data && sniper_data->reset_concent) {
            reset_concentration(creature, true);
        }

        if (creature.is_leaving()) {
            break;
        }
    }

    update_smell(*creature.get_floor(), creature.get_position());
}

/*!
 * @brief プレイヤーの行動エネルギーが充填される（＝プレイヤーのターンが回る）毎に行われる処理  / process the effects per 100 energy at player speed.
 */
void process_upkeep_with_speed(CreatureEntity &creature)
{
    if (!load && creature.get_enchant_energy_need() > 0 && !creature.is_leaving()) {
        creature.sub_enchant_energy_need(speed_to_energy(static_cast<byte>(creature.get_speed())));
    }

    if (creature.get_enchant_energy_need() > 0) {
        return;
    }

    while (creature.get_enchant_energy_need() <= 0) {
        if (!load) {
            check_music(creature);
        }

        if (!load) {
            check_emission(creature);
        }

        if (!load) {
            check_demigod(creature);
        }

        SpellHex spell_hex(creature);
        if (!load) {
            spell_hex.decrease_mana();
        }

        if (!load) {
            spell_hex.continue_revenge();
        }

        creature.add_enchant_energy_need(ENERGY_NEED());
    }
}
