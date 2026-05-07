/*!
 * @brief プレイヤーの発動コマンド実装
 * @date 2018/09/07
 * @author deskull
 */

#include "object-activation/activation-switcher.h"
#include "artifact/random-art-effects.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "object-activation/activation-bolt-ball.h"
#include "object-activation/activation-breath.h"
#include "object-activation/activation-charm.h"
#include "object-activation/activation-genocide.h"
#include "object-activation/activation-others.h"
#include "object-activation/activation-resistance.h"
#include "object-activation/activation-teleport.h"
#include "object-enchant/activation-info-table.h"
#include "player/digestion-processor.h"
#include "player/player-damage.h"
#include "specific-object/blade-turner.h"
#include "specific-object/bloody-moon.h"
#include "specific-object/death-crimson.h"
#include "specific-object/monster-ball.h"
#include "specific-object/muramasa.h"
#include "specific-object/ring-of-power.h"
#include "specific-object/stone-of-lore.h"
#include "specific-object/toragoroshi.h"
#include "spell-kind/spells-floor.h"
#include "spell-realm/spells-sorcery.h"
#include "spell/spells-object.h"
#include "spell/spells-status.h"
#include "spell/spells-summon.h"
#include "spell/summon-types.h"
#include "status/bad-status-setter.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "status/experience.h"
#include "status/shape-changer.h"
#include "status/sight-setter.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "util/dice.h"
#include "view/display-messages.h"

