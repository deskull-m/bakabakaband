#include "realm/realm-life.h"
#include "cmd-action/cmd-spell.h"
#include "effect/attribute-types.h"
#include "player/digestion-processor.h"
#include "realm/realm-types.h"
#include "spell-kind/spells-curse-removal.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-genocide.h"
#include "spell-kind/spells-grid.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-world.h"
#include "spell/spells-status.h"
#include "status/bad-status-setter.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "status/experience.h"
#include "status/temporary-resistance.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "target/target-getter.h"
#include "util/dice.h"

/*!
 * @brief 生命領域魔法の各処理を行う
 * @param creature クリーチャーへの参照
 * @param spell 魔法ID
 * @param mode 処理内容 (SpellProcessType::NAME / SPELL_DESC / SpellProcessType::INFO / SpellProcessType::CAST)
 * @return SpellProcessType::NAME / SPELL_DESC / SpellProcessType::INFO 時には文字列を返す。SpellProcessType::CAST時は tl::nullopt を返す。
 */
tl::optional<std::string> do_life_spell(CreatureEntity &creature, SPELL_IDX spell, SpellProcessType mode)
{
    bool info = mode == SpellProcessType::INFO;
    bool cast = mode == SpellProcessType::CAST;

    PLAYER_LEVEL plev = creature.get_level();

    switch (spell) {
    case 0: {
        const Dice dice(2, 10);
        if (info) {
            return info_heal(dice);
        }
        if (cast) {
            (void)cure_light_wounds(creature, dice.roll());
        }
    } break;

    case 1: {
        int base = 12;
        const Dice dice(1, 12);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_blessed(creature, dice.roll() + base, false);
        }
    } break;

    case 2: {
        const Dice dice(3 + (plev - 1) / 5, 4);

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            const auto dir = get_aim_dir(creature);
            if (!dir) {
                return tl::nullopt;
            }
            fire_ball_hide(creature, AttributeType::WOUNDS, dir, dice.roll(), 0);
        }
    } break;

    case 3: {
        const Dice dice(2, plev / 2);
        POSITION rad = plev / 10 + 1;

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            lite_area(creature, dice.roll(), rad);
        }
    } break;

    case 4: {
        POSITION rad = DETECT_RAD_DEFAULT;

        if (info) {
            return info_radius(rad);
        }

        if (cast) {
            detect_traps(creature, rad, true);
            detect_doors(creature, rad);
            detect_stairs(creature, rad);
        }
    } break;

    case 5: {
        const Dice dice(4, 10);

        if (info) {
            return info_heal(dice);
        }
        if (cast) {
            (void)cure_serious_wounds(creature, dice.roll());
        }
    } break;

    case 6:
        if (cast) {
            (void)BadStatusSetter(creature).set_poison(0);
        }

        break;

    case 7: {
        if (cast) {
            set_food(creature, PY_FOOD_MAX - 1);
        }
    } break;

    case 8: {
        if (cast) {
            (void)remove_curse(creature);
        }
    } break;

    case 9: {
        const Dice dice(8 + (plev - 5) / 4, 8);

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            const auto dir = get_aim_dir(creature);
            if (!dir) {
                return tl::nullopt;
            }
            fire_ball_hide(creature, AttributeType::WOUNDS, dir, dice.roll(), 0);
        }
    } break;

    case 10: {
        const Dice dice(8, 10);

        if (info) {
            return info_heal(dice);
        }
        if (cast) {
            (void)cure_critical_wounds(creature, dice.roll());
        }
    } break;

    case 11: {
        int base = 20;
        const Dice dice(1, 20);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_oppose_cold(creature, dice.roll() + base, false);
            set_oppose_fire(creature, dice.roll() + base, false);
        }
    } break;

    case 12: {
        POSITION rad = DETECT_RAD_MAP;

        if (info) {
            return info_radius(rad);
        }

        if (cast) {
            map_area(creature, rad);
        }
    } break;

    case 13: {
        if (cast) {
            turn_undead(creature);
        }
    } break;

    case 14: {
        int heal = 300;
        if (info) {
            return info_heal(heal);
        }
        if (cast) {
            (void)cure_critical_wounds(creature, heal);
        }
    } break;

    case 15: {
        if (cast) {
            create_rune_protection_one(creature);
        }
    } break;

    case 16: {
        if (cast) {
            (void)remove_all_curse(creature);
        }
    } break;

    case 17: {
        if (cast) {
            if (!ident_spell(creature, false)) {
                return tl::nullopt;
            }
        }
    } break;

    case 18: {
        const Dice dice(1, plev * 5);

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            dispel_undead(creature, dice.roll());
        }
    } break;

    case 19: {
        int power = plev * 2;

        if (info) {
            return info_power(power);
        }

        if (cast) {
            charm_monsters(creature, power);
        }
    } break;

    case 20: {
        const Dice dice(5 + (plev - 5) / 3, 15);

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            const auto dir = get_aim_dir(creature);
            if (!dir) {
                return tl::nullopt;
            }
            fire_ball_hide(creature, AttributeType::WOUNDS, dir, dice.roll(), 0);
        }
    } break;

    case 21: {
        int base = 15;
        const Dice dice(1, 20);

        if (info) {
            return info_delay(base, dice);
        }

        if (cast) {
            if (!recall_player(creature, dice.roll() + base)) {
                return tl::nullopt;
            }
        }
    } break;

    case 22: {
        int base = 15;
        const Dice dice(1, 20);

        if (info) {
            return info_delay(base, dice);
        }

        if (cast) {
            reserve_alter_reality(creature, dice.roll() + base);
        }
    } break;

    case 23: {
        POSITION rad = 1;

        if (info) {
            return info_radius(rad);
        }

        if (cast) {
            create_rune_protection_one(creature);
            create_rune_protection_area(creature, creature.y, creature.x);
        }
    } break;

    case 24: {
        if (cast) {
            creature.get_floor()->num_repro += MAX_REPRODUCTION;
        }
    } break;

    case 25: {
        POSITION rad = DETECT_RAD_DEFAULT;

        if (info) {
            return info_radius(rad);
        }

        if (cast) {
            detect_all(creature, rad);
        }
    } break;

    case 26: {
        int power = plev + 50;

        if (info) {
            return info_power(power);
        }

        if (cast) {
            mass_genocide_undead(creature, power, true);
        }
    } break;

    case 27: {
        if (cast) {
            wiz_lite(creature, false);
        }
    } break;

    case 28: {
        if (cast) {
            (void)restore_all_status(creature);
            restore_level(creature);
        }
    } break;

    case 29: {
        int heal = 2000;
        if (info) {
            return info_heal(heal);
        }
        if (cast) {
            (void)cure_critical_wounds(creature, heal);
        }
    } break;

    case 30: {
        if (cast) {
            if (!identify_fully(creature, false)) {
                return tl::nullopt;
            }
        }
    } break;

    case 31: {
        TIME_EFFECT base = (TIME_EFFECT)plev / 2;
        const Dice dice(1, plev / 2);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            const auto v = static_cast<TIME_EFFECT>(dice.roll() + base);
            set_acceleration(creature, v, false);
            set_oppose_acid(creature, v, false);
            set_oppose_elec(creature, v, false);
            set_oppose_fire(creature, v, false);
            set_oppose_cold(creature, v, false);
            set_oppose_pois(creature, v, false);
            set_ultimate_res(creature, v, false);
        }
    } break;
    }

    return "";
}
