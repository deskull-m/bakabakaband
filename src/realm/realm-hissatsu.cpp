﻿#include "realm/realm-hissatsu.h"
#include "artifact/fixed-art-types.h"
#include "cmd-action/cmd-attack.h"
#include "cmd-action/cmd-spell.h"
#include "cmd-item/cmd-throw.h"
#include "combat/combat-options-type.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "dungeon/dungeon-flag-types.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "effect/spells-effect-util.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "grid/feature-flag-types.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "mind/mind-ninja.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-brightness-mask.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-update.h"
#include "object-enchant/tr-types.h"
#include "player-info/equipment-info.h"
#include "player/player-damage.h"
#include "player/player-move.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell/technic-info-table.h"
#include "status/bad-status-setter.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/grid-selector.h"
#include "target/projection-path-calculator.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief 剣術の各処理を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param spell 剣術ID
 * @param mode 処理内容 (SpellProcessType::NAME / SPELL_DESC / SpellProcessType::CAST)
 * @return SpellProcessType::NAME / SPELL_DESC 時には文字列を返す。SpellProcessType::CAST時は std::nullopt を返す。
 */
std::optional<std::string> do_hissatsu_spell(PlayerType *player_ptr, SPELL_IDX spell, SpellProcessType mode)
{
    bool name = mode == SpellProcessType::NAME;
    bool desc = mode == SpellProcessType::DESCRIPTION;
    bool cast = mode == SpellProcessType::CAST;

    PLAYER_LEVEL plev = player_ptr->lev;

    switch (spell) {
    case 0:
        if (name) {
            return _("飛飯綱", "Tobi-Izuna");
        }
        if (desc) {
            return _("2マス離れたところにいるモンスターを攻撃する。", "Attacks a monster two squares away.");
        }

        if (cast) {
            project_length = 2;
            int dir;
            if (!get_aim_dir(player_ptr, &dir)) {
                return std::nullopt;
            }

            project_hook(player_ptr, AttributeType::ATTACK, dir, HISSATSU_2, PROJECT_STOP | PROJECT_KILL);
        }
        break;

    case 1:
        if (name) {
            return _("五月雨斬り", "3-Way Attack");
        }
        if (desc) {
            return _("3方向に対して攻撃する。", "Attacks in 3 directions at one time.");
        }

        if (cast) {
            const auto cdir = get_direction_as_cdir(player_ptr);
            if (!cdir) {
                return std::nullopt;
            }

            const auto attack_to = [player_ptr](int cdir) {
                const auto pos = player_ptr->get_position() + CCW_DD[cdir];
                const auto &grid = player_ptr->current_floor_ptr->get_grid(pos);

                if (grid.has_monster()) {
                    do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_NONE);
                } else {
                    msg_print(_("攻撃は空を切った。", "You attack the empty air."));
                }
            };

            attack_to(*cdir); // 指定方向
            attack_to((*cdir + 7) % 8); // 指定方向の右
            attack_to((*cdir + 1) % 8); // 指定方向の左
        }
        break;

    case 2:
        if (name) {
            return _("ブーメラン", "Boomerang");
        }
        if (desc) {
            return _(
                "武器を手元に戻ってくるように投げる。戻ってこないこともある。", "Throws current weapon. It'll return to your hand unless the action failed.");
        }

        if (cast) {
            if (!ThrowCommand(player_ptr).do_cmd_throw(1, true, -1)) {
                return std::nullopt;
            }
        }
        break;

    case 3:
        if (name) {
            return _("焔霊", "Burning Strike");
        }
        if (desc) {
            return _("火炎耐性のないモンスターに大ダメージを与える。", "Attacks a monster with more damage unless it has resistance to fire.");
        }

        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir || (dir == 5)) {
                return std::nullopt;
            }

            const auto pos = player_ptr->get_neighbor(*dir);
            if (player_ptr->current_floor_ptr->get_grid(pos).has_monster()) {
                do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_FIRE);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return std::nullopt;
            }
        }
        break;

    case 4:
        if (name) {
            return _("殺気感知", "Detect Ferocity");
        }
        if (desc) {
            return _("近くの思考することができるモンスターを感知する。", "Detects all monsters except the mindless in your vicinity.");
        }

        if (cast) {
            detect_monsters_mind(player_ptr, DETECT_RAD_DEFAULT);
        }
        break;

    case 5:
        if (name) {
            return _("みね打ち", "Strike to Stun");
        }
        if (desc) {
            return _("相手にダメージを与えないが、朦朧とさせる。", "Attempts to stun a monster next to you.");
        }

        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir || (dir == 5)) {
                return std::nullopt;
            }

            const auto pos = player_ptr->get_neighbor(*dir);
            if (player_ptr->current_floor_ptr->get_grid(pos).has_monster()) {
                do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_MINEUCHI);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return std::nullopt;
            }
        }
        break;

    case 6:
        if (name) {
            return _("カウンター", "Counter");
        }
        if (desc) {
            return _("相手に攻撃されたときに反撃する。反撃するたびにMPを消費。",
                "Prepares to counterattack. When attacked by a monster, strikes back using SP each time.");
        }

        if (cast) {
            if (player_ptr->riding) {
                msg_print(_("乗馬中には無理だ。", "You cannot do it when riding."));
                return std::nullopt;
            }
            msg_print(_("相手の攻撃に対して身構えた。", "You prepare to counterattack."));
            player_ptr->counter = true;
        }
        break;

    case 7:
        if (name) {
            return _("払い抜け", "Harainuke");
        }
        if (desc) {
            return _("攻撃した後、反対側に抜ける。",
                "In one action, attacks a monster with your weapons normally and then moves to the space beyond the monster if that space is not blocked.");
        }

        if (cast) {
            if (player_ptr->riding) {
                msg_print(_("乗馬中には無理だ。", "You cannot do it when riding."));
                return std::nullopt;
            }

            const auto dir = get_direction(player_ptr);
            if (!dir || (dir == 5)) {
                return std::nullopt;
            }

            const auto pos_target = player_ptr->get_neighbor(*dir);
            const auto &floor = *player_ptr->current_floor_ptr;
            const auto &grid_target = floor.get_grid(pos_target);
            if (!grid_target.has_monster()) {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return std::nullopt;
            }

            do_cmd_attack(player_ptr, pos_target.y, pos_target.x, HISSATSU_NONE);
            if (!player_can_enter(player_ptr, grid_target.feat, 0) || is_trap(player_ptr, grid_target.feat)) {
                break;
            }

            const Pos2D pos_opposite(pos_target.y + ddy[*dir], pos_target.x + ddx[*dir]);
            const auto &grid_opposite = floor.get_grid(pos_opposite);
            if (player_can_enter(player_ptr, grid_opposite.feat, 0) && !is_trap(player_ptr, grid_opposite.feat) && !grid_opposite.m_idx) {
                msg_print(nullptr);
                (void)move_player_effect(player_ptr, pos_opposite.y, pos_opposite.x, MPE_FORGET_FLOW | MPE_HANDLE_STUFF | MPE_DONT_PICKUP);
            }
        }
        break;

    case 8:
        if (name) {
            return _("サーペンツタン", "Serpent's Tongue");
        }
        if (desc) {
            return _("毒耐性のないモンスターに大ダメージを与える。", "Attacks a monster with more damage unless it has resistance to poison.");
        }

        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir || (dir == 5)) {
                return std::nullopt;
            }

            const auto pos = player_ptr->get_neighbor(*dir);
            const auto &floor = *player_ptr->current_floor_ptr;
            if (floor.get_grid(pos).has_monster()) {
                do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_POISON);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return std::nullopt;
            }
        }
        break;

    case 9:
        if (name) {
            return _("斬魔剣弐の太刀", "Zammaken");
        }
        if (desc) {
            return _("生命のない邪悪なモンスターに大ダメージを与えるが、他のモンスターには全く効果がない。",
                "Attacks an evil unliving monster with great damage. Has no effect on other monsters.");
        }

        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir || (dir == 5)) {
                return std::nullopt;
            }

            const auto pos = player_ptr->get_neighbor(*dir);
            const auto &floor = *player_ptr->current_floor_ptr;
            if (floor.get_grid(pos).has_monster()) {
                do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_ZANMA);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return std::nullopt;
            }
        }
        break;

    case 10:
        if (name) {
            return _("裂風剣", "Wind Blast");
        }
        if (desc) {
            return _("攻撃した相手を後方へ吹き飛ばす。", "Attacks an adjacent monster and blows it away.");
        }

        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir || (dir == 5)) {
                return std::nullopt;
            }

            const auto pos = player_ptr->get_neighbor(*dir);
            auto &floor = *player_ptr->current_floor_ptr;
            const auto &grid = floor.get_grid(pos);
            if (grid.has_monster()) {
                do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_NONE);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return std::nullopt;
            }
            if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::NO_MELEE)) {
                return "";
            }
            if (grid.has_monster()) {
                Pos2D target(pos.y, pos.x);
                Pos2D origin(pos.y, pos.x);
                const auto m_idx = grid.m_idx;
                auto &monster = floor.m_list[m_idx];
                const auto m_name = monster_desc(player_ptr, &monster, 0);
                Pos2D neighbor(pos.y, pos.x);
                for (auto i = 0; i < 5; i++) {
                    neighbor.y += ddy[*dir];
                    neighbor.x += ddx[*dir];
                    if (is_cave_empty_bold(player_ptr, neighbor.y, neighbor.x)) {
                        target = Pos2D(neighbor.y, neighbor.x);
                    } else {
                        break;
                    }
                }
                if (target != origin) {
                    msg_format(_("%sを吹き飛ばした！", "You blow %s away!"), m_name.data());
                    floor.get_grid(origin).m_idx = 0;
                    floor.get_grid(target).m_idx = m_idx;
                    monster.fy = target.y;
                    monster.fx = target.x;

                    update_monster(player_ptr, m_idx, true);
                    lite_spot(player_ptr, origin.y, origin.x);
                    lite_spot(player_ptr, target.y, target.x);

                    if (monster.get_monrace().brightness_flags.has_any_of(ld_mask)) {
                        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::MONSTER_LITE);
                    }
                }
            }
        }
        break;

    case 11:
        if (name) {
            return _("刀匠の目利き", "Judge");
        }
        if (desc) {
            return _("武器・防具を1つ識別する。レベル45以上で武器・防具の能力を完全に知ることができる。",
                "Identifies a weapon or armor. *Identifies* the item at level 45.");
        }

        if (cast) {
            if (plev > 44) {
                if (!identify_fully(player_ptr, true)) {
                    return std::nullopt;
                }
            } else {
                if (!ident_spell(player_ptr, true)) {
                    return std::nullopt;
                }
            }
        }
        break;

    case 12:
        if (name) {
            return _("破岩斬", "Rock Smash");
        }
        if (desc) {
            return _("岩を壊し、岩石系のモンスターに大ダメージを与える。", "Breaks rock or greatly damages a monster made of rocks.");
        }

        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir || (dir == 5)) {
                return std::nullopt;
            }

            const auto pos = player_ptr->get_neighbor(*dir);
            const auto &floor = *player_ptr->current_floor_ptr;
            if (floor.get_grid(pos).has_monster()) {
                do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_HAGAN);
            }

            if (!cave_has_flag_bold(player_ptr->current_floor_ptr, pos.y, pos.x, TerrainCharacteristics::HURT_ROCK)) {
                break;
            }

            /* Destroy the feature */
            cave_alter_feat(player_ptr, pos.y, pos.x, TerrainCharacteristics::HURT_ROCK);
            RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::FLOW);
        }
        break;

    case 13:
        if (name) {
            return _("乱れ雪月花", "Midare-Setsugekka");
        }
        if (desc) {
            return _("攻撃回数が増え、冷気耐性のないモンスターに大ダメージを与える。",
                "Attacks a monster with an increased number of attacks and more damage unless it has resistance to cold.");
        }

        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir || (dir == 5)) {
                return std::nullopt;
            }

            const auto pos = player_ptr->get_neighbor(*dir);
            const auto &floor = *player_ptr->current_floor_ptr;
            if (floor.get_grid(pos).has_monster()) {
                do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_COLD);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return std::nullopt;
            }
        }
        break;

    case 14:
        if (name) {
            return _("急所突き", "Spot Aiming");
        }
        if (desc) {
            return _("モンスターを一撃で倒す攻撃を繰り出す。失敗すると1点しかダメージを与えられない。",
                "Attempts to kill a monster instantly. If that fails, causes only 1HP of damage.");
        }

        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir || (dir == 5)) {
                return std::nullopt;
            }

            const auto pos = player_ptr->get_neighbor(*dir);
            const auto &floor = *player_ptr->current_floor_ptr;
            if (floor.get_grid(pos).has_monster()) {
                do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_KYUSHO);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return std::nullopt;
            }
        }
        break;

    case 15:
        if (name) {
            return _("魔神斬り", "Majingiri");
        }
        if (desc) {
            return _("会心の一撃で攻撃する。攻撃がかわされやすい。", "Attempts to attack with a critical hit, but this attack is easy to evade for a monster.");
        }

        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir || (dir == 5)) {
                return std::nullopt;
            }

            const auto pos = player_ptr->get_neighbor(*dir);
            const auto &floor = *player_ptr->current_floor_ptr;
            if (floor.get_grid(pos).has_monster()) {
                do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_MAJIN);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return std::nullopt;
            }
        }
        break;

    case 16:
        if (name) {
            return _("捨て身", "Desperate Attack");
        }
        if (desc) {
            return _("強力な攻撃を繰り出す。次のターンまでの間、食らうダメージが増える。",
                "Attacks with all of your power, but all damage you take will be doubled for one turn.");
        }

        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir || (dir == 5)) {
                return std::nullopt;
            }

            const auto pos = player_ptr->get_neighbor(*dir);
            const auto &floor = *player_ptr->current_floor_ptr;
            if (floor.get_grid(pos).has_monster()) {
                do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_SUTEMI);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return std::nullopt;
            }
            player_ptr->sutemi = true;
        }
        break;

    case 17:
        if (name) {
            return _("雷撃鷲爪斬", "Lightning Eagle");
        }
        if (desc) {
            return _("電撃耐性のないモンスターに非常に大きいダメージを与える。", "Attacks a monster with more damage unless it has resistance to electricity.");
        }

        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir || (dir == 5)) {
                return std::nullopt;
            }

            const auto pos = player_ptr->get_neighbor(*dir);
            const auto &floor = *player_ptr->current_floor_ptr;
            if (floor.get_grid(pos).has_monster()) {
                do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_ELEC);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return std::nullopt;
            }
        }
        break;

    case 18:
        if (name) {
            return _("入身", "Rush Attack");
        }
        if (desc) {
            return _("素早く相手に近寄り攻撃する。", "Steps close to a monster and attacks at the same time.");
        }

        if (cast) {
            if (!rush_attack(player_ptr, nullptr)) {
                return std::nullopt;
            }
        }
        break;

    case 19:
        if (name) {
            return _("赤流渦", "Bloody Maelstrom");
        }

        if (desc) {
            return _("自分自身も傷を作りつつ、その傷が深いほど大きい威力で全方向の敵を攻撃できる。生きていないモンスターには効果がない。",
                "Attacks all adjacent monsters with power corresponding to your cuts. Then increases your cuts. Has no effect on unliving monsters.");
        }

        if (cast) {
            const auto current_cut = player_ptr->effects()->cut().current();
            short new_cut = current_cut < 300 ? current_cut + 300 : current_cut * 2;
            (void)BadStatusSetter(player_ptr).set_cut(new_cut);
            const auto &floor = *player_ptr->current_floor_ptr;
            for (auto dir = 0; dir < 8; dir++) {
                const auto pos = player_ptr->get_position();
                const Pos2D pos_ddd(pos.y + ddy_ddd[dir], pos.x + ddx_ddd[dir]);
                const auto &grid = floor.get_grid(pos_ddd);
                const auto &monster = floor.m_list[grid.m_idx];
                if (!grid.has_monster() || (!monster.ml && !cave_has_flag_bold(&floor, pos_ddd.y, pos_ddd.x, TerrainCharacteristics::PROJECT))) {
                    continue;
                }

                if (monster.has_living_flag()) {
                    do_cmd_attack(player_ptr, pos_ddd.y, pos_ddd.x, HISSATSU_SEKIRYUKA);
                    continue;
                }

                const auto m_name = monster_desc(player_ptr, &monster, 0);
                msg_format(_("%sには効果がない！", "%s is unharmed!"), m_name.data());
            }
        }

        break;
    case 20:
        if (name) {
            return _("激震撃", "Earthquake Blow");
        }
        if (desc) {
            return _("地震を起こす。", "Shakes dungeon structure, and results in random swapping of floors and walls.");
        }

        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir || (dir == 5)) {
                return std::nullopt;
            }

            const auto pos = player_ptr->get_neighbor(*dir);
            const auto &floor = *player_ptr->current_floor_ptr;
            if (floor.get_grid(pos).has_monster()) {
                do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_QUAKE);
            } else {
                earthquake(player_ptr, player_ptr->y, player_ptr->x, 10, 0);
            }
        }
        break;

    case 21:
        if (name) {
            return _("地走り", "Crack");
        }
        if (desc) {
            return _("衝撃波のビームを放つ。", "Fires a shock wave as a beam.");
        }

        if (cast) {
            int total_damage = 0, basedam, i;
            ItemEntity *o_ptr;
            int dir;
            if (!get_aim_dir(player_ptr, &dir)) {
                return std::nullopt;
            }
            msg_print(_("武器を大きく振り下ろした。", "You swing your weapon downward."));
            for (i = 0; i < 2; i++) {
                int damage;

                if (!has_melee_weapon(player_ptr, INVEN_MAIN_HAND + i)) {
                    break;
                }
                o_ptr = &player_ptr->inventory_list[INVEN_MAIN_HAND + i];
                basedam = (o_ptr->dd * (o_ptr->ds + 1)) * 50;
                damage = o_ptr->to_d * 100;

                // @todo ヴォーパルの多重定義.
                if (o_ptr->is_specific_artifact(FixedArtifactId::VORPAL_BLADE) || o_ptr->is_specific_artifact(FixedArtifactId::CHAINSWORD)) {
                    /* vorpal blade */
                    basedam *= 5;
                    basedam /= 3;
                } else if (o_ptr->get_flags().has(TR_VORPAL)) {
                    /* vorpal flag only */
                    basedam *= 11;
                    basedam /= 9;
                }
                damage += basedam;
                damage *= player_ptr->num_blow[i];
                total_damage += damage / 200;
                if (i) {
                    total_damage = total_damage * 7 / 10;
                }
            }
            fire_beam(player_ptr, AttributeType::FORCE, dir, total_damage);
        }
        break;

    case 22:
        if (name) {
            return _("気迫の雄叫び", "War Cry");
        }
        if (desc) {
            return _("視界内の全モンスターに対して轟音の攻撃を行う。さらに、近くにいるモンスターを怒らせる。",
                "Damages all monsters in sight with sound. Aggravates nearby monsters.");
        }

        if (cast) {
            msg_print(_("雄叫びをあげた！", "You roar!"));
            project_all_los(player_ptr, AttributeType::SOUND, randint1(plev * 3));
            aggravate_monsters(player_ptr, 0);
        }
        break;

    case 23:
        if (name) {
            return _("無双三段", "Musou-Sandan");
        }
        if (desc) {
            return _("強力な3段攻撃を繰り出す。", "Attacks with three powerful strikes.");
        }

        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir || (dir == 5)) {
                return std::nullopt;
            }

            auto &floor = *player_ptr->current_floor_ptr;
            for (auto i = 0; i < 3; i++) {
                const Pos2D pos = player_ptr->get_neighbor(*dir);
                auto &grid = floor.get_grid(pos);

                if (grid.has_monster()) {
                    do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_3DAN);
                } else {
                    msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                    return std::nullopt;
                }

                if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::NO_MELEE)) {
                    return "";
                }

                /* Monster is dead? */
                if (!grid.has_monster()) {
                    break;
                }

                const Pos2D pos_new(pos.y + ddy[*dir], pos.x + ddx[*dir]);
                const auto m_idx = grid.m_idx;
                auto &monster = floor.m_list[m_idx];

                /* Monster cannot move back? */
                if (!monster_can_enter(player_ptr, pos_new.y, pos_new.x, &monster.get_monrace(), 0)) {
                    /* -more- */
                    if (i < 2) {
                        msg_print(nullptr);
                    }
                    continue;
                }

                grid.m_idx = 0;
                floor.get_grid(pos_new).m_idx = m_idx;
                monster.fy = pos_new.y;
                monster.fx = pos_new.x;

                update_monster(player_ptr, m_idx, true);

                /* Redraw the old spot */
                lite_spot(player_ptr, pos.y, pos.x);

                /* Redraw the new spot */
                lite_spot(player_ptr, pos_new.y, pos_new.x);

                /* Player can move forward? */
                if (player_can_enter(player_ptr, grid.feat, 0)) {
                    if (!move_player_effect(player_ptr, pos.y, pos.x, MPE_FORGET_FLOW | MPE_HANDLE_STUFF | MPE_DONT_PICKUP)) {
                        break;
                    }
                } else {
                    break;
                }

                /* -more- */
                if (i < 2) {
                    msg_print(nullptr);
                }
            }
        }
        break;

    case 24:
        if (name) {
            return _("吸血鬼の牙", "Vampire's Fang");
        }
        if (desc) {
            return _("攻撃した相手の体力を吸いとり、自分の体力を回復させる。生命を持たないモンスターには通じない。",
                "Attacks with vampiric strikes which absorb HP from a monster and heal you. Has no effect on unliving monsters.");
        }

        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir || (dir == 5)) {
                return std::nullopt;
            }

            const auto pos = player_ptr->get_neighbor(*dir);
            const auto &floor = *player_ptr->current_floor_ptr;
            if (floor.get_grid(pos).has_monster()) {
                do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_DRAIN);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return std::nullopt;
            }
        }
        break;

    case 25:
        if (name) {
            return _("幻惑", "Moon Dazzling");
        }
        if (desc) {
            return _("視界内の起きている全モンスターに朦朧、混乱、眠りを与えようとする。", "Attempts to stun, confuse and put to sleep all waking monsters.");
        }

        if (cast) {
            msg_print(_("武器を不規則に揺らした．．．", "You irregularly wave your weapon..."));
            project_all_los(player_ptr, AttributeType::ENGETSU, plev * 4);
        }
        break;

    case 26:
        if (name) {
            return _("百人斬り", "Hundred Slaughter");
        }
        if (desc) {
            return _("連続して入身でモンスターを攻撃する。攻撃するたびにMPを消費。MPがなくなるか、モンスターを倒せなかったら百人斬りは終了する。",
                "Performs a series of rush attacks. The series continues as long as the attacked monster dies and you have sufficient SP.");
        }

        if (cast) {
            const int mana_cost_per_monster = 8;
            bool is_new = true;
            bool mdeath;

            do {
                if (!rush_attack(player_ptr, &mdeath)) {
                    break;
                }
                if (is_new) {
                    /* Reserve needed mana point */
                    player_ptr->csp -= technic_info[REALM_HISSATSU - MIN_TECHNIC][26].smana;
                    is_new = false;
                } else {
                    player_ptr->csp -= mana_cost_per_monster;
                }

                if (!mdeath) {
                    break;
                }
                command_dir = 0;

                RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::MP);
                handle_stuff(player_ptr);
            } while (player_ptr->csp > mana_cost_per_monster);

            if (is_new) {
                return std::nullopt;
            }

            /* Restore reserved mana */
            player_ptr->csp += technic_info[REALM_HISSATSU - MIN_TECHNIC][26].smana;
        }
        break;

    case 27:
        if (name) {
            return _("天翔龍閃", "Dragonic Flash");
        }
        if (desc) {
            return _("視界内の場所を指定して、その場所と自分の間にいる全モンスターを攻撃し、その場所に移動する。",
                "Runs toward given location while attacking all monsters on the path.");
        }

        if (cast) {
            POSITION y, x;

            if (!tgt_pt(player_ptr, &x, &y)) {
                return std::nullopt;
            }

            const auto is_teleportable = cave_player_teleportable_bold(player_ptr, y, x, TELEPORT_SPONTANEOUS);
            const auto dist = distance(y, x, player_ptr->y, player_ptr->x);
            if (!is_teleportable || (dist > MAX_PLAYER_SIGHT / 2) || !projectable(player_ptr, player_ptr->y, player_ptr->x, y, x)) {
                msg_print(_("失敗！", "You cannot move to that place!"));
                break;
            }
            if (player_ptr->anti_tele) {
                msg_print(_("不思議な力がテレポートを防いだ！", "A mysterious force prevents you from teleporting!"));
                break;
            }
            project(player_ptr, 0, 0, y, x, HISSATSU_ISSEN, AttributeType::ATTACK, PROJECT_BEAM | PROJECT_KILL);
            teleport_player_to(player_ptr, y, x, TELEPORT_SPONTANEOUS);
        }
        break;

    case 28:
        if (name) {
            return _("二重の剣撃", "Twin Slash");
        }
        if (desc) {
            return _("1ターンで2度攻撃を行う。", "Attack twice in one turn.");
        }

        if (cast) {
            int dir;
            if (!get_rep_dir(player_ptr, &dir)) {
                return std::nullopt;
            }

            const auto pos = player_ptr->get_neighbor(dir);
            const auto &grid = player_ptr->current_floor_ptr->get_grid(pos);
            if (grid.has_monster()) {
                do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_NONE);
                if (grid.has_monster()) {
                    handle_stuff(player_ptr);
                    do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_NONE);
                }
            } else {
                msg_print(_("その方向にはモンスターはいません。", "You don't see any monster in this direction"));
                return std::nullopt;
            }
        }
        break;

    case 29:
        if (name) {
            return _("虎伏絶刀勢", "Kofuku-Zettousei");
        }
        if (desc) {
            return _("強力な攻撃を行い、近くの場所にも効果が及ぶ。", "Performs a powerful attack which even affects nearby monsters.");
        }

        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir || (dir == 5)) {
                return std::nullopt;
            }

            const auto pos = player_ptr->get_neighbor(*dir);
            const auto &floor = *player_ptr->current_floor_ptr;
            if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::NO_MELEE)) {
                msg_print(_("なぜか攻撃することができない。", "Something prevents you from attacking."));
                return "";
            }

            msg_print(_("武器を大きく振り下ろした。", "You swing your weapon downward."));
            auto total_damage = 0;
            for (auto i = 0; i < 2; i++) {
                if (!has_melee_weapon(player_ptr, INVEN_MAIN_HAND + i)) {
                    break;
                }

                const auto &item = player_ptr->inventory_list[INVEN_MAIN_HAND + i];
                auto basedam = (item.dd * (item.ds + 1)) * 50;
                auto damage = item.to_d * 100;

                // @todo ヴォーパルの多重定義.
                if (item.is_specific_artifact(FixedArtifactId::VORPAL_BLADE) || item.is_specific_artifact(FixedArtifactId::CHAINSWORD)) {
                    /* vorpal blade */
                    basedam *= 5;
                    basedam /= 3;
                } else if (item.get_flags().has(TR_VORPAL)) {
                    /* vorpal flag only */
                    basedam *= 11;
                    basedam /= 9;
                }
                damage += basedam;
                damage += player_ptr->to_d[i] * 100;
                damage *= player_ptr->num_blow[i];
                total_damage += (damage / 100);
            }

            const auto is_bold = cave_has_flag_bold(&floor, pos.y, pos.x, TerrainCharacteristics::PROJECT);
            constexpr auto flags = PROJECT_KILL | PROJECT_JUMP | PROJECT_ITEM;
            project(player_ptr, 0, (is_bold ? 5 : 0), pos.y, pos.x, total_damage * 3 / 2, AttributeType::METEOR, flags);
        }
        break;

    case 30:
        if (name) {
            return _("慶雲鬼忍剣", "Keiun-Kininken");
        }
        if (desc) {
            return _("自分もダメージをくらうが、相手に非常に大きなダメージを与える。アンデッドには特に効果がある。",
                "Attacks a monster with extremely powerful damage, but you also take some damage. Hurts an undead monster greatly.");
        }

        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir || (dir == 5)) {
                return std::nullopt;
            }

            const auto pos = player_ptr->get_neighbor(*dir);
            const auto &floor = *player_ptr->current_floor_ptr;
            if (floor.get_grid(pos).has_monster()) {
                do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_UNDEAD);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return std::nullopt;
            }
            take_hit(player_ptr, DAMAGE_NOESCAPE, 100 + randint1(100), _("慶雲鬼忍剣を使った衝撃", "exhaustion on using Keiun-Kininken"));
        }
        break;

    case 31:
        if (name) {
            return _("切腹", "Harakiri");
        }
        if (desc) {
            return _("「武士道とは、死ぬことと見つけたり。」", "'Bushido, the way of warriors, is found in death'");
        }

        if (cast) {
            int i;
            if (!input_check(_("何もかも諦めますか? ", "Do you give up everything? "))) {
                return std::nullopt;
            }
            /* Special Verification for suicide */
            prt(_("確認のため '@' を押して下さい。", "Please verify SUICIDE by typing the '@' sign: "), 0, 0);

            flush();
            i = inkey();
            prt("", 0, 0);
            if (i != '@') {
                return std::nullopt;
            }
            if (w_ptr->total_winner) {
                take_hit(player_ptr, DAMAGE_FORCE, 9999, "Seppuku");
                w_ptr->total_winner = true;
            } else {
                msg_print(_("武士道とは、死ぬことと見つけたり。", "The meaning of bushido is found in death."));
                take_hit(player_ptr, DAMAGE_FORCE, 9999, "Seppuku");
            }
        }
        break;
    }

    return "";
}
