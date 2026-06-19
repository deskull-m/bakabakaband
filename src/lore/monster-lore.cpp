/*!
 * @brief モンスターの思い出表示に必要なフラグ類の処理
 * @date 2020/06/09
 * @author Hourier
 */

#include "lore/monster-lore.h"
#include "game-option/cheat-options.h"
#include "game-option/text-display-options.h"
#include "lore/lore-calculator.h"
#include "lore/lore-util.h"
#include "lore/magic-types-setter.h"
#include "monster-race/race-misc-flags.h"
#include "player-ability/player-ability-types.h"
#include "system/angband.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "term/term-color-types.h"
#include "view/display-lore-attacks.h"
#include "view/display-lore-drops.h"
#include "view/display-lore-magics.h"
#include "view/display-lore-status.h"
#include "view/display-lore.h"
#include <algorithm>

static void set_msex_flags(lore_type *lore_ptr)
{
    lore_ptr->msex = MonsterSex::NONE;
    if (lore_ptr->monrace->is_male()) {
        lore_ptr->msex = MonsterSex::MALE;
    }
    if (lore_ptr->monrace->is_female()) {
        lore_ptr->msex = MonsterSex::FEMALE;
    }
}

static void set_flags1(lore_type *lore_ptr)
{
    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::UNIQUE)) {
        lore_ptr->kind_flags.set(MonsterKindType::UNIQUE);
    }

    if (lore_ptr->monrace->misc_flags.has(MonsterMiscType::QUESTOR)) {
        lore_ptr->misc_flags.set(MonsterMiscType::QUESTOR);
    }

    if (lore_ptr->monrace->misc_flags.has(MonsterMiscType::HAS_FRIENDS)) {
        lore_ptr->misc_flags.set(MonsterMiscType::HAS_FRIENDS);
    }

    if (lore_ptr->monrace->misc_flags.has(MonsterMiscType::ESCORT)) {
        lore_ptr->misc_flags.set(MonsterMiscType::ESCORT);
    }

    if (lore_ptr->monrace->misc_flags.has(MonsterMiscType::MORE_ESCORT)) {
        lore_ptr->misc_flags.set(MonsterMiscType::MORE_ESCORT);
    }
}

