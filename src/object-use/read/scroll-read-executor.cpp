/*
 * @brief 読むことができるアイテム群の内、巻物を読んだ時の効果や処理を記述する.
 * @date 2022/02/26
 * @author Hourier
 */

#include "object-use/read/scroll-read-executor.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "player-base/player-class.h"
#include "player-info/equipment-info.h"
#include "player/attack-defense-types.h"
#include "player/digestion-processor.h"
#include "player/player-damage.h"
#include "player/player-status-flags.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/magic-item-recharger.h"
#include "spell-kind/spells-curse-removal.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-enchant.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-genocide.h"
#include "spell-kind/spells-grid.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-chaos.h"
#include "spell/spells-object.h"
#include "spell/spells-summon.h"
#include "spell/summon-types.h"
#include "status/bad-status-setter.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "store/rumor.h"
#include "sv-definition/sv-scroll-types.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/item-entity.h"
#include "system/redrawing-flags-updater.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

ScrollReadExecutor::ScrollReadExecutor(CreatureEntity &creature, ItemEntity *o_ptr, bool known)
    : creature(creature)
    , o_ptr(o_ptr)
    , known(known)
{
}

bool ScrollReadExecutor::is_identified() const
{
    return this->ident;
}

bool ScrollReadExecutor::read()
{
    auto used_up = true;
    const auto &floor = *this->creature.get_floor();
    switch (*this->o_ptr->bi_key.sval()) {
    case SV_SCROLL_DARKNESS:
        if (!this->creature.has_resist_blind() && !this->creature.has_resist_dark()) {
            (void)BadStatusSetter(this->creature).mod_blindness(3 + randint1(5));
        }

        if (unlite_area(this->creature, 10, 3)) {
            this->ident = true;
        }

        break;
    case SV_SCROLL_AGGRAVATE_MONSTER:
        msg_print(_("カン高くうなる様な音が辺りを覆った。", "There is a high pitched humming noise."));
        aggravate_monsters(this->creature, 0);
        this->ident = true;
        break;
    case SV_SCROLL_CURSE_ARMOR:
        if (curse_armor(this->creature)) {
            this->ident = true;
        }

        break;
    case SV_SCROLL_CURSE_WEAPON: {
        auto k = 0;
        if (has_melee_weapon(this->creature, INVEN_MAIN_HAND)) {
            k = INVEN_MAIN_HAND;
            if (has_melee_weapon(this->creature, INVEN_SUB_HAND) && one_in_(2)) {
                k = INVEN_SUB_HAND;
            }
        } else if (has_melee_weapon(this->creature, INVEN_SUB_HAND)) {
            k = INVEN_SUB_HAND;
        }

        if (k && curse_weapon_object(this->creature, false, *this->creature.inventory[k])) {
            this->ident = true;
        }

        break;
    }
    case SV_SCROLL_SUMMON_MONSTER:
        for (auto k = 0; k < randint1(3); k++) {
            if (summon_specific(this->creature, this->creature.y, this->creature.x, floor.dun_level, SUMMON_NONE,
                    PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET)) {
                this->ident = true;
            }
        }

        break;
    case SV_SCROLL_SUMMON_UNDEAD:
        for (auto k = 0; k < randint1(3); k++) {
            if (summon_specific(this->creature, this->creature.y, this->creature.x, floor.dun_level, SUMMON_UNDEAD,
                    PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET)) {
                this->ident = true;
            }
        }

        break;
    case SV_SCROLL_SUMMON_PET:
        if (summon_specific(
                this->creature, this->creature.y, this->creature.x, floor.dun_level, SUMMON_NONE, PM_ALLOW_GROUP | PM_FORCE_PET)) {
            this->ident = true;
        }

        break;
    case SV_SCROLL_SUMMON_KIN:
        if (summon_kin_player(this->creature, this->creature.get_level(), this->creature.y, this->creature.x, PM_FORCE_PET | PM_ALLOW_GROUP)) {
            this->ident = true;
        }

        break;
    case SV_SCROLL_TRAP_CREATION:
        if (trap_creation(this->creature, this->creature.y, this->creature.x)) {
            this->ident = true;
        }

        break;
    case SV_SCROLL_PHASE_DOOR:
        teleport_player(this->creature, 10, TELEPORT_SPONTANEOUS);
        this->ident = true;
        break;
    case SV_SCROLL_TELEPORT:
        teleport_player(this->creature, 100, TELEPORT_SPONTANEOUS);
        this->ident = true;
        break;
    case SV_SCROLL_TELEPORT_LEVEL: {
        (void)teleport_level(this->creature, 0);
        this->ident = true;
        break;
    }
    case SV_SCROLL_WORD_OF_RECALL:
        if (!recall_player(this->creature, randint0(21) + 15)) {
            used_up = false;
        }

        this->ident = true;
        break;
    case SV_SCROLL_IDENTIFY:
        if (!ident_spell(this->creature, false)) {
            used_up = false;
        }

        this->ident = true;
        break;
    case SV_SCROLL_STAR_IDENTIFY:
        if (!identify_fully(this->creature, false)) {
            used_up = false;
        }

        this->ident = true;
        break;
    case SV_SCROLL_REMOVE_CURSE:
        if (remove_curse(this->creature)) {
            this->ident = true;
        }

        break;
    case SV_SCROLL_STAR_REMOVE_CURSE:
        if (remove_all_curse(this->creature)) {
            this->ident = true;
        }

        break;
    case SV_SCROLL_ENCHANT_ARMOR:
        this->ident = true;
        if (!enchant_spell(this->creature, 0, 0, 1)) {
            used_up = false;
        }

        break;
    case SV_SCROLL_ENCHANT_WEAPON_TO_HIT:
        if (!enchant_spell(this->creature, 1, 0, 0)) {
            used_up = false;
        }

        this->ident = true;
        break;
    case SV_SCROLL_ENCHANT_WEAPON_TO_DAM:
        if (!enchant_spell(this->creature, 0, 1, 0)) {
            used_up = false;
        }

        this->ident = true;
        break;
    case SV_SCROLL_STAR_ENCHANT_ARMOR:
        if (!enchant_spell(this->creature, 0, 0, randint1(3) + 2)) {
            used_up = false;
        }

        this->ident = true;
        break;
    case SV_SCROLL_STAR_ENCHANT_WEAPON:
        if (!enchant_spell(this->creature, randnum1<short>(3), randint1(3), 0)) {
            used_up = false;
        }

        this->ident = true;
        break;
    case SV_SCROLL_RECHARGING:
        if (!recharge(this->creature, 130)) {
            used_up = false;
        }

        this->ident = true;
        break;
    case SV_SCROLL_MUNDANITY:
        this->ident = true;
        if (!mundane_spell(this->creature, false)) {
            used_up = false;
        }

        break;
    case SV_SCROLL_LIGHT:
        if (lite_area(this->creature, Dice::roll(2, 8), 2)) {
            this->ident = true;
        }

        break;

    case SV_SCROLL_MAPPING:
        map_area(this->creature, DETECT_RAD_MAP);
        this->ident = true;
        break;

    case SV_SCROLL_DETECT_GOLD: {
        const auto detected_treasure = detect_treasure(this->creature, DETECT_RAD_DEFAULT);
        const auto detected_gold = detect_objects_gold(this->creature, DETECT_RAD_DEFAULT);

        if (detected_treasure || detected_gold) {
            this->ident = true;
        }
        break;
    }

    case SV_SCROLL_DETECT_ITEM:
        if (detect_objects_normal(this->creature, DETECT_RAD_DEFAULT)) {
            this->ident = true;
        }
        break;

    case SV_SCROLL_DETECT_TRAP:
        if (detect_traps(this->creature, DETECT_RAD_DEFAULT, this->known)) {
            this->ident = true;
        }

        break;
    case SV_SCROLL_DETECT_DOOR: {
        const auto detected_doors = detect_doors(this->creature, DETECT_RAD_DEFAULT);
        const auto detected_stairs = detect_stairs(this->creature, DETECT_RAD_DEFAULT);

        if (detected_doors || detected_stairs) {
            this->ident = true;
        }
    }

    break;
    case SV_SCROLL_DETECT_INVIS:
        if (detect_monsters_invis(this->creature, DETECT_RAD_DEFAULT)) {
            this->ident = true;
        }

        break;
    case SV_SCROLL_SATISFY_HUNGER: {
        if (set_food(this->creature, PY_FOOD_MAX - 1)) {
            this->ident = true;
        }

        break;
    }
    case SV_SCROLL_BLESSING:
        if (set_blessed(this->creature, this->creature.get_timed_effect(CreatureTimedEffect::BLESSED) + randint1(12) + 6, false)) {
            this->ident = true;
        }

        break;
    case SV_SCROLL_HOLY_CHANT:
        if (set_blessed(this->creature, this->creature.get_timed_effect(CreatureTimedEffect::BLESSED) + randint1(24) + 12, false)) {
            this->ident = true;
        }

        break;
    case SV_SCROLL_HOLY_PRAYER:
        if (set_blessed(this->creature, this->creature.get_timed_effect(CreatureTimedEffect::BLESSED) + randint1(48) + 24, false)) {
            this->ident = true;
        }

        break;
    case SV_SCROLL_MONSTER_CONFUSION:
        if (this->creature.has_special_attack(ATTACK_CONFUSE)) {
            break;
        }

        msg_print(_("手が輝き始めた。", "Your hands begin to glow."));
        this->creature.add_special_attack(ATTACK_CONFUSE);
        RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
        this->ident = true;
        break;
    case SV_SCROLL_PROTECTION_FROM_EVIL: {
        const auto k = 3 * this->creature.get_level();
        BodyImprovement improvement(this->creature);
        improvement.mod_protection(randint1(25) + k);
        if (improvement.has_effect()) {
            this->ident = true;
        }

        break;
    }
    case SV_SCROLL_RUNE_OF_PROTECTION:
        create_rune_protection_one(this->creature);
        this->ident = true;
        break;
    case SV_SCROLL_TRAP_DOOR_DESTRUCTION:
        if (destroy_doors_touch(this->creature)) {
            this->ident = true;
        }

        break;
    case SV_SCROLL_STAR_DESTRUCTION:
        if (destroy_area(this->creature, this->creature.y, this->creature.x, 13 + randint0(5), false)) {
            this->ident = true;
        } else {
            msg_print(_("ダンジョンが揺れた...", "The dungeon trembles..."));
        }

        break;
    case SV_SCROLL_DISPEL_UNDEAD:
        if (dispel_undead(this->creature, 80)) {
            this->ident = true;
        }

        break;
    case SV_SCROLL_SPELL:
        if (!CreatureClass(this->creature).has_number_of_spells_learned()) {
            break;
        }

        this->creature.add_spells++;
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::SPELLS);
        this->ident = true;
        break;
    case SV_SCROLL_GENOCIDE:
        (void)symbol_genocide(this->creature, 300, true);
        this->ident = true;
        break;
    case SV_SCROLL_MASS_GENOCIDE:
        (void)mass_genocide(this->creature, 300, true);
        this->ident = true;
        break;
    case SV_SCROLL_ACQUIREMENT:
        acquirement(this->creature, this->creature.y, this->creature.x, this->creature.get_level() / 12 + 1, true);
        this->ident = true;
        break;
    case SV_SCROLL_STAR_ACQUIREMENT:
        acquirement(this->creature, this->creature.y, this->creature.x, this->creature.get_level() / 6 + 3, true);
        this->ident = true;
        break;
    case SV_SCROLL_FIRE:
        fire_ball(this->creature, AttributeType::FIRE, Direction::self(), 666, 4);
        if (!(is_oppose_fire(this->creature) || this->creature.has_resist_fire() || this->creature.has_immune_fire())) {
            take_hit(this->creature, DAMAGE_NOESCAPE, 50 + randint1(50), _("炎の巻物", "a Scroll of Fire"));
        }

        this->ident = true;
        break;
    case SV_SCROLL_ICE:
        fire_ball(this->creature, AttributeType::ICE, Direction::self(), 777, 4);
        if (!(is_oppose_cold(this->creature) || this->creature.has_resist_cold() || this->creature.has_immune_cold())) {
            take_hit(this->creature, DAMAGE_NOESCAPE, 100 + randint1(100), _("氷の巻物", "a Scroll of Ice"));
        }

        this->ident = true;
        break;
    case SV_SCROLL_CHAOS:
        fire_ball(this->creature, AttributeType::CHAOS, Direction::self(), 1000, 4);
        if (!this->creature.has_resist_chaos()) {
            take_hit(this->creature, DAMAGE_NOESCAPE, 111 + randint1(111), _("ログルスの巻物", "a Scroll of Logrus"));
        }

        this->ident = true;
        break;
    case SV_SCROLL_RUMOR:
        msg_print(_("巻物にはメッセージが書かれている:", "There is message on the scroll. It says:"));
        msg_erase();
        display_rumor(this->creature, true);
        msg_erase();
        msg_print(_("巻物は煙を立てて消え去った！", "The scroll disappears in a puff of smoke!"));
        this->ident = true;
        break;
    case SV_SCROLL_ARTIFACT:
        this->ident = true;
        if (!artifact_scroll(this->creature)) {
            used_up = false;
        }

        break;
    case SV_SCROLL_RESET_RECALL:
        this->ident = true;
        if (!reset_recall(this->creature)) {
            used_up = false;
        }

        break;
    case SV_SCROLL_AMUSEMENT:
        this->ident = true;
        generate_amusement(this->creature, 1, false);
        break;
    case SV_SCROLL_STAR_AMUSEMENT:
        this->ident = true;
        generate_amusement(this->creature, randint1(2) + 1, false);
        break;
    case SV_SCROLL_HUGE_EARTHQUAKE: {
        this->ident = true;
        earthquake(this->creature, this->creature.get_position(), randint1(20) + 50, 0);
        break;
    }
    case SV_SCROLL_CALL_THE_VOID: {
        this->ident = true;
        call_the_void(this->creature);
        break;
    }
    case SV_SCROLL_THUNDER: {
        fire_ball(this->creature, AttributeType::ELEC, Direction::self(), 888, 4);
        if (!(is_oppose_elec(this->creature) || this->creature.has_resist_elec() || this->creature.has_immune_elec())) {
            take_hit(this->creature, DAMAGE_NOESCAPE, 100 + randint1(100), _("雷の巻物", "a Scroll of Thunder"));
        }
        this->ident = true;
        break;
    }
    case SV_SCROLL_POWERFUL_EYE_SENIOR: {
        for (int k = 0; k < 20; k++) {
            summon_specific(this->creature, this->creature.y, this->creature.x, 50, SUMMON_POWERFUL_EYE_SENIOR, 0);
        }
        this->ident = true;
        break;
    }
    case SV_SCROLL_TREE_CREATION: {
        tree_creation(this->creature, this->creature.y, this->creature.x);
        break;
    }
    default:
        break;
    }

    return used_up;
}
