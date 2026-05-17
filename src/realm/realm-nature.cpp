#include "realm/realm-nature.h"
#include "avatar/avatar.h"
#include "cmd-action/cmd-spell.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "effect/spells-effect-util.h"
#include "floor/floor-object.h"
#include "hpmp/hp-mp-processor.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "player-attack/player-attack.h"
#include "player-base/player-race.h"
#include "player-info/race-info.h"
#include "player-info/race-types.h"
#include "player/player-damage.h"
#include "player/player-status-flags.h"
#include "realm/realm-types.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/spells-beam.h"
#include "spell-kind/spells-charm.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-grid.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-sight.h"
#include "spell-realm/spells-nature.h"
#include "spell/spells-diceroll.h"
#include "spell/spells-object.h"
#include "spell/spells-status.h"
#include "spell/summon-types.h"
#include "status/bad-status-setter.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "sv-definition/sv-food-types.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "target/target-getter.h"
#include "util/dice.h"
#include "view/display-messages.h"

/*!
 * @brief 自然領域魔法の各処理を行う
 * @param creature クリーチャーへの参照
 * @param spell 魔法ID
 * @param mode 処理内容 (SpellProcessType::NAME / SPELL_DESC / SpellProcessType::INFO / SpellProcessType::CAST)
 * @return SpellProcessType::NAME / SPELL_DESC / SpellProcessType::INFO 時には文字列を返す。SpellProcessType::CAST時は tl::nullopt を返す。
 */
