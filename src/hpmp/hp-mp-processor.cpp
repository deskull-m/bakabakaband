#include "hpmp/hp-mp-processor.h"
#include "avatar/avatar.h"
#include "cmd-action/cmd-pet.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/pattern-walk.h"
#include "grid/grid.h"
#include "hpmp/hp-mp-regenerator.h"
#include "inventory/inventory-slot-types.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object/tval-types.h"
#include "pet/pet-util.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/monk-data-type.h"
#include "player-info/race-info.h"
#include "player-info/race-types.h"
#include "player-info/samurai-data-type.h"
#include "player/attack-defense-types.h"
#include "player/digestion-processor.h"
#include "player/player-damage.h"
#include "player/player-status-flags.h"
#include "player/player-status-resist.h"
#include "player/player-status.h"
#include "player/special-defense-types.h"
#include "status/bad-status-setter.h"
#include "status/element-resistance.h"
#include "system/creature-entity.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain/terrain-definition.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <functional>
#include <sstream>

/*!
 * @brief 地形によるダメージを与える / Deal damage from feature.
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param grid 現在の床の情報への参照
 * @param msg_levitation 浮遊時にダメージを受けた場合に表示するメッセージ
 * @param msg_normal 通常時にダメージを受けた場合に表示するメッセージの述部
 * @param 耐性等によるダメージレートを計算する関数
 * @param ダメージを受けた際の追加処理を行う関数
 * @return ダメージを与えたらTRUE、なければFALSE
 * @details
 * ダメージを受けた場合、自然回復できない。
 */
static bool deal_damege_by_feat(CreatureEntity &creature, const Grid &grid, concptr msg_levitation, concptr msg_normal,
    std::function<PERCENTAGE(CreatureEntity &)> damage_rate, std::function<void(CreatureEntity &, int)> additional_effect)
{
    const auto &terrain = grid.get_terrain();
    auto damage = 0;
    if (terrain.flags.has(TerrainCharacteristics::CHAOS_TAINTED) || terrain.flags.has(TerrainCharacteristics::PLASMA)) {
        damage = 12000 + randint0(8000);
    } else if (terrain.flags.has(TerrainCharacteristics::VOID)) {
        damage = 18000 + randint0(12000);
    } else if (terrain.flags.has(TerrainCharacteristics::DEEP)) {
        damage = 6000 + randint0(4000);
    } else if (!creature.levitation) {
        damage = 3000 + randint0(2000);
    }

    damage *= damage_rate(creature);
    damage /= 100;
    if (creature.levitation) {
        damage /= 5;
    }

    damage = damage / 100 + evaluate_percent(damage % 100);

    if (damage == 0) {
        return false;
    }

    if (creature.levitation) {
        msg_print(msg_levitation);
        constexpr auto mes = _("%%s\u306e\u4e0a\u306b\u6d6e\u904a\u3057\u305f\u30c0\u30e1\u30fc\u30b8", "flying over %%s");
        take_hit(creature, DAMAGE_NOESCAPE, damage, format(mes, grid.get_terrain(TerrainKind::MIMIC).name.data()));

        if (additional_effect != nullptr) {
            additional_effect(creature, damage);
        }
    } else {
        const auto p_pos = creature.get_position();
        const auto &name = creature.current_floor_ptr->get_grid(p_pos).get_terrain(TerrainKind::MIMIC).name;
        msg_format(_("%%s%%s\uff01", "The %%s %%s!"), name.data(), msg_normal);
        take_hit(creature, DAMAGE_NOESCAPE, damage, name);

        if (additional_effect != nullptr) {
            additional_effect(creature, damage);
        }
    }

    return true;
}

/*!
 * @brief 10ゲームターンが進行するごとにプレイヤーのHPとMPの増減処理を行う。
 *  / Handle timed damage and regeneration every 10 game turns
 */