static void set_race_flags(lore_type *lore_ptr)
{
    if (!lore_ptr->monrace->r_tkills && !lore_ptr->know_everything) {
        return;
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::ORC)) {
        lore_ptr->kind_flags.set(MonsterKindType::ORC);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::TROLL)) {
        lore_ptr->kind_flags.set(MonsterKindType::TROLL);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::GIANT)) {
        lore_ptr->kind_flags.set(MonsterKindType::GIANT);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::DRAGON)) {
        lore_ptr->kind_flags.set(MonsterKindType::DRAGON);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::DEMON)) {
        lore_ptr->kind_flags.set(MonsterKindType::DEMON);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::UNDEAD)) {
        lore_ptr->kind_flags.set(MonsterKindType::UNDEAD);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::EVIL)) {
        lore_ptr->kind_flags.set(MonsterKindType::EVIL);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::GOOD)) {
        lore_ptr->kind_flags.set(MonsterKindType::GOOD);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::ANIMAL)) {
        lore_ptr->kind_flags.set(MonsterKindType::ANIMAL);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::AMBERITE)) {
        lore_ptr->kind_flags.set(MonsterKindType::AMBERITE);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::HUMAN)) {
        lore_ptr->kind_flags.set(MonsterKindType::HUMAN);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::QUANTUM)) {
        lore_ptr->kind_flags.set(MonsterKindType::QUANTUM);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::ANGEL)) {
        lore_ptr->kind_flags.set(MonsterKindType::ANGEL);
    }

    if (lore_ptr->monrace->misc_flags.has(MonsterMiscType::FORCE_DEPTH)) {
        lore_ptr->misc_flags.set(MonsterMiscType::FORCE_DEPTH);
    }

    if (lore_ptr->monrace->misc_flags.has(MonsterMiscType::FORCE_MAXHP)) {
        lore_ptr->misc_flags.set(MonsterMiscType::FORCE_MAXHP);
    }

    if (lore_ptr->monrace->misc_flags.has(MonsterMiscType::STALKER)) {
        lore_ptr->misc_flags.set(MonsterMiscType::STALKER);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::YAZYU)) {
        lore_ptr->kind_flags.set(MonsterKindType::YAZYU);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::DOG)) {
        lore_ptr->kind_flags.set(MonsterKindType::DOG);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::CAT)) {
        lore_ptr->kind_flags.set(MonsterKindType::CAT);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::RABBIT)) {
        lore_ptr->kind_flags.set(MonsterKindType::RABBIT);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::PEASANT)) {
        lore_ptr->kind_flags.set(MonsterKindType::PEASANT);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::RABBLE)) {
        lore_ptr->kind_flags.set(MonsterKindType::RABBLE);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::NOBLE)) {
        lore_ptr->kind_flags.set(MonsterKindType::NOBLE);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::BEAST)) {
        lore_ptr->kind_flags.set(MonsterKindType::BEAST);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::LEECH)) {
        lore_ptr->kind_flags.set(MonsterKindType::LEECH);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::JELLYFISH)) {
        lore_ptr->kind_flags.set(MonsterKindType::JELLYFISH);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::CITIZEN)) {
        lore_ptr->kind_flags.set(MonsterKindType::CITIZEN);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::TREEFOLK)) {
        lore_ptr->kind_flags.set(MonsterKindType::TREEFOLK);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::VIRUS)) {
        lore_ptr->kind_flags.set(MonsterKindType::VIRUS);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::SPHINX)) {
        lore_ptr->kind_flags.set(MonsterKindType::SPHINX);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::SCORPION)) {
        lore_ptr->kind_flags.set(MonsterKindType::SCORPION);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::MINDCRAFTER)) {
        lore_ptr->kind_flags.set(MonsterKindType::MINDCRAFTER);
    }
    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::TANUKI)) {
        lore_ptr->kind_flags.set(MonsterKindType::TANUKI);
    }
    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::CHAMELEON)) {
        lore_ptr->kind_flags.set(MonsterKindType::CHAMELEON);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::ARCHER)) {
        lore_ptr->kind_flags.set(MonsterKindType::ARCHER);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::GUNNER)) {
        lore_ptr->kind_flags.set(MonsterKindType::GUNNER);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::SMITH)) {
        lore_ptr->kind_flags.set(MonsterKindType::SMITH);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::WHEEL)) {
        lore_ptr->kind_flags.set(MonsterKindType::WHEEL);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::GREAT_OLD_ONE)) {
        lore_ptr->kind_flags.set(MonsterKindType::GREAT_OLD_ONE);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::APE)) {
        lore_ptr->kind_flags.set(MonsterKindType::APE);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::HORSE)) {
        lore_ptr->kind_flags.set(MonsterKindType::HORSE);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::FROG)) {
        lore_ptr->kind_flags.set(MonsterKindType::FROG);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::BEHOLDER)) {
        lore_ptr->kind_flags.set(MonsterKindType::BEHOLDER);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::YEEK)) {
        lore_ptr->kind_flags.set(MonsterKindType::YEEK);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::AQUATIC_MAMMAL)) {
        lore_ptr->kind_flags.set(MonsterKindType::AQUATIC_MAMMAL);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::FISH)) {
        lore_ptr->kind_flags.set(MonsterKindType::FISH);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::BIRD)) {
        lore_ptr->kind_flags.set(MonsterKindType::BIRD);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::WALL)) {
        lore_ptr->kind_flags.set(MonsterKindType::WALL);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::PLANT)) {
        lore_ptr->kind_flags.set(MonsterKindType::PLANT);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::FUNGUS)) {
        lore_ptr->kind_flags.set(MonsterKindType::FUNGUS);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::TURTLE)) {
        lore_ptr->kind_flags.set(MonsterKindType::TURTLE);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::SNAKE)) {
        lore_ptr->kind_flags.set(MonsterKindType::SNAKE);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::FAIRY)) {
        lore_ptr->kind_flags.set(MonsterKindType::FAIRY);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::VAMPIRE)) {
        lore_ptr->kind_flags.set(MonsterKindType::VAMPIRE);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::BEAR)) {
        lore_ptr->kind_flags.set(MonsterKindType::BEAR);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::VORTEX)) {
        lore_ptr->kind_flags.set(MonsterKindType::VORTEX);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::OOZE)) {
        lore_ptr->kind_flags.set(MonsterKindType::OOZE);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::DINOSAUR)) {
        lore_ptr->kind_flags.set(MonsterKindType::DINOSAUR);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::LICH)) {
        lore_ptr->kind_flags.set(MonsterKindType::LICH);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::GHOST)) {
        lore_ptr->kind_flags.set(MonsterKindType::GHOST);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::BERSERK)) {
        lore_ptr->kind_flags.set(MonsterKindType::BERSERK);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::EXPLOSIVE)) {
        lore_ptr->kind_flags.set(MonsterKindType::EXPLOSIVE);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::RAT)) {
        lore_ptr->kind_flags.set(MonsterKindType::RAT);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::MINOTAUR)) {
        lore_ptr->kind_flags.set(MonsterKindType::MINOTAUR);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::SKAVEN)) {
        lore_ptr->kind_flags.set(MonsterKindType::SKAVEN);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::KOBOLD)) {
        lore_ptr->kind_flags.set(MonsterKindType::KOBOLD);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::OGRE)) {
        lore_ptr->kind_flags.set(MonsterKindType::OGRE);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::BOVINE)) {
        lore_ptr->kind_flags.set(MonsterKindType::BOVINE);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::MERFOLK)) {
        lore_ptr->kind_flags.set(MonsterKindType::MERFOLK);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::SHARK)) {
        lore_ptr->kind_flags.set(MonsterKindType::SHARK);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::MESUGAKI)) {
        lore_ptr->kind_flags.set(MonsterKindType::MESUGAKI);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::HYDRA)) {
        lore_ptr->kind_flags.set(MonsterKindType::HYDRA);
    }

    if (lore_ptr->monrace->kind_flags.has(MonsterKindType::SHIP)) {
        lore_ptr->kind_flags.set(MonsterKindType::SHIP);
    }
}

