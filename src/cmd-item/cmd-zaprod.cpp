#include "cmd-item/cmd-zaprod.h"
#include "action/action-limited.h"
#include "effect/attribute-types.h"
#include "floor/floor-object.h"
#include "floor/geometry.h"
#include "object-enchant/special-object-flags.h"
#include "object-use/zaprod-execution.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player-info/samurai-data-type.h"
#include "player/attack-defense-types.h"
#include "player/special-defense-types.h"
#include "spell-kind/spells-beam.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-random.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-specific-bolt.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell/spells-status.h"
#include "status/action-setter.h"
#include "status/buff-setter.h"
#include "status/experience.h"
#include "status/shape-changer.h"
#include "sv-definition/sv-rod-types.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief ロッドの効果を発動する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param sval オブジェクトのsval
 * @param dir 発動目標の方向ID
 * @param use_charge チャージを消費したかどうかを返す参照ポインタ
 * @param powerful 強力発動上の処理ならばTRUE
 * @return 発動により効果内容が確定したならばTRUEを返す
 */
int rod_effect(CreatureEntity &creature, int sval, const Direction &dir, bool *use_charge, bool powerful)
{
    auto &player = static_cast<PlayerType &>(creature);
    int ident = false;
    PLAYER_LEVEL lev = powerful ? player.level * 2 : player.level;
    POSITION detect_rad = powerful ? DETECT_RAD_DEFAULT * 3 / 2 : DETECT_RAD_DEFAULT;
    POSITION rad = powerful ? 3 : 2;

    player.plus_incident_tree("ZAP_ROD", 1);

    switch (sval) {
    case SV_ROD_DETECT_TRAP: {
        if (detect_traps(player, detect_rad, !dir)) {
            ident = true;
        }
        break;
    }

    case SV_ROD_DETECT_DOOR: {
        if (detect_doors(player, detect_rad)) {
            ident = true;
        }
        if (detect_stairs(player, detect_rad)) {
            ident = true;
        }
        break;
    }

    case SV_ROD_IDENTIFY: {
        if (powerful) {
            if (!identify_fully(player, false)) {
                *use_charge = false;
            }
        } else {
            if (!ident_spell(player, false)) {
                *use_charge = false;
            }
        }
        ident = true;
        break;
    }

    case SV_ROD_RECALL: {
        if (!recall_player(player, randint0(21) + 15)) {
            *use_charge = false;
        }
        ident = true;
        break;
    }

    case SV_ROD_ILLUMINATION: {
        if (lite_area(player, Dice::roll(2, 8), (powerful ? 4 : 2))) {
            ident = true;
        }
        break;
    }

    case SV_ROD_MAPPING: {
        map_area(player, powerful ? DETECT_RAD_MAP * 3 / 2 : DETECT_RAD_MAP);
        ident = true;
        break;
    }

    case SV_ROD_DETECTION: {
        detect_all(player, detect_rad);
        ident = true;
        break;
    }

    case SV_ROD_PROBING: {
        probing(player);
        ident = true;
        break;
    }

    case SV_ROD_CURING: {
        if (true_healing(player, 0)) {
            ident = true;
        }
        if (set_berserk(player, 0, true)) {
            ident = true;
        }
        break;
    }

    case SV_ROD_HEALING: {
        if (cure_critical_wounds(player, powerful ? 750 : 500)) {
            ident = true;
        }
        break;
    }

    case SV_ROD_RESTORATION: {
        if (restore_level(creature)) {
            ident = true;
        }
        if (restore_all_status(player)) {
            ident = true;
        }
        break;
    }

    case SV_ROD_SPEED: {
        if (set_acceleration(player, randint1(30) + (powerful ? 30 : 15), false)) {
            ident = true;
        }
        break;
    }

    case SV_ROD_PESTICIDE: {
        if (dispel_monsters(player, powerful ? 8 : 4)) {
            ident = true;
        }
        break;
    }

    case SV_ROD_TELEPORT_AWAY: {
        int distance = MAX_PLAYER_SIGHT * (powerful ? 8 : 5);
        if (teleport_monster(player, dir, distance)) {
            ident = true;
        }
        break;
    }

    case SV_ROD_DISARMING: {
        if (disarm_trap(player, dir)) {
            ident = true;
        }
        if (powerful && disarm_traps_touch(player)) {
            ident = true;
        }
        break;
    }

    case SV_ROD_LITE: {
        int dam = Dice::roll((powerful ? 12 : 6), 8);
        msg_print(_("青く輝く光線が放たれた。", "A line of blue shimmering light appears."));
        (void)lite_line(player, dir, dam);
        ident = true;
        break;
    }

    case SV_ROD_SLEEP_MONSTER: {
        if (sleep_monster(player, dir, lev)) {
            ident = true;
        }
        break;
    }

    case SV_ROD_SLOW_MONSTER: {
        if (slow_monster(player, dir, lev)) {
            ident = true;
        }
        break;
    }

    case SV_ROD_HYPODYNAMIA: {
        if (hypodynamic_bolt(player, dir, 70 + 3 * lev / 2)) {
            ident = true;
        }
        break;
    }

    case SV_ROD_POLYMORPH: {
        if (poly_monster(player, dir, lev)) {
            ident = true;
        }
        break;
    }

    case SV_ROD_ACID_BOLT: {
        fire_bolt_or_beam(player, 10, AttributeType::ACID, dir, Dice::roll(6 + lev / 7, 8));
        ident = true;
        break;
    }

    case SV_ROD_ELEC_BOLT: {
        fire_bolt_or_beam(player, 10, AttributeType::ELEC, dir, Dice::roll(4 + lev / 9, 8));
        ident = true;
        break;
    }

    case SV_ROD_FIRE_BOLT: {
        fire_bolt_or_beam(player, 10, AttributeType::FIRE, dir, Dice::roll(7 + lev / 6, 8));
        ident = true;
        break;
    }

    case SV_ROD_COLD_BOLT: {
        fire_bolt_or_beam(player, 10, AttributeType::COLD, dir, Dice::roll(5 + lev / 8, 8));
        ident = true;
        break;
    }

    case SV_ROD_ACID_BALL: {
        fire_ball(player, AttributeType::ACID, dir, 60 + lev, rad);
        ident = true;
        break;
    }

    case SV_ROD_ELEC_BALL: {
        fire_ball(player, AttributeType::ELEC, dir, 40 + lev, rad);
        ident = true;
        break;
    }

    case SV_ROD_FIRE_BALL: {
        fire_ball(player, AttributeType::FIRE, dir, 70 + lev, rad);
        ident = true;
        break;
    }

    case SV_ROD_COLD_BALL: {
        fire_ball(player, AttributeType::COLD, dir, 50 + lev, rad);
        ident = true;
        break;
    }

    case SV_ROD_HAVOC: {
        call_chaos(player);
        ident = true;
        break;
    }

    case SV_ROD_STONE_TO_MUD: {
        int dam = powerful ? 40 + randint1(60) : 20 + randint1(30);
        if (wall_to_mud(player, dir, dam)) {
            ident = true;
        }
        break;
    }

    case SV_ROD_AGGRAVATE: {
        aggravate_monsters(player, 0);
        ident = true;
        break;
    }
    }
    return ident;
}

/*!
 * @brief ロッドを使うコマンドのメインルーチン /
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_zap_rod(CreatureEntity &creature)
{
    auto &player = static_cast<PlayerType &>(creature);
    if (AngbandWorld::get_instance().is_wild_mode()) {
        return;
    }

    if (cmd_limit_arena(creature)) {
        return;
    }

    CreatureClass(creature).break_samurai_stance({ SamuraiStanceType::MUSOU, SamuraiStanceType::KOUKIJIN });

    constexpr auto q = _("どのロッドを振りますか? ", "Zap which rod? ");
    constexpr auto s = _("使えるロッドがない。", "You have no rod to zap.");
    short i_idx;
    if (!choose_object(player, &i_idx, q, s, (USE_INVEN | USE_FLOOR), TvalItemTester(ItemKindType::ROD))) {
        return;
    }

    ObjectZapRodEntity(player).execute(i_idx);
}