tl::optional<std::string> do_nature_spell(CreatureEntity &creature, SPELL_IDX spell, SpellProcessType mode)
{
    bool info = mode == SpellProcessType::INFO;
    bool cast = mode == SpellProcessType::CAST;

    PLAYER_LEVEL plev = creature.get_level();

    switch (spell) {
    case 0: {
        POSITION rad = DETECT_RAD_DEFAULT;

        if (info) {
            return info_radius(rad);
        }

        if (cast) {
            detect_monsters_normal(creature, rad);
        }
    } break;

    case 1: {
        const Dice dice(3 + (plev - 1) / 5, 4);
        POSITION range = plev / 6 + 2;

        if (info) {
            return format("%s%s %s%d", KWD_DAM, dice.to_string().data(), KWD_RANGE, range);
        }

        if (cast) {
            project_length = range;

            const auto dir = get_aim_dir(creature);
            if (!dir) {
                return tl::nullopt;
            }

            fire_beam(creature, AttributeType::ELEC, dir, dice.roll());
        }
    } break;

    case 2: {
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

    case 3: {
        if (cast) {
            msg_print(_("食料を生成した。", "A food ration is produced."));
            ItemEntity item({ ItemKindType::FOOD, SV_FOOD_RATION });
            (void)drop_near(creature, item, creature.get_position());
        }
    } break;

    case 4: {
        const Dice dice(2, plev / 2);
        POSITION rad = (plev / 10) + 1;

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            lite_area(creature, dice.roll(), rad);

            CreatureRace race(&creature);
            if (race.life() == PlayerRaceLifeType::UNDEAD && race.tr_flags().has(TR_VUL_LITE) && !creature.has_resist_lite()) {
                msg_print(_("日の光があなたの肉体を焦がした！", "The daylight scorches your flesh!"));
                take_hit(creature, DAMAGE_NOESCAPE, Dice::roll(2, 2), _("日の光", "daylight"));
            }
        }
    } break;

    case 5: {
        int power = plev;

        if (info) {
            return info_power(power);
        }

        if (cast) {
            const auto dir = get_aim_dir(creature);
            if (!dir) {
                return tl::nullopt;
            }

            charm_animal(creature, dir, plev);
        }
    } break;

    case 6: {
        int base = 20;
        const Dice dice(1, 20);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_oppose_cold(creature, dice.roll() + base, false);
            set_oppose_fire(creature, dice.roll() + base, false);
            set_oppose_elec(creature, dice.roll() + base, false);
        }
    } break;

    case 7: {
        const Dice dice(2, 8);

        if (info) {
            return info_heal(dice);
        }

        if (cast) {
            BadStatusSetter bss(creature);
            hp_player(creature, dice.roll());
            (void)bss.set_cut(0);
            (void)bss.set_poison(0);
        }
    } break;

    case 8: {
        const Dice dice(1, 30);
        int base = 20;

        if (info) {
            return info_damage(dice, base);
        }

        if (cast) {
            const auto dir = get_aim_dir(creature);
            if (!dir) {
                return tl::nullopt;
            }

            wall_to_mud(creature, dir, base + dice.roll());
        }
    } break;

    case 9: {
        const Dice dice(3 + (plev - 5) / 4, 8);

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            const auto dir = get_aim_dir(creature);
            if (!dir) {
                return tl::nullopt;
            }
            fire_bolt_or_beam(creature, beam_chance(creature) - 10, AttributeType::COLD, dir, dice.roll());
        }
    } break;

    case 10: {
        int rad1 = DETECT_RAD_MAP;
        int rad2 = DETECT_RAD_DEFAULT;

        if (info) {
            return info_radius(std::max(rad1, rad2));
        }

        if (cast) {
            map_area(creature, rad1);
            detect_traps(creature, rad2, true);
            detect_doors(creature, rad2);
            detect_stairs(creature, rad2);
            detect_monsters_normal(creature, rad2);
        }
    } break;

    case 11: {
        const Dice dice(5 + (plev - 5) / 4, 8);

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            const auto dir = get_aim_dir(creature);
            if (!dir) {
                return tl::nullopt;
            }
            fire_bolt_or_beam(creature, beam_chance(creature) - 10, AttributeType::FIRE, dir, dice.roll());
        }
    } break;

    case 12: {
        const Dice dice(6, 8);

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            const auto dir = get_aim_dir(creature);
            if (!dir) {
                return tl::nullopt;
            }
            msg_print(_("太陽光線が現れた。", "A line of sunlight appears."));
            lite_line(creature, dir, dice.roll());
        }
    } break;

    case 13: {
        int power = plev;
        if (info) {
            return info_power(power);
        }
        if (cast) {
            slow_monsters(creature, plev);
        }
    } break;

    case 14: {
        if (cast) {
            if (!(summon_specific(creature, creature.y, creature.x, plev, SUMMON_ANIMAL_RANGER, (PM_ALLOW_GROUP | PM_FORCE_PET)))) {
                msg_print(_("動物は現れなかった。", "No animals arrive."));
            }
            break;
        }
    } break;

    case 15: {
        int heal = 500;
        if (info) {
            return info_heal(heal);
        }
        if (cast) {
            (void)cure_critical_wounds(creature, heal);
        }
    } break;

    case 16: {
        if (cast) {
            stair_creation(creature);
        }
    } break;

    case 17: {
        int base = 20;
        const Dice dice(1, 30);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_shield(creature, dice.roll() + base, false);
        }
    } break;

    case 18: {
        int base = 20;
        const Dice dice(1, 20);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_oppose_acid(creature, dice.roll() + base, false);
            set_oppose_elec(creature, dice.roll() + base, false);
            set_oppose_fire(creature, dice.roll() + base, false);
            set_oppose_cold(creature, dice.roll() + base, false);
            set_oppose_pois(creature, dice.roll() + base, false);
        }
    } break;

    case 19: {
        if (cast) {
            tree_creation(creature, creature.y, creature.x);
        }
    } break;

    case 20: {
        int power = plev * 2;
        if (info) {
            return info_power(power);
        }
        if (cast) {
            charm_animals(creature, power);
        }
    } break;

    case 21: {
        if (cast) {
            if (!identify_fully(creature, false)) {
                return tl::nullopt;
            }
        }
    } break;

    case 22: {
        if (cast) {
            wall_stone(creature);
        }
    } break;

    case 23: {
        if (cast) {
            if (!rustproof(creature)) {
                return tl::nullopt;
            }
        }
    } break;

    case 24: {
        POSITION rad = 10;

        if (info) {
            return info_radius(rad);
        }

        if (cast) {
            earthquake(creature, creature.get_position(), rad);
        }
    } break;

    case 25:
        if (cast) {
            massacre(creature);
        }
        break;

    case 26: {
        int dam = 70 + plev * 3 / 2;
        POSITION rad = plev / 12 + 1;

        if (info) {
            return info_damage(dam);
        }

        if (cast) {
            const auto dir = get_aim_dir(creature);
            if (!dir) {
                return tl::nullopt;
            }

            fire_ball(creature, AttributeType::COLD, dir, dam, rad);
        }
    } break;

    case 27: {
        int dam = 90 + plev * 3 / 2;
        POSITION rad = plev / 12 + 1;

        if (info) {
            return info_damage(dam);
        }

        if (cast) {
            const auto dir = get_aim_dir(creature);
            if (!dir) {
                return tl::nullopt;
            }
            fire_ball(creature, AttributeType::ELEC, dir, dam, rad);
            break;
        }
    } break;

    case 28: {
        int dam = 100 + plev * 3 / 2;
        POSITION rad = plev / 12 + 1;

        if (info) {
            return info_damage(dam);
        }

        if (cast) {
            const auto dir = get_aim_dir(creature);
            if (!dir) {
                return tl::nullopt;
            }
            fire_ball(creature, AttributeType::WATER, dir, dam, rad);
        }
    } break;

    case 29: {
        int dam = 150;
        POSITION rad = 8;

        if (info) {
            return info_damage(dam / 2);
        }

        if (cast) {
            fire_ball(creature, AttributeType::LITE, Direction::self(), dam, rad);
            chg_virtue(creature, Virtue::KNOWLEDGE, 1);
            chg_virtue(creature, Virtue::ENLIGHTEN, 1);
            wiz_lite(creature, false);

            CreatureRace race(&creature);
            if (race.life() == PlayerRaceLifeType::UNDEAD && race.tr_flags().has(TR_VUL_LITE) && !creature.has_resist_lite()) {
                msg_print(_("日光があなたの肉体を焦がした！", "The sunlight scorches your flesh!"));
                take_hit(creature, DAMAGE_NOESCAPE, 50, _("日光", "sunlight"));
            }
        }
    } break;

    case 30: {
        if (cast) {
            brand_weapon(creature, randint0(2));
        }
    } break;

    case 31: {
        int d_dam = 4 * plev;
        int b_dam = (100 + plev) * 2;
        POSITION b_rad = 1 + plev / 12;
        POSITION q_rad = 20 + plev / 2;

        if (info) {
            return format("%s%d+%d", KWD_DAM, d_dam, b_dam / 2);
        }

        if (cast) {
            dispel_monsters(creature, d_dam);
            earthquake(creature, creature.get_position(), q_rad);
            project(creature, 0, b_rad, creature.y, creature.x, b_dam, AttributeType::DISINTEGRATE, PROJECT_KILL | PROJECT_ITEM);
        }
    } break;
    }

    return "";
}