void process_player_hp_mp(CreatureEntity &creature)
{
    const auto &floor = *creature.current_floor_ptr;
    const auto &grid = floor.get_grid(creature.get_position());
    const auto &terrain = grid.get_terrain();
    bool cave_no_regen = false;
    int upkeep_factor = 0;
    int regen_amount = PY_REGEN_NORMAL;
    const auto effects = creature.effects();
    if (terrain.flags.has(TerrainCharacteristics::RUNE_HEALING)) {
        hp_player(creature, 2 + creature.level / 6);
    }

    const auto &player_poison = effects->poison();
    if (player_poison.is_poisoned() && !creature.is_invulnerable()) {
        if (take_hit(creature, DAMAGE_NOESCAPE, 1, _("毒", "poison")) > 0) {
            sound(SoundKind::DAMAGE_OVER_TIME);
        }
    }

    const auto &player_cut = effects->cut();
    if (player_cut.is_cut() && !creature.is_invulnerable()) {
        const auto dam = player_cut.get_damage();
        if (take_hit(creature, DAMAGE_NOESCAPE, dam, _("致命傷", "a mortal wound")) > 0) {
            sound(SoundKind::DAMAGE_OVER_TIME);
        }
    }

    const CreatureRace race(&creature);
    if (race.life() == PlayerRaceLifeType::UNDEAD && race.tr_flags().has(TR_VUL_LITE)) {
        if (!floor.is_underground() && !has_resist_lite(creature) && !creature.is_invulnerable() && AngbandWorld::get_instance().is_daytime()) {
            if ((floor.grid_array[creature.y][creature.x].info & (CAVE_GLOW | CAVE_MNDK)) == CAVE_GLOW) {
                msg_print(_("日光があなたのアンデッドの肉体を焼き焦がした！", "The sun's rays scorch your undead flesh!"));
                take_hit(creature, DAMAGE_NOESCAPE, 1, _("日光", "sunlight"));
                cave_no_regen = true;
            }
        }

        const auto &item = *creature.inventory[INVEN_LITE];
        const auto flags = item.get_flags();
        if ((creature.inventory[INVEN_LITE]->bi_key.tval() != ItemKindType::NONE) && flags.has_not(TR_DARK_SOURCE) && !has_resist_lite(creature)) {
            const auto item_name = describe_flavor(creature, item, (OD_OMIT_PREFIX | OD_NAME_ONLY));
            msg_format(_("%sがあなたのアンデッドの肉体を焼き焦がした！", "The %s scorches your undead flesh!"), item_name.data());
            cave_no_regen = true;
            if (!creature.is_invulnerable()) {
                const auto wielding_item_name = describe_flavor(creature, item, OD_NAME_ONLY);
                std::stringstream ss;
                ss << _(wielding_item_name, "wielding ") << _("を装備したダメージ", wielding_item_name);
                take_hit(creature, DAMAGE_NOESCAPE, 1, ss.str());
            }
        }
    }

    if (terrain.flags.has(TerrainCharacteristics::LAVA) && !creature.is_invulnerable() && !has_immune_fire(creature)) {
        constexpr auto mes_leviation = _("熱で火傷した！", "The heat burns you!");
        constexpr auto mes_normal = _("で火傷した！", "burns you!");
        if (deal_damege_by_feat(creature, grid, mes_leviation, mes_normal, calc_fire_damage_rate, nullptr)) {
            cave_no_regen = true;
            sound(SoundKind::TERRAIN_DAMAGE);
        }
    }

    if (terrain.flags.has(TerrainCharacteristics::COLD_PUDDLE) && !creature.is_invulnerable() && !has_immune_cold(creature)) {
        constexpr auto mes_leviation = _("冷気に覆われた！", "The cold engulfs you!");
        constexpr auto mes_normal = _("に凍えた！", "frostbites you!");
        if (deal_damege_by_feat(creature, grid, mes_leviation, mes_normal, calc_cold_damage_rate, nullptr)) {
            cave_no_regen = true;
            sound(SoundKind::TERRAIN_DAMAGE);
        }
    }

    if (terrain.flags.has(TerrainCharacteristics::ELEC_PUDDLE) && !creature.is_invulnerable() && !has_immune_elec(creature)) {
        constexpr auto mes_leviation = _("電撃を受けた！", "The electricity shocks you!");
        constexpr auto mes_normal = _("に感電した！", "shocks you!");
        if (deal_damege_by_feat(creature, grid, mes_leviation, mes_normal, calc_elec_damage_rate, nullptr)) {
            cave_no_regen = true;
            sound(SoundKind::TERRAIN_DAMAGE);
        }
    }

    if (terrain.flags.has(TerrainCharacteristics::ACID_PUDDLE) && !creature.is_invulnerable() && !has_immune_acid(creature)) {
        constexpr auto mes_leviation = _("酸が飛び散った！", "The acid melts you!");
        constexpr auto mes_normal = _("に溶かされた！", "melts you!");
        if (deal_damege_by_feat(creature, grid, mes_leviation, mes_normal, calc_acid_damage_rate, nullptr)) {
            cave_no_regen = true;
            sound(SoundKind::TERRAIN_DAMAGE);
        }
    }

    if (terrain.flags.has(TerrainCharacteristics::POISON_PUDDLE) && !creature.is_invulnerable()) {
        constexpr auto mes_leviation = _("毒気を吸い込んだ！", "The gas poisons you!");
        constexpr auto mes_normal = _("に毒された！", "poisons you!");
        if (deal_damege_by_feat(creature, grid, mes_leviation, mes_normal, calc_pois_damage_rate,
                [](CreatureEntity &creature, int damage) {
                    if (!has_resist_pois(creature)) {
                        (void)BadStatusSetter(creature).mod_poison(static_cast<TIME_EFFECT>(damage));
                    }
                })) {
            cave_no_regen = true;
            sound(SoundKind::TERRAIN_DAMAGE);
        }
    }

    if (terrain.flags.has(TerrainCharacteristics::DUNG_POOL) && !creature.is_invulnerable()) {
        cave_no_regen = deal_damege_by_feat(creature, grid, _("糞が飛び散った！", "The feced scatter to you!"), _("に浸かった！", "tainted you!"),
            calc_acid_damage_rate, [](CreatureEntity &creature, int damage) {
                if (!has_resist_pois(creature)) {
                    (void)BadStatusSetter(creature).mod_poison(static_cast<TIME_EFFECT>(damage));
                }
            });
    }

    const auto can_drown = terrain.flags.has_all_of({ TerrainCharacteristics::WATER, TerrainCharacteristics::DEEP });
    if (can_drown && !creature.levitation && !creature.can_swim && !has_resist_water(creature)) {
        if (calc_inventory_weight(creature) > calc_weight_limit(creature)) {
            msg_print(_("溺れている！", "You are drowning!"));
            take_hit(creature, DAMAGE_NOESCAPE, randint1(creature.level), _("溺れ", "drowning"));
            cave_no_regen = true;
            sound(SoundKind::TERRAIN_DAMAGE);
        }
    }

    if (terrain.flags.has(TerrainCharacteristics::THORN) && !creature.levitation && !creature.is_invulnerable()) {
        int damage;
        msg_print(_("棘に体が突き刺さっている！", "Your body is stuck in a thorn!"));
        if (calc_inventory_weight(creature) > calc_weight_limit(creature)) {
            damage = randint1(creature.level);
        } else {
            damage = (randint1(creature.level) + 1) / 2;
        }
        cave_no_regen = true;
        take_hit(creature, DAMAGE_NOESCAPE, damage, _("突起物", "Protrusions"));
        sound(SoundKind::TERRAIN_DAMAGE);
    }

    if (terrain.flags.has(TerrainCharacteristics::PLASMA) && !creature.is_invulnerable()) {
        cave_no_regen = deal_damege_by_feat(creature, grid, _("に包まれた!", "engulfs you!"), _("に包まれた!", "engulfs you"), calc_plasma_damage_rate, NULL);
        sound(SoundKind::TERRAIN_DAMAGE);
    }

    if (terrain.flags.has(TerrainCharacteristics::CHAOS_TAINTED) && !creature.is_invulnerable()) {
        cave_no_regen = deal_damege_by_feat(creature, grid, _("に汚染された!", "taints you!"),
            _("に汚染された!", "taints you"), calc_chaos_damage_rate_rand, NULL);
        sound(SoundKind::TERRAIN_DAMAGE);
    }

    if (terrain.flags.has(TerrainCharacteristics::VOID) && !creature.is_invulnerable()) {
        cave_no_regen = deal_damege_by_feat(creature, grid, _("に巻き込まれて己の存在が薄れていく!", "erases your existence!"),
            _("に巻き込まれて己の存在が薄れていく!", "erases your existence!"), calc_void_damage_rate_rand, NULL);
        sound(SoundKind::TERRAIN_DAMAGE);
    }

    if (get_player_flags(creature, TR_SELF_FIRE) && !has_immune_fire(creature)) {
        int damage;
        damage = creature.level;
        if (race.tr_flags().has(TR_VUL_FIRE)) {
            damage += damage / 3;
        }
        if (has_resist_fire(creature)) {
            damage = damage / 3;
        }
        if (is_oppose_fire(creature)) {
            damage = damage / 3;
        }

        damage = std::max(damage, 1);
        msg_print(_("熱い！", "It's hot!"));
        take_hit(creature, DAMAGE_NOESCAPE, damage, _("炎のオーラ", "Fire aura"));
    }

    if (get_player_flags(creature, TR_SELF_ELEC) && !has_immune_elec(creature)) {
        int damage;
        damage = creature.level;
        if (race.tr_flags().has(TR_VUL_ELEC)) {
            damage += damage / 3;
        }
        if (has_resist_elec(creature)) {
            damage = damage / 3;
        }
        if (is_oppose_elec(creature)) {
            damage = damage / 3;
        }

        damage = std::max(damage, 1);
        msg_print(_("痛い！", "It hurts!"));
        take_hit(creature, DAMAGE_NOESCAPE, damage, _("電気のオーラ", "Elec aura"));
    }

    if (get_player_flags(creature, TR_SELF_COLD) && !has_immune_cold(creature)) {
        int damage;
        damage = creature.level;
        if (race.tr_flags().has(TR_VUL_COLD)) {
            damage += damage / 3;
        }
        if (has_resist_cold(creature)) {
            damage = damage / 3;
        }
        if (is_oppose_cold(creature)) {
            damage = damage / 3;
        }

        damage = std::max(damage, 1);
        msg_print(_("冷たい！", "It's cold!"));
        take_hit(creature, DAMAGE_NOESCAPE, damage, _("冷気のオーラ", "Cold aura"));
    }

    if (creature.riding) {
        int damage;
        auto auras = floor.get_monster(creature.riding).get_monrace().aura_flags;
        if (auras.has(MonsterAuraType::FIRE) && !has_immune_fire(creature)) {
            damage = floor.get_monster(creature.riding).get_monrace().level / 2;
            if (race.tr_flags().has(TR_VUL_FIRE)) {
                damage += damage / 3;
            }
            if (has_resist_fire(creature)) {
                damage = damage / 3;
            }
            if (is_oppose_fire(creature)) {
                damage = damage / 3;
            }

            damage = std::max(damage, 1);
            msg_print(_("熱い！", "It's hot!"));
            take_hit(creature, DAMAGE_NOESCAPE, damage, _("炎のオーラ", "Fire aura"));
        }

        if (auras.has(MonsterAuraType::ELEC) && !has_immune_elec(creature)) {
            damage = floor.get_monster(creature.riding).get_monrace().level / 2;
            if (race.tr_flags().has(TR_VUL_ELEC)) {
                damage += damage / 3;
            }
            if (has_resist_elec(creature)) {
                damage = damage / 3;
            }
            if (is_oppose_elec(creature)) {
                damage = damage / 3;
            }

            damage = std::max(damage, 1);
            msg_print(_("痛い！", "It hurts!"));
            take_hit(creature, DAMAGE_NOESCAPE, damage, _("電気のオーラ", "Elec aura"));
        }

        if (auras.has(MonsterAuraType::COLD) && !has_immune_cold(creature)) {
            damage = floor.get_monster(creature.riding).get_monrace().level / 2;
            if (race.tr_flags().has(TR_VUL_COLD)) {
                damage += damage / 3;
            }
            if (has_resist_cold(creature)) {
                damage = damage / 3;
            }
            if (is_oppose_cold(creature)) {
                damage = damage / 3;
            }

            damage = std::max(damage, 1);
            msg_print(_("冷たい！", "It's cold!"));
            take_hit(creature, DAMAGE_NOESCAPE, damage, _("冷気のオーラ", "Cold aura"));
        }
    }

    /* Spectres -- take damage when moving through walls */
    /*
     * Added: ANYBODY takes damage if inside through walls
     * without wraith form -- NOTE: Spectres will never be
     * reduced below 0 hp by being inside a stone wall; others
     * WILL BE!
     */
    if (terrain.flags.has_none_of({ TerrainCharacteristics::MOVE, TerrainCharacteristics::CAN_FLY })) {
        auto should_damage = !creature.is_invulnerable();
        should_damage &= creature.get_remaining_wraith_form() == 0;
        should_damage &= creature.tim_pass_wall == 0;
        should_damage &= (creature.hp > (creature.level / 5)) || !has_pass_wall(creature);
        if (should_damage) {
            concptr dam_desc;
            cave_no_regen = true;

            if (has_pass_wall(creature)) {
                msg_print(_("体の分子が分解した気がする！", "Your molecules feel disrupted!"));
                dam_desc = _("密度", "density");
            } else {
                msg_print(_("崩れた岩に押し潰された！", "You are being crushed!"));
                dam_desc = _("硬い岩", "solid rock");
            }

            take_hit(creature, DAMAGE_NOESCAPE, 1 + (creature.level / 5), dam_desc);
        }
    }

    if (creature.food < PY_FOOD_WEAK) {
        if (creature.food < PY_FOOD_STARVE) {
            regen_amount = 0;
        } else if (creature.food < PY_FOOD_FAINT) {
            regen_amount = PY_REGEN_FAINT;
        } else {
            regen_amount = PY_REGEN_WEAK;
        }
    }

    CreatureClass pc(creature);
    if (pattern_effect(creature)) {
        cave_no_regen = true;
    } else {
        if (creature.regenerate) {
            regen_amount = regen_amount * 2;
        }

        if (!pc.monk_stance_is(MonkStanceType::NONE) || !pc.samurai_stance_is(SamuraiStanceType::NONE)) {
            regen_amount /= 2;
        }
        if (creature.cursed.has(CurseTraitType::SLOW_REGEN)) {
            regen_amount /= 5;
        }
    }

    if ((creature.action == ACTION_SEARCH) || (creature.action == ACTION_REST)) {
        regen_amount = regen_amount * 2;
    }

    upkeep_factor = calculate_upkeep(creature);
    if ((creature.action == ACTION_LEARN) || (creature.action == ACTION_HAYAGAKE) || pc.samurai_stance_is(SamuraiStanceType::KOUKIJIN)) {
        upkeep_factor += 100;
    }

    regenmana(creature, upkeep_factor, regen_amount);
    if (pc.equals(PlayerClassType::MAGIC_EATER)) {
        regenmagic(creature, regen_amount);
    }

    if ((creature.csp == 0) && (creature.csp_frac == 0)) {
        while (upkeep_factor > 100) {
            msg_print(_("こんなに多くのペットを制御できない！", "Too many pets to control at once!"));
            msg_erase();
            do_cmd_pet_dismiss(creature);

            upkeep_factor = calculate_upkeep(creature);

            msg_format(_("維持ＭＰは %d%%", "Upkeep: %d%% mana."), upkeep_factor);
            msg_erase();
        }
    }

    if (player_poison.is_poisoned()) {
        regen_amount = 0;
    }
    if (player_cut.is_cut()) {
        regen_amount = 0;
    }
    if (cave_no_regen) {
        regen_amount = 0;
    }

    // Apply hygiene-based regeneration modifier
    if (regen_amount > 0 && terrain.hygiene != 0) {
        const int hygiene_modifier = 100 + terrain.hygiene;
        regen_amount = (regen_amount * hygiene_modifier) / 100;
        if (regen_amount < 0) {
            regen_amount = 0;
        }
    }

    regen_amount = (regen_amount * creature.mutant_regenerate_mod) / 100;
    if ((creature.hp < creature.maxhp) && !cave_no_regen) {
        regenhp(creature, regen_amount);
    }
}

