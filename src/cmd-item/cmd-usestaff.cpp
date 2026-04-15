#include "cmd-item/cmd-usestaff.h"
#include "action/action-limited.h"
#include "floor/floor-object.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "object-use/use-execution.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "player-info/race-types.h"
#include "player-info/samurai-data-type.h"
#include "player/attack-defense-types.h"
#include "player/player-status-flags.h"
#include "player/special-defense-types.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/spells-curse-removal.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-genocide.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell/spells-staff-only.h"
#include "spell/spells-status.h"
#include "spell/spells-summon.h"
#include "spell/summon-types.h"
#include "status/action-setter.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/buff-setter.h"
#include "status/shape-changer.h"
#include "status/sight-setter.h"
#include "sv-definition/sv-staff-types.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "util/dice.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief 杖の効果を発動する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param sval オブジェクトのsval
 * @param use_charge 使用回数を消費したかどうかを返す参照ポインタ
 * @param powerful 強力発動上の処理ならばTRUE
 * @param magic 魔道具術上の処理ならばTRUE
 * @param known 判明済ならばTRUE
 * @return 発動により効果内容が確定したならばTRUEを返す
 */
int staff_effect(CreatureEntity &creature, int sval, bool *use_charge, bool powerful, bool magic, bool known)
{
    int k;
    bool ident = false;
    PLAYER_LEVEL lev = powerful ? creature.level * 2 : creature.level;
    POSITION detect_rad = powerful ? DETECT_RAD_DEFAULT * 3 / 2 : DETECT_RAD_DEFAULT;

    creature.plus_incident_tree("ZAP_STAFF", 1);

    /* Analyze the staff */
    BadStatusSetter bss(creature);
    switch (sval) {
    case SV_STAFF_DARKNESS:
        if (!has_resist_blind(creature) && !has_resist_dark(creature)) {
            if (bss.mod_blindness(3 + randint1(5))) {
                ident = true;
            }
        }

        if (unlite_area(creature, 10, (powerful ? 6 : 3))) {
            ident = true;
        }

        break;
    case SV_STAFF_SLOWNESS: {
        if (bss.mod_deceleration(randint1(30) + 15, false)) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_HASTE_MONSTERS: {
        if (speed_monsters(creature)) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_SUMMONING: {
        const int times = randint1(powerful ? 8 : 4);
        for (k = 0; k < times; k++) {
            if (summon_specific(creature, creature.y, creature.x, creature.get_floor()->dun_level, SUMMON_NONE,
                    (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET))) {
                ident = true;
            }
        }
        break;
    }

    case SV_STAFF_TELEPORTATION: {
        teleport_player(creature, (powerful ? 150 : 100), 0L);
        ident = true;
        break;
    }

    case SV_STAFF_IDENTIFY: {
        if (powerful) {
            if (!identify_fully(creature, false)) {
                *use_charge = false;
            }
        } else {
            if (!ident_spell(creature, false)) {
                *use_charge = false;
            }
        }
        ident = true;
        break;
    }

    case SV_STAFF_REMOVE_CURSE: {
        bool result = (powerful ? remove_all_curse(creature) : remove_curse(creature)) != 0;
        if (result) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_STARLITE:
        ident = starlight(creature, magic);
        break;

    case SV_STAFF_LITE: {
        if (lite_area(creature, Dice::roll(2, 8), (powerful ? 4 : 2))) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_MAPPING: {
        map_area(creature, powerful ? DETECT_RAD_MAP * 3 / 2 : DETECT_RAD_MAP);
        ident = true;
        break;
    }

    case SV_STAFF_DETECT_GOLD: {
        if (detect_treasure(creature, detect_rad)) {
            ident = true;
        }
        if (detect_objects_gold(creature, detect_rad)) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_DETECT_ITEM: {
        if (detect_objects_normal(creature, detect_rad)) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_DETECT_TRAP: {
        if (detect_traps(creature, detect_rad, known)) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_DETECT_DOOR: {
        if (detect_doors(creature, detect_rad)) {
            ident = true;
        }
        if (detect_stairs(creature, detect_rad)) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_DETECT_INVIS: {
        if (set_tim_invis(creature, creature.get_timed_effect(CreatureTimedEffect::TIM_INVIS) + 12 + randint1(12), false)) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_DETECT_EVIL: {
        if (detect_monsters_evil(creature, detect_rad)) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_CURE_LIGHT: {
        ident = cure_light_wounds(creature, Dice::roll(powerful ? 4 : 2, 8));
        break;
    }

    case SV_STAFF_CURING: {
        ident = true_healing(creature, 0);
        if (set_berserk(creature, 0, true)) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_HEALING: {
        if (cure_critical_wounds(creature, powerful ? 500 : 300)) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_THE_MAGI: {
        if (do_res_stat(creature, A_INT)) {
            ident = true;
        }
        ident |= restore_mana(creature, false);
        if (set_berserk(creature, 0, true)) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_SLEEP_MONSTERS: {
        if (sleep_monsters(creature, lev)) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_SLOW_MONSTERS: {
        if (slow_monsters(creature, lev)) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_SPEED: {
        if (set_acceleration(creature, randint1(30) + (powerful ? 30 : 15), false)) {
            ident = true;
        }
        break;
    }

    case SV_STAFF_PROBING: {
        ident = probing(creature);
        break;
    }

    case SV_STAFF_DISPEL_EVIL: {
        ident = dispel_evil(creature, powerful ? 120 : 80);
        break;
    }

    case SV_STAFF_POWER: {
        ident = dispel_monsters(creature, powerful ? 225 : 150);
        break;
    }

    case SV_STAFF_HOLINESS: {
        ident = cleansing_nova(creature, magic, powerful);
        break;
    }

    case SV_STAFF_GENOCIDE: {
        ident = symbol_genocide(creature, (magic ? lev + 50 : 200), true);
        break;
    }

    case SV_STAFF_EARTHQUAKES: {
        if (earthquake(creature, creature.get_position(), (powerful ? 15 : 10))) {
            ident = true;
        } else {
            msg_print(_("ダンジョンが揺れた。", "The dungeon trembles."));
        }

        break;
    }

    case SV_STAFF_DESTRUCTION: {
        ident = destroy_area(creature, creature.y, creature.x, (powerful ? 18 : 13) + randint0(5), false);
        break;
    }

    case SV_STAFF_ANIMATE_DEAD: {
        ident = animate_dead(creature, 0, creature.y, creature.x);
        break;
    }

    case SV_STAFF_MSTORM: {
        ident = unleash_mana_storm(creature, powerful);
        break;
    }

    case SV_STAFF_NOTHING: {
        msg_print(_("何も起らなかった。", "Nothing happens."));
        if (CreatureRace(&creature).food() == PlayerRaceFoodType::MANA) {
            msg_print(_("もったいない事をしたような気がする。食べ物は大切にしなくては。", "What a waste.  It's your food!"));
        }
        break;
    }
    }
    return ident;
}

/*!
 * @brief 杖を使うコマンドのメインルーチン /
 */
void do_cmd_use_staff(CreatureEntity &creature)
{
    if (AngbandWorld::get_instance().is_wild_mode()) {
        return;
    }

    if (cmd_limit_arena(creature)) {
        return;
    }

    CreatureClass(creature).break_samurai_stance({ SamuraiStanceType::MUSOU, SamuraiStanceType::KOUKIJIN });
    constexpr auto q = _("どの杖を使いますか? ", "Use which staff? ");
    constexpr auto s = _("使える杖がない。", "You have no staff to use.");
    short i_idx;
    if (!choose_object(creature, &i_idx, q, s, (USE_INVEN | USE_FLOOR), TvalItemTester(ItemKindType::STAFF))) {
        return;
    }

    ObjectUseEntity(creature, i_idx).execute();
}
