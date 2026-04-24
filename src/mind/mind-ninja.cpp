#include "mind/mind-ninja.h"
#include "cmd-action/cmd-attack.h"
#include "cmd-item/cmd-throw.h"
#include "core/disturbance.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "effect/spells-effect-util.h"
#include "floor/floor-object.h"
#include "floor/floor-util.h"
#include "game-option/disturbance-options.h"
#include "grid/grid.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-mirror-master.h"
#include "mind/mind-numbers.h"
#include "mind/mind-warrior.h"
#include "monster/monster-describer.h"
#include "monster/monster-update.h"
#include "player-attack/player-attack.h"
#include "player-base/player-class.h"
#include "player-info/equipment-info.h"
#include "player-info/ninja-data-type.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/player-status-flags.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-fetcher.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-grid.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-teleport.h"
#include "spell/spells-status.h"
#include "status/action-setter.h"
#include "status/body-improvement.h"
#include "status/element-resistance.h"
#include "status/temporary-resistance.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain/terrain-definition.h"
#include "target/projection-path-calculator.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"

/*!
 * @brief 変わり身処理
 * @param creature クリーチャーへの参照
 * @param success 判定成功上の処理ならばTRUE
 * @return 作用が実際にあった場合TRUEを返す
 */
bool kawarimi(CreatureEntity &creature, bool success)
{
    auto ninja_data = CreatureClass(creature).get_specific_data<ninja_data_type>();
    if (!ninja_data || !ninja_data->kawarimi) {
        return false;
    }

    if (creature.is_dead()) {
        return false;
    }

    const auto effects = creature.effects();
    const auto is_confused = effects->confusion().is_confused();
    const auto is_blind = effects->blindness().is_blind();
    const auto is_hallucinated = effects->hallucination().is_hallucinated();
    const auto is_paralyzed = effects->paralysis().is_paralyzed();
    if (is_confused || is_blind || is_paralyzed || is_hallucinated) {
        return false;
    }

    if (effects->stun().current() > randint0(200)) {
        return false;
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (!success && one_in_(3)) {
        msg_print(_("変わり身失敗！逃げられなかった。", "Kawarimi failed! You couldn't run away."));
        ninja_data->kawarimi = false;
        rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
        return false;
    }

    const auto p_pos_orig = creature.get_position(); //!< @details 元の位置に変わり身を置く.
    teleport_player(creature, 10 + randint1(90), TELEPORT_SPONTANEOUS);
    constexpr auto sv_wooden_statue = 0;
    ItemEntity item({ ItemKindType::STATUE, sv_wooden_statue });
    item.pval = enum2i(MonraceId::NINJA);
    (void)drop_near(creature, item, p_pos_orig);

    if (success) {
        msg_print(_("攻撃を受ける前に素早く身をひるがえした。", "You have turned around just before the attack hit you."));
    } else {
        msg_print(_("変わり身失敗！攻撃を受けてしまった。", "Kawarimi failed! You are hit by the attack."));
    }

    ninja_data->kawarimi = false;
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    return true;
}

/*!
 * @brief 入身処理 / "Rush Attack" routine for Samurai or Ninja
 * @param creature クリーチャーへの参照
 * @param mdeath 目標モンスターが死亡したかを返す
 * @return 作用が実際にあった場合TRUEを返す /  Return value is for checking "done"
 */
bool rush_attack(CreatureEntity &creature, bool *mdeath)
{
    if (mdeath) {
        *mdeath = false;
    }

    project_length = 5;
    const auto dir = get_aim_dir(creature);
    if (!dir) {
        return false;
    }

    const auto p_pos = creature.get_position();
    const auto pos_target = dir.get_target_position(p_pos, project_length);

    auto tm_idx = 0;
    auto &floor = *creature.get_floor();
    if (floor.contains(pos_target, FloorBoundary::OUTER_WALL_EXCLUSIVE)) {
        tm_idx = floor.get_grid(pos_target).m_idx;
    }

    ProjectionPath path_g(floor, project_length, p_pos, p_pos, pos_target, PROJECT_STOP | PROJECT_KILL);
    project_length = 0;
    if (path_g.path_num() == 0) {
        return true;
    }

    auto p_pos_new = p_pos;
    auto tmp_mdeath = false;
    auto moved = false;
    for (const auto &pos : path_g) {
        const auto &grid_new = floor.get_grid(pos);
        if (floor.is_empty_at(pos) && (pos != p_pos) && player_can_enter(creature, grid_new.feat, 0)) {
            p_pos_new = pos;
            continue;
        }

        if (!grid_new.has_monster()) {
            const auto *mes = tm_idx > 0 ? _("失敗！", "Failed!") : _("ここには入身では入れない。", "You can't move to that place.");
            msg_print(mes);
            break;
        }

        if (!creature.is_located_at(p_pos_new)) {
            teleport_player_to(creature, p_pos_new.y, p_pos_new.x, TELEPORT_NONMAGICAL);
        }

        update_monster(creature, grid_new.m_idx, true);
        const auto &monster = floor.get_monster(grid_new.m_idx);
        if (tm_idx != grid_new.m_idx) {
#ifdef JP
            msg_format("%s%sが立ちふさがっている！", tm_idx > 0 ? "別の" : "", monster.get_monster_profile().ml ? "モンスター" : "何か");
#else
            msg_format("There is %s in the way!", monster.get_monster_profile().ml ? (tm_idx > 0 ? "another monster" : "a monster") : "someone");
#endif
        } else if (!creature.is_located_at(p_pos_new)) {
            const auto m_name = monster_desc(creature, monster, 0);
            msg_format(_("素早く%sの懐に入り込んだ！", "You quickly jump in and attack %s!"), m_name.data());
        }

        if (!creature.is_located_at(p_pos_new)) {
            teleport_player_to(creature, p_pos_new.y, p_pos_new.x, TELEPORT_NONMAGICAL);
        }

        moved = true;
        tmp_mdeath = do_cmd_attack(creature, pos.y, pos.x, HISSATSU_NYUSIN);
        break;
    }

    if (!moved && !creature.is_located_at(p_pos_new)) {
        teleport_player_to(creature, p_pos_new.y, p_pos_new.x, TELEPORT_NONMAGICAL);
    }

    if (mdeath) {
        *mdeath = tmp_mdeath;
    }

    return true;
}

/*!
 * @brief 盗賊と忍者における不意打ち
 * @param creature クリーチャーへの参照
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 */
void process_surprise_attack(CreatureEntity &creature, player_attack_type *pa_ptr)
{

    const auto &monrace = pa_ptr->m_ptr->get_monrace();
    if (!has_melee_weapon(creature, enum2i(INVEN_MAIN_HAND) + pa_ptr->hand) || creature.is_icky_wield[pa_ptr->hand]) {
        return;
    }

    int tmp = creature.level * 6 + (creature.skill_stl + 10) * 4;
    if (creature.monlite && (pa_ptr->mode != HISSATSU_NYUSIN)) {
        tmp /= 3;
    }
    if (has_aggravate(creature)) {
        tmp /= 2;
    }
    if (monrace.level > (creature.level * creature.level / 20 + 10)) {
        tmp /= 3;
    }

    auto ninja_data = CreatureClass(creature).get_specific_data<ninja_data_type>();
    if (pa_ptr->m_ptr->is_asleep() && pa_ptr->m_ptr->get_monster_profile().ml) {
        /* Can't backstab creatures that we can't see, right? */
        pa_ptr->backstab = true;
    } else if ((ninja_data && ninja_data->s_stealth) && (randint0(tmp) > (monrace.level + 20)) &&
               pa_ptr->m_ptr->get_monster_profile().ml && !monrace.resistance_flags.has(MonsterResistanceType::RESIST_ALL)) {
        pa_ptr->surprise_attack = true;
    } else if (pa_ptr->m_ptr->is_fearful() && pa_ptr->m_ptr->get_monster_profile().ml) {
        pa_ptr->stab_fleeing = true;
    }
}

void print_surprise_attack(player_attack_type *pa_ptr)
{
    if (pa_ptr->backstab) {
        msg_format(_("あなたは冷酷にも眠っている無力な%sを突き刺した！", "You cruelly stab the helpless, sleeping %s!"), pa_ptr->m_name);
        sound(SoundKind::BACKSTAB_HIT);
    } else if (pa_ptr->surprise_attack) {
        msg_format(_("不意を突いて%sに強烈な一撃を喰らわせた！", "You make surprise attack, and hit %s with a powerful blow!"), pa_ptr->m_name);
        sound(SoundKind::SURPRISE_HIT);
    } else if (pa_ptr->stab_fleeing) {
        msg_format(_("逃げる%sを背中から突き刺した！", "You backstab the fleeing %s!"), pa_ptr->m_name);
        sound(SoundKind::FLEEING_HIT);
    } else if (!pa_ptr->monk_attack) {
        msg_format(_("%sを攻撃した。", "You hit %s."), pa_ptr->m_name);
    }
}

/*!
 * @brief 盗賊と忍者における不意打ちのダメージ計算
 * @param creature クリーチャーへの参照
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 */
void calc_surprise_attack_damage(CreatureEntity &creature, player_attack_type *pa_ptr)
{
    if (pa_ptr->backstab) {
        pa_ptr->attack_damage *= (3 + (creature.level / 20));
        return;
    }

    if (pa_ptr->surprise_attack) {
        pa_ptr->attack_damage = pa_ptr->attack_damage * (5 + (creature.level * 2 / 25)) / 2;
        return;
    }

    if (pa_ptr->stab_fleeing) {
        pa_ptr->attack_damage = (3 * pa_ptr->attack_damage) / 2;
    }
}

/*!
 * @brief 速駆け処理
 * @param creature クリーチャーへの参照
 * @return 常にTRUE
 */
bool hayagake(CreatureEntity &creature)
{
    PlayerEnergy energy(creature);
    if (creature.action == ACTION_HAYAGAKE) {
        set_action(creature, ACTION_NONE);
        energy.reset_player_turn();
        return true;
    }

    const auto &grid = creature.get_floor()->get_grid(creature.get_position());
    const auto &terrain = grid.get_terrain();
    if (terrain.flags.has_not(TerrainCharacteristics::PROJECTION) || (!creature.has_levitation() && terrain.flags.has(TerrainCharacteristics::DEEP))) {
        msg_print(_("ここでは素早く動けない。", "You cannot run in here."));
    } else {
        set_action(creature, ACTION_HAYAGAKE);
    }

    energy.reset_player_turn();
    return true;
}

/*!
 * @brief 超隠密状態をセットする
 * @param creature クリーチャーへの参照
 * @param set TRUEならば超隠密状態になる。
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_superstealth(CreatureEntity &creature, bool set)
{
    bool notice = false;

    auto ninja_data = CreatureClass(creature).get_specific_data<ninja_data_type>();
    if (!ninja_data || creature.is_dead()) {
        return false;
    }

    if (set) {
        if (!ninja_data->s_stealth) {
            if (creature.get_floor()->grid_array[creature.y][creature.x].info & CAVE_MNLT) {
                msg_print(_("敵の目から薄い影の中に覆い隠された。", "You are mantled in weak shadow from ordinary eyes."));
                creature.monlite = creature.old_monlite = true;
            } else {
                msg_print(_("敵の目から影の中に覆い隠された！", "You are mantled in shadow from ordinary eyes!"));
                creature.monlite = creature.old_monlite = false;
            }

            notice = true;
            ninja_data->s_stealth = true;
        }
    } else {
        if (ninja_data->s_stealth) {
            msg_print(_("再び敵の目にさらされるようになった。", "You are exposed to common sight once more."));
            notice = true;
            ninja_data->s_stealth = false;
        }
    }

    if (!notice) {
        return false;
    }

    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (disturb_state) {
        disturb(creature, false, false);
    }

    return true;
}

/*!
 * @brief 忍術の発動 /
 * do_cmd_cast calls this function if the creature's class is 'ninja'.
 * @param creature クリーチャーへの参照
 * @param spell 発動する特殊技能のID
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
bool cast_ninja_spell(CreatureEntity &creature, MindNinjaType spell)
{
    PLAYER_LEVEL plev = creature.level;
    auto ninja_data = CreatureClass(creature).get_specific_data<ninja_data_type>();
    switch (spell) {
    case MindNinjaType::DARKNESS_CREATION:
        (void)unlite_area(creature, 0, 3);
        break;
    case MindNinjaType::DETECT_NEAR:
        if (plev > 44) {
            wiz_lite(creature, true);
        }

        detect_monsters_normal(creature, DETECT_RAD_DEFAULT);
        if (plev > 4) {
            detect_traps(creature, DETECT_RAD_DEFAULT, true);
            detect_doors(creature, DETECT_RAD_DEFAULT);
            detect_stairs(creature, DETECT_RAD_DEFAULT);
        }

        if (plev > 14) {
            detect_objects_normal(creature, DETECT_RAD_DEFAULT);
        }

        break;
    case MindNinjaType::HIDE_LEAVES:
        teleport_player(creature, 10, TELEPORT_SPONTANEOUS);
        break;
    case MindNinjaType::KAWARIMI:
        if (ninja_data && !ninja_data->kawarimi) {
            msg_print(_("敵の攻撃に対して敏感になった。", "You are now prepared to evade any attacks."));
            ninja_data->kawarimi = true;
            RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
        }

        break;
    case MindNinjaType::ABSCONDING:
        teleport_player(creature, plev * 5, TELEPORT_SPONTANEOUS);
        break;
    case MindNinjaType::HIT_AND_AWAY:
        if (!hit_and_away(creature)) {
            return false;
        }

        break;
    case MindNinjaType::BIND_MONSTER: {
        const auto dir = get_aim_dir(creature);
        if (!dir) {
            return false;
        }

        (void)stasis_monster(creature, dir);
        break;
    }
    case MindNinjaType::ANCIENT_KNOWLEDGE:
        return ident_spell(creature, false);
    case MindNinjaType::FLOATING:
        set_tim_levitation(creature, randint1(20) + 20, false);
        break;
    case MindNinjaType::HIDE_FLAMES:
        fire_ball(creature, AttributeType::FIRE, Direction::self(), 50 + plev, plev / 10 + 2);
        teleport_player(creature, 30, TELEPORT_SPONTANEOUS);
        set_oppose_fire(creature, (TIME_EFFECT)plev, false);
        break;
    case MindNinjaType::NYUSIN:
        return rush_attack(creature, nullptr);
    case MindNinjaType::SYURIKEN_SPREADING: {
        for (int i = 0; i < 8; i++) {
            OBJECT_IDX slot;

            for (slot = 0; slot < INVEN_PACK; slot++) {
                if (creature.inventory[slot]->bi_key.tval() == ItemKindType::SPIKE) {
                    break;
                }
            }

            if (slot == INVEN_PACK) {
                if (!i) {
                    msg_print(_("くさびを持っていない。", "You have no Iron Spikes."));
                } else {
                    msg_print(_("くさびがなくなった。", "You have no more Iron Spikes."));
                }

                return false;
            }

            (void)ThrowCommand(creature).do_cmd_throw(1, false, slot);
            PlayerEnergy(creature).set_player_turn_energy(100);
        }

        break;
    }
    case MindNinjaType::CHAIN_HOOK:
        (void)fetch_monster(creature);
        break;
    case MindNinjaType::SMOKE_BALL: {
        const auto dir = get_aim_dir(creature);
        if (!dir) {
            return false;
        }

        fire_ball(creature, AttributeType::OLD_CONF, dir, plev * 3, 3);
        break;
    }
    case MindNinjaType::SWAP_POSITION: {
        project_length = -1;
        const auto dir = get_aim_dir(creature);
        if (!dir) {
            project_length = 0;
            return false;
        }

        project_length = 0;
        (void)teleport_swap(creature, dir);
        break;
    }
    case MindNinjaType::EXPLOSIVE_RUNE:
        create_rune_explosion(creature, creature.y, creature.x);
        break;
    case MindNinjaType::HIDE_MUD:
        (void)set_pass_wall(creature, randint1(plev / 2) + plev / 2, false);
        set_oppose_acid(creature, (TIME_EFFECT)plev, false);
        break;
    case MindNinjaType::HIDE_MIST:
        fire_ball(creature, AttributeType::POIS, Direction::self(), 75 + plev * 2 / 3, plev / 5 + 2);
        fire_ball(creature, AttributeType::HYPODYNAMIA, Direction::self(), 75 + plev * 2 / 3, plev / 5 + 2);
        fire_ball(creature, AttributeType::CONFUSION, Direction::self(), 75 + plev * 2 / 3, plev / 5 + 2);
        teleport_player(creature, 30, TELEPORT_SPONTANEOUS);
        break;
    case MindNinjaType::PURGATORY_FLAME: {
        const auto num = Dice::roll(3, 9);
        for (auto k = 0; k < num; k++) {
            const auto type = one_in_(2) ? AttributeType::FIRE : one_in_(3) ? AttributeType::NETHER
                                                                            : AttributeType::PLASMA;
            auto attempts = 1000;
            Pos2D pos(0, 0);
            while (attempts--) {
                pos = scatter(*creature.get_floor(), creature.get_position(), 4, PROJECT_NONE);
                if (!creature.is_located_at(pos)) {
                    break;
                }
            }

            const uint32_t flags = PROJECT_BEAM | PROJECT_THRU | PROJECT_GRID | PROJECT_KILL;
            project(creature, 0, 0, pos.y, pos.x, Dice::roll(6 + plev / 8, 10), type, flags);
        }

        break;
    }
    case MindNinjaType::ALTER_EGO:
        set_multishadow(creature, 6 + randint1(6), false);
        break;
    default:
        msg_print(_("なに？", "Zap?"));
        break;
    }
    return true;
}