/*
 * Increase players hit points, notice effects
 */
bool hp_player(CreatureEntity &creature, int num)
{
    int vir;
    vir = virtue_number(creature, Virtue::VITALITY);

    if (num <= 0) {
        return false;
    }

    if (vir) {
        auto it = creature.virtues.find(Virtue::VITALITY);
        if (it != creature.virtues.end()) {
            num = num * (it->second + 1250) / 1250;
        }
    }

    if (creature.hp < creature.maxhp) {
        if ((num > 0) && (creature.hp < (creature.maxhp / 3))) {
            chg_virtue(creature, Virtue::TEMPERANCE, 1);
        }

        creature.hp += num;
        if (creature.hp >= creature.maxhp) {
            creature.hp = creature.maxhp;
            creature.hp_frac = 0;
        }

        auto &rfu = RedrawingFlagsUpdater::get_instance();
        rfu.set_flag(MainWindowRedrawingFlag::HP);
        rfu.set_flag(SubWindowRedrawingFlag::PLAYER);
        if (num < 5) {
            msg_print(_("少し気分が良くなった。", "You feel a little better."));
        } else if (num < 15) {
            msg_print(_("気分が良くなった。", "You feel better."));
        } else if (num < 35) {
            msg_print(_("とても気分が良くなった。", "You feel much better."));
        } else {
            msg_print(_("ひじょうに気分が良くなった。", "You feel very good."));
        }

        return true;
    }

    return false;
}