bool switch_activation(CreatureEntity &creature, ItemEntity **o_ptr_ptr, const RandomArtActType index, std::string_view name)
{
    auto *o_ptr = *o_ptr_ptr;
    switch (index) {
    case RandomArtActType::SUNLIGHT:
        return activate_sunlight(creature);
    case RandomArtActType::BO_MISS_1:
        return activate_missile_1(creature);
    case RandomArtActType::BA_POIS_1:
        return activate_ball_pois_1(creature);
    case RandomArtActType::BO_ELEC_1:
        return activate_bolt_elec_1(creature);
    case RandomArtActType::BO_ACID_1:
        return activate_bolt_acid_1(creature);
    case RandomArtActType::BO_COLD_1:
        return activate_bolt_cold_1(creature);
    case RandomArtActType::BO_FIRE_1:
        return activate_bolt_fire_1(creature);
    case RandomArtActType::BA_COLD_1:
        return activate_ball_cold_1(creature);
    case RandomArtActType::BA_COLD_2:
        return activate_ball_cold_2(creature);
    case RandomArtActType::BA_COLD_3:
        return activate_ball_cold_2(creature);
    case RandomArtActType::BA_FIRE_1:
        return activate_ball_fire_1(creature);
    case RandomArtActType::BA_FIRE_2:
        return activate_ball_fire_2(creature, name);
    case RandomArtActType::BA_FIRE_3:
        return activate_ball_fire_3(creature);
    case RandomArtActType::BA_FIRE_4:
        return activate_ball_fire_4(creature);
    case RandomArtActType::BA_ELEC_2:
        return activate_ball_elec_2(creature);
    case RandomArtActType::BA_ELEC_3:
        return activate_ball_elec_3(creature);
    case RandomArtActType::BA_ACID_1:
        return activate_ball_acid_1(creature);
    case RandomArtActType::BA_NUKE_1:
        return activate_ball_nuke_1(creature);
    case RandomArtActType::HYPODYNAMIA_1:
        return activate_bolt_hypodynamia_1(creature, name);
    case RandomArtActType::HYPODYNAMIA_2:
        return activate_bolt_hypodynamia_2(creature);
    case RandomArtActType::DRAIN_1:
        return activate_bolt_drain_1(creature);
    case RandomArtActType::BO_MISS_2:
        return activate_missile_2(creature);
    case RandomArtActType::WHIRLWIND:
        return activate_whirlwind(creature);
    case RandomArtActType::DRAIN_2:
        return activate_bolt_drain_2(creature);
    case RandomArtActType::CALL_CHAOS:
        return activate_call_chaos(creature);
    case RandomArtActType::ROCKET:
        return activate_rocket(creature);
    case RandomArtActType::DISP_EVIL:
        return activate_dispel_evil(creature);
    case RandomArtActType::BA_MISS_3:
        return activate_missile_3(creature);
    case RandomArtActType::DISP_GOOD:
        return activate_dispel_good(creature);
    case RandomArtActType::BO_MANA:
        return activate_bolt_mana(creature, name);
    case RandomArtActType::BA_WATER:
        return activate_ball_water(creature, name);
    case RandomArtActType::BA_DARK:
        return activate_ball_dark(creature, name);
    case RandomArtActType::BA_MANA:
        return activate_ball_mana(creature, name);
    case RandomArtActType::PESTICIDE:
        return activate_pesticide(creature);
    case RandomArtActType::BLINDING_LIGHT:
        return activate_blinding_light(creature, name);
    case RandomArtActType::BIZARRE:
        return activate_ring_of_power(creature, name);
    case RandomArtActType::CAST_BA_STAR:
        return activate_ball_lite(creature, name);
    case RandomArtActType::BLADETURNER:
        return activate_bladeturner(creature);
    case RandomArtActType::BR_FIRE:
        return activate_breath_fire(creature, o_ptr);
    case RandomArtActType::BR_COLD:
        return activate_breath_cold(creature, o_ptr);
    case RandomArtActType::BR_DRAGON:
        return activate_dragon_breath(creature, o_ptr);
    case RandomArtActType::TREE_CREATION:
        return activate_tree_creation(creature, o_ptr, name);
    case RandomArtActType::ANIM_DEAD:
        return activate_animate_dead(creature, o_ptr);
    case RandomArtActType::CONFUSE:
        return activate_confusion(creature);
    case RandomArtActType::SLEEP:
        return activate_sleep(creature);
    case RandomArtActType::QUAKE:
        return activate_earthquake(creature);
    case RandomArtActType::TERROR:
        return activate_terror(creature);
    case RandomArtActType::TELE_AWAY:
        return activate_teleport_away(creature);
    case RandomArtActType::BANISH_EVIL:
        return activate_banish_evil(creature);
    case RandomArtActType::GENOCIDE:
        return activate_genocide(creature);
    case RandomArtActType::MASS_GENO:
        return activate_mass_genocide(creature);
    case RandomArtActType::SCARE_AREA:
        return activate_scare(creature);
    case RandomArtActType::AGGRAVATE:
        return activate_aggravation(creature, o_ptr, name);
    case RandomArtActType::CHARM_ANIMAL:
        return activate_charm_animal(creature);
    case RandomArtActType::CHARM_UNDEAD:
        return activate_charm_undead(creature);
    case RandomArtActType::CHARM_OTHER:
        return activate_charm_other(creature);
    case RandomArtActType::CHARM_ANIMALS:
        return activate_charm_animals(creature);
    case RandomArtActType::CHARM_OTHERS:
        return activate_charm_others(creature);
    case RandomArtActType::SUMMON_ANIMAL:
        (void)summon_specific(creature, creature.y, creature.x, creature.level, SUMMON_ANIMAL_RANGER, PM_ALLOW_GROUP | PM_FORCE_PET);
        return true;
    case RandomArtActType::SUMMON_PHANTOM:
        msg_print(_("幻霊を召喚した。", "You summon a phantasmal servant."));
        (void)summon_specific(creature, creature.y, creature.x, creature.get_floor()->dun_level, SUMMON_PHANTOM, PM_ALLOW_GROUP | PM_FORCE_PET);
        return true;
    case RandomArtActType::SUMMON_ELEMENTAL:
        return cast_summon_elemental(creature, (creature.level * 3) / 2);
    case RandomArtActType::SUMMON_DEMON:
        cast_summon_demon(creature, (creature.level * 3) / 2);
        return true;
    case RandomArtActType::SUMMON_UNDEAD:
        return cast_summon_undead(creature, (creature.level * 3) / 2);
    case RandomArtActType::SUMMON_HOUND:
        return cast_summon_hound(creature, (creature.level * 3) / 2);
    case RandomArtActType::SUMMON_DAWN:
        msg_print(_("暁の師団を召喚した。", "You summon the Legion of the Dawn."));
        (void)summon_specific(creature, creature.y, creature.x, creature.get_floor()->dun_level, SUMMON_DAWN, PM_ALLOW_GROUP | PM_FORCE_PET);
        return true;
    case RandomArtActType::SUMMON_OCTOPUS:
        return cast_summon_octopus(creature);
    case RandomArtActType::CHOIR_SINGS:
        msg_print(_("天国の歌が聞こえる...", "A heavenly choir sings..."));
        (void)cure_critical_wounds(creature, 777);
        (void)set_hero(creature, randint1(25) + 25, false);
        return true;
    case RandomArtActType::CURE_LW:
        return activate_cure_lw(creature);
    case RandomArtActType::CURE_MW:
        msg_print(_("深紫色の光を発している...", "It radiates deep purple..."));
        (void)cure_serious_wounds(creature, Dice::roll(4, 8));
        return true;
    case RandomArtActType::CURE_POISON: {
        msg_print(_("深青色に輝いている...", "It glows deep blue..."));
        BadStatusSetter bss(creature);
        (void)bss.set_fear(0);
        (void)bss.set_poison(0);
        return true;
    }
    case RandomArtActType::REST_EXP:
        msg_print(_("深紅に輝いている...", "It glows a deep red..."));
        restore_level(creature);
        return true;
    case RandomArtActType::REST_ALL:
        msg_print(_("濃緑色に輝いている...", "It glows a deep green..."));
        (void)restore_all_status(creature);
        (void)restore_level(creature);
        return true;
    case RandomArtActType::CURE_700:
        msg_print(_("深青色に輝いている...", "It glows deep blue..."));
        msg_print(_("体内に暖かい鼓動が感じられる...", "You feel a warm tingling inside..."));
        (void)cure_critical_wounds(creature, 700);
        return true;
    case RandomArtActType::CURE_1000:
        msg_print(_("白く明るく輝いている...", "It glows a bright white..."));
        msg_print(_("ひじょうに気分がよい...", "You feel much better..."));
        (void)cure_critical_wounds(creature, 1000);
        return true;
    case RandomArtActType::CURING:
        msg_format(_("%sの優しさに癒される...", "the %s cures you affectionately ..."), name.data());
        true_healing(creature, 0);
        return true;
    case RandomArtActType::CURE_MANA_FULL:
        msg_format(_("%sが青白く光った．．．", "The %s glows palely..."), name.data());
        restore_mana(creature, true);
        return true;
    case RandomArtActType::ESP:
        (void)set_tim_esp(creature, randint1(30) + 25, false);
        return true;
    case RandomArtActType::BERSERK:
        (void)berserk(creature, randint1(25) + 25);
        return true;
    case RandomArtActType::PROT_EVIL:
        msg_format(_("%sから鋭い音が流れ出た...", "The %s lets out a shrill wail..."), name.data());
        BodyImprovement(creature).set_protection(randint1(25) + creature.level * 3);
        return true;
    case RandomArtActType::RESIST_ALL:
        return activate_resistance_elements(creature);
    case RandomArtActType::SPEED:
        msg_print(_("明るく緑色に輝いている...", "It glows bright green..."));
        (void)set_acceleration(creature, randint1(20) + 20, false);
        return true;
    case RandomArtActType::MID_SPEED:
        msg_print(_("明るく緑色に輝いている...", "It glows bright green..."));
        (void)set_acceleration(creature, randint1(50) + 50, false);
        return true;
    case RandomArtActType::XTRA_SPEED:
        msg_print(_("明るく輝いている...", "It glows brightly..."));
        (void)set_acceleration(creature, randint1(75) + 75, false);
        return true;
    case RandomArtActType::WRAITH:
        set_wraith_form(creature, randint1(creature.level / 2) + (creature.level / 2), false);
        return true;
    case RandomArtActType::INVULN:
        (void)set_invuln(creature, randint1(8) + 8, false);
        return true;
    case RandomArtActType::HERO:
        (void)heroism(creature, 25);
        return true;
    case RandomArtActType::HERO_SPEED:
        (void)set_acceleration(creature, randint1(50) + 50, false);
        (void)heroism(creature, 50);
        return true;
    case RandomArtActType::ACID_BALL_AND_RESISTANCE:
        return activate_acid_ball_and_resistance(creature, name);
    case RandomArtActType::FIRE_BALL_AND_RESISTANCE:
        return activate_fire_ball_and_resistance(creature, name);
    case RandomArtActType::COLD_BALL_AND_RESISTANCE:
        return activate_cold_ball_and_resistance(creature, name);
    case RandomArtActType::ELEC_BALL_AND_RESISTANCE:
        return activate_elec_ball_and_resistance(creature, name);
    case RandomArtActType::POIS_BALL_AND_RESISTANCE:
        return activate_pois_ball_and_resistance(creature, name);
    case RandomArtActType::RESIST_ACID:
        return activate_resistance_acid(creature, name);
    case RandomArtActType::RESIST_FIRE:
        return activate_resistance_fire(creature, name);
    case RandomArtActType::RESIST_COLD:
        return activate_resistance_cold(creature, name);
    case RandomArtActType::RESIST_ELEC:
        return activate_resistance_elec(creature, name);
    case RandomArtActType::RESIST_POIS:
        return activate_resistance_pois(creature, name);
    case RandomArtActType::LIGHT:
        return activate_light(creature, name);
    case RandomArtActType::MAP_LIGHT:
        return activate_map_light(creature);
    case RandomArtActType::DETECT_ALL:
        return activate_all_detection(creature);
    case RandomArtActType::DETECT_XTRA:
        return activate_extra_detection(creature);
    case RandomArtActType::ID_FULL:
        return activate_fully_identification(creature);
    case RandomArtActType::ID_PLAIN:
        return activate_identification(creature);
    case RandomArtActType::RUNE_EXPLO:
        return activate_exploding_rune(creature);
    case RandomArtActType::RUNE_PROT:
        return activate_protection_rune(creature);
    case RandomArtActType::SATIATE:
        (void)set_food(creature, PY_FOOD_MAX - 1);
        return true;
    case RandomArtActType::DEST_DOOR:
        return activate_door_destroy(creature);
    case RandomArtActType::STONE_MUD:
        return activate_stone_mud(creature);
    case RandomArtActType::RECHARGE:
        return activate_recharge(creature);
    case RandomArtActType::ALCHEMY:
        msg_print(_("明るい黄色に輝いている...", "It glows bright yellow..."));
        (void)alchemy(creature);
        return true;
    case RandomArtActType::DIM_DOOR:
        return activate_dimension_door(creature);
    case RandomArtActType::TELEPORT:
        return activate_teleport(creature);
    case RandomArtActType::RECALL:
        return activate_recall(creature);
    case RandomArtActType::JUDGE:
        return activate_judgement(creature, name);
    case RandomArtActType::TELEKINESIS:
        return activate_telekinesis(creature, name);
    case RandomArtActType::DETECT_UNIQUE:
        return activate_unique_detection(creature);
    case RandomArtActType::ESCAPE:
        return activate_escape(creature);
    case RandomArtActType::DISP_CURSE_XTRA:
        return activate_dispel_curse(creature, name);
    case RandomArtActType::BRAND_FIRE_BOLTS:
        msg_format(_("%sが深紅に輝いた...", "Your %s glows deep red..."), name.data());
        brand_bolts(creature);
        return true;
    case RandomArtActType::RECHARGE_XTRA:
        return activate_recharge_extra(creature, name);
    case RandomArtActType::LORE:
        return StoneOfLore(creature).perilous_secrets();
    case RandomArtActType::SHIKOFUMI:
        return activate_shikofumi(creature);
    case RandomArtActType::PHASE_DOOR:
        return activate_phase_door(creature);
    case RandomArtActType::DETECT_ALL_MONS:
        return activate_all_monsters_detection(creature);
    case RandomArtActType::ULTIMATE_RESIST:
        return activate_ultimate_resistance(creature);
    case RandomArtActType::ELBERETH:
        return activate_protection_elbereth(creature);
    case RandomArtActType::DETECT_TREASURE:
        return activate_detect_treasure(creature);
    case RandomArtActType::CAST_OFF:
        if (const auto item_casted = cosmic_cast_off(creature, **o_ptr_ptr); item_casted) {
            *o_ptr_ptr = item_casted.get();
            return true;
        }

        return false;
    case RandomArtActType::FALLING_STAR:
        return activate_toragoroshi(creature);
    case RandomArtActType::GRAND_CROSS:
        return activate_grand_cross(creature);
    case RandomArtActType::TELEPORT_LEVEL:
        return activate_teleport_level(creature);
    case RandomArtActType::STRAIN_HASTE:
        msg_format(_("%sはあなたの体力を奪った...", "The %s drains your vitality..."), name.data());
        take_hit(creature, DAMAGE_LOSELIFE, Dice::roll(3, 8), _("加速した疲労", "the strain of haste"));
        (void)mod_acceleration(creature, 25 + randint1(25), false);
        return true;
    case RandomArtActType::FISHING:
        return fishing(creature);
    case RandomArtActType::INROU:
        mitokohmon(creature);
        return true;
    case RandomArtActType::MURAMASA:
        return activate_muramasa(creature, o_ptr);
    case RandomArtActType::BLOODY_MOON:
        return activate_bloody_moon(creature, o_ptr);
    case RandomArtActType::CRIMSON:
        return activate_crimson(creature, o_ptr);
    case RandomArtActType::HERO_BLESS:
        (void)set_hero(creature, randint1(25) + 25, false);
        (void)set_blessed(creature, randint1(25) + 25, true);
        return true;
    case RandomArtActType::CREATE_AMMO:
        return activate_create_ammo(creature);
    case RandomArtActType::DISPEL_MAGIC:
        return activate_dispel_magic(creature);
    case RandomArtActType::DESTRUCTION:
        return destroy_area(creature, creature.y, creature.x, (13) + randint0(5), false);
    case RandomArtActType::HUGE_STINKING_STORM:
        return activate_huge_stinking_storm(creature);
    case RandomArtActType::WHISTLE:
        return activate_whistle(creature, *o_ptr);
    case RandomArtActType::CAPTURE_MONSTER:
        return exe_monster_capture(creature, *o_ptr);
    case RandomArtActType::THE_WORLD:
        return time_walk(creature);
    default:
        msg_format(_("Unknown activation effect: %d.", "Unknown activation effect: %d."), enum2i(index));
        return false;
    }
}