/*!
 * @brief モンスターの思い出情報を表示するメインルーチン
 * Hack -- display monster information using "hooked_roff()"
 * @param r_idx モンスターの種族ID
 * @param mode 表示オプション
 * @details
 * This function should only be called with the cursor placed at the
 * left edge of the screen, on a cleared line, in which the recall is
 * to take place.  One extra blank line is left after the recall.
 */
void process_monster_lore(CreatureEntity &creature, MonraceId r_idx, monster_lore_mode mode)
{
    lore_type tmp_lore(r_idx, mode);
    lore_type *lore_ptr = &tmp_lore;
    if (cheat_know || (mode == MONSTER_LORE_RESEARCH) || (mode == MONSTER_LORE_DEBUG)) {
        lore_ptr->know_everything = true;
    }
    set_flags_for_full_knowledge(lore_ptr);
    set_msex_flags(lore_ptr);
    set_flags1(lore_ptr);
    set_race_flags(lore_ptr);
    const auto &text = lore_ptr->monrace->text;

    if (show_lore_summary || lore_summary_only) {
        display_monster_kind_tags(lore_ptr);
        display_monster_hp_ac_summary(lore_ptr);
        display_monster_speed_summary(lore_ptr);
        display_monster_alert_summary(lore_ptr);
        display_monster_kills_summary(lore_ptr);
        display_where_to_appear_summary(lore_ptr);
        display_monster_exp_summary(lore_ptr);
        display_monster_evolution_summary(lore_ptr);
        hooked_roff("\n");
        set_monster_aura_summary(lore_ptr);
        display_monster_behavior_summary(lore_ptr);
        display_monster_drops_summary(lore_ptr);
        display_monster_melee_summary_line(lore_ptr);
        display_monster_magic_rate(lore_ptr);
        display_monster_magic_tables(creature, lore_ptr);
        display_monster_resistance_table(lore_ptr);
        hook_c_roff(TERM_L_DARK, "------------------------------------------------------------\n");
    }

    if (lore_summary_only && lore_ptr->mode != MONSTER_LORE_DEBUG) {
        return;
    }

    display_kill_numbers(lore_ptr);

    if (!text.empty()) {
        hooked_roff(text);
        hooked_roff("\n");
    }

    if (r_idx == MonraceId::KAGE) {
        hooked_roff("\n");
        return;
    }

    if (!display_where_to_appear(lore_ptr)) {
        return;
    }

    display_monster_move(lore_ptr);
    display_monster_never_move(lore_ptr);
    if (lore_ptr->old) {
        hooked_roff(_("。", ".  "));
        lore_ptr->old = false;
    }

    display_lore_this(creature, lore_ptr);
    if (lore_ptr->special_flags.has(MonsterSpecialType::DIMINISH_MAX_DAMAGE)) {
        hooked_roff(format(_("%s^は", "%s^ "), Who::who(lore_ptr->msex).data()));
        hook_c_roff(TERM_RED, _("致命的な威力の攻撃に対して大きな耐性を持っている。", "has the strong resistance for a critical damage.  "));
    }
    display_monster_aura(lore_ptr);
    if (lore_ptr->misc_flags.has(MonsterMiscType::REFLECTING)) {
        hooked_roff(format(_("%s^は矢の呪文を跳ね返す。", "%s^ reflects bolt spells.  "), Who::who(lore_ptr->msex).data()));
    }

    display_monster_collective(lore_ptr);
    lore_ptr->lore_msgs.clear();
    if (lore_ptr->ability_flags.has(MonsterAbilityType::SHRIEK)) {
        lore_ptr->lore_msgs.emplace_back(_("悲鳴で助けを求める", "shriek for help"), TERM_L_WHITE);
    }

    display_monster_launching(creature, lore_ptr);
    if (lore_ptr->ability_flags.has(MonsterAbilityType::SPECIAL)) {
        lore_ptr->lore_msgs.emplace_back(_("特別な行動をする", "do something"), TERM_VIOLET);
    }

    display_monster_sometimes(lore_ptr);
    set_breath_types(creature, lore_ptr);
    display_monster_breath(lore_ptr);

    lore_ptr->lore_msgs.clear();
    set_ball_types(creature, lore_ptr);
    set_particular_types(creature, lore_ptr);
    set_bolt_types(creature, lore_ptr);
    set_status_types(lore_ptr);
    set_teleport_types(lore_ptr);
    set_floor_types(creature, lore_ptr);
    set_summon_types(lore_ptr);
    display_monster_magic_types(lore_ptr);
    display_mosnter_magic_possibility(lore_ptr);
    display_monster_hp_ac(lore_ptr);

    lore_ptr->lore_msgs.clear();
    display_monster_concrete_abilities(lore_ptr);
    display_monster_abilities(lore_ptr);
    display_monster_constitutions(lore_ptr);

    lore_ptr->lore_msgs.clear();
    display_monster_concrete_weakness(lore_ptr);
    display_monster_weakness(lore_ptr);

    lore_ptr->lore_msgs.clear();
    display_monster_concrete_resistances(lore_ptr);
    display_monster_resistances(lore_ptr);
    display_monster_evolution(lore_ptr);

    lore_ptr->lore_msgs.clear();
    display_monster_concrete_immunities(lore_ptr);
    display_monster_immunities(lore_ptr);
    display_monster_alert(lore_ptr);
    display_monster_drops(lore_ptr);
    display_monster_dead_spawns(lore_ptr);
    display_monster_blows(lore_ptr);
    display_monster_guardian(lore_ptr);
}
