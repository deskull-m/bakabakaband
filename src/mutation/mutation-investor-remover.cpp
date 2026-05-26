#include "mutation/mutation-investor-remover.h"
#include "avatar/avatar.h"
#include "core/stuff-handler.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "mutation/gain-mutation-switcher.h"
#include "mutation/lose-mutation-switcher.h"
#include "mutation/mutation-calculator.h" //!< @todo calc_mutant_regenerate_mod() が相互依存している、後で消す.
#include "mutation/mutation-flag-types.h"
#include "mutation/mutation-util.h"
#include "player-base/player-race.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "system/redrawing-flags-updater.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

static void sweep_gain_mutation(CreatureEntity &creature, glm_type *gm_ptr)
{
    int attempts_left = 20;
    if (gm_ptr->choose_mut) {
        attempts_left = 1;
    }

    while (attempts_left--) {
        switch_gain_mutation(creature, gm_ptr);
        if (gm_ptr->muta_which != PlayerMutationType::MAX && creature.get_mutations().has_not(gm_ptr->muta_which)) {
            gm_ptr->muta_chosen = true;
        }

        if (gm_ptr->muta_chosen) {
            break;
        }
    }
}

static void race_dependent_mutation(CreatureEntity &creature, glm_type *gm_ptr)
{
    if (gm_ptr->choose_mut != 0) {
        return;
    }

    CreatureRace pr(&creature);
    if (pr.equals(PlayerRaceType::VAMPIRE) && creature.get_mutations().has_not(PlayerMutationType::HYPN_GAZE) && (randint1(10) < 7)) {
        gm_ptr->muta_which = PlayerMutationType::HYPN_GAZE;
        gm_ptr->muta_desc = _("眼が幻惑的になった...", "Your eyes look mesmerizing...");
        return;
    }

    if (pr.equals(PlayerRaceType::IMP) && creature.get_mutations().has_not(PlayerMutationType::HORNS) && (randint1(10) < 7)) {
        gm_ptr->muta_which = PlayerMutationType::HORNS;
        gm_ptr->muta_desc = _("角が額から生えてきた！", "Horns pop forth into your forehead!");
        return;
    }

    if (pr.equals(PlayerRaceType::YEEK) && creature.get_mutations().has_not(PlayerMutationType::SHRIEK) && (randint1(10) < 7)) {
        gm_ptr->muta_which = PlayerMutationType::SHRIEK;
        gm_ptr->muta_desc = _("声質がかなり強くなった。", "Your vocal cords get much tougher.");
        return;
    }

    if (pr.equals(PlayerRaceType::BEASTMAN) && creature.get_mutations().has_not(PlayerMutationType::POLYMORPH) && (randint1(10) < 2)) {
        gm_ptr->muta_which = PlayerMutationType::POLYMORPH;
        gm_ptr->muta_desc = _("あなたの肉体は変化できるようになった、", "Your body seems mutable.");
        return;
    }

    if (pr.equals(PlayerRaceType::MIND_FLAYER) && creature.get_mutations().has_not(PlayerMutationType::TENTACLES) && (randint1(10) < 7)) {
        gm_ptr->muta_which = PlayerMutationType::TENTACLES;
        gm_ptr->muta_desc = _("邪悪な触手が口の周りに生えた。", "Evil-looking tentacles sprout from your mouth.");
    }
}

static void neutralize_base_status(CreatureEntity &creature, glm_type *gm_ptr)
{
    if (gm_ptr->muta_which == PlayerMutationType::PUNY) {
        if (creature.get_mutations().has(PlayerMutationType::HYPER_STR)) {
            msg_print(_("あなたはもう超人的に強くはない！", "You no longer feel super-strong!"));
            creature.remove_mutation(PlayerMutationType::HYPER_STR);
        }

        return;
    }

    if (gm_ptr->muta_which == PlayerMutationType::HYPER_STR) {
        if (creature.get_mutations().has(PlayerMutationType::PUNY)) {
            msg_print(_("あなたはもう虚弱ではない！", "You no longer feel puny!"));
            creature.remove_mutation(PlayerMutationType::PUNY);
        }

        return;
    }

    if (gm_ptr->muta_which == PlayerMutationType::MORONIC) {
        if (creature.get_mutations().has(PlayerMutationType::HYPER_INT)) {
            msg_print(_("あなたの脳はもう生体コンピュータではない。", "Your brain is no longer a living computer."));
            creature.remove_mutation(PlayerMutationType::HYPER_INT);
        }

        return;
    }

    if (gm_ptr->muta_which == PlayerMutationType::HYPER_INT) {
        if (creature.get_mutations().has(PlayerMutationType::MORONIC)) {
            msg_print(_("あなたはもう精神薄弱ではない。", "You are no longer moronic."));
            creature.remove_mutation(PlayerMutationType::MORONIC);
        }

        return;
    }

    if (gm_ptr->muta_which == PlayerMutationType::IRON_SKIN) {
        if (creature.get_mutations().has(PlayerMutationType::SCALES)) {
            msg_print(_("鱗がなくなった。", "You lose your scales."));
            creature.remove_mutation(PlayerMutationType::SCALES);
        }

        if (creature.get_mutations().has(PlayerMutationType::FLESH_ROT)) {
            msg_print(_("肉体が腐乱しなくなった。", "Your flesh rots no longer."));
            creature.remove_mutation(PlayerMutationType::FLESH_ROT);
        }

        if (creature.get_mutations().has(PlayerMutationType::WART_SKIN)) {
            msg_print(_("肌のイボイボがなくなった。", "You lose your warts."));
            creature.remove_mutation(PlayerMutationType::WART_SKIN);
        }

        return;
    }

    if (gm_ptr->muta_which == PlayerMutationType::WART_SKIN || gm_ptr->muta_which == PlayerMutationType::SCALES || gm_ptr->muta_which == PlayerMutationType::FLESH_ROT) {
        if (creature.get_mutations().has(PlayerMutationType::IRON_SKIN)) {
            msg_print(_("あなたの肌はもう鉄ではない。", "Your skin is no longer made of steel."));
            creature.remove_mutation(PlayerMutationType::IRON_SKIN);
        }

        return;
    }

    if (gm_ptr->muta_which == PlayerMutationType::FEARLESS) {
        if (creature.get_mutations().has(PlayerMutationType::COWARDICE)) {
            msg_print(_("臆病でなくなった。", "You are no longer cowardly."));
            creature.remove_mutation(PlayerMutationType::COWARDICE);
        }

        return;
    }

    if (gm_ptr->muta_which == PlayerMutationType::FLESH_ROT) {
        if (creature.get_mutations().has(PlayerMutationType::REGEN)) {
            msg_print(_("急速に回復しなくなった。", "You stop regenerating."));
            creature.remove_mutation(PlayerMutationType::REGEN);
        }

        return;
    }

    if (gm_ptr->muta_which == PlayerMutationType::REGEN) {
        if (creature.get_mutations().has(PlayerMutationType::FLESH_ROT)) {
            msg_print(_("肉体が腐乱しなくなった。", "Your flesh stops rotting."));
            creature.remove_mutation(PlayerMutationType::FLESH_ROT);
        }

        return;
    }

    if (gm_ptr->muta_which == PlayerMutationType::LIMBER) {
        if (creature.get_mutations().has(PlayerMutationType::ARTHRITIS)) {
            msg_print(_("関節が痛くなくなった。", "Your joints stop hurting."));
            creature.remove_mutation(PlayerMutationType::ARTHRITIS);
        }

        return;
    }

    if (gm_ptr->muta_which == PlayerMutationType::ARTHRITIS) {
        if (creature.get_mutations().has(PlayerMutationType::LIMBER)) {
            msg_print(_("あなたはしなやかでなくなった。", "You no longer feel limber."));
            creature.remove_mutation(PlayerMutationType::LIMBER);
        }

        return;
    }
}

static void neutralize_other_status(CreatureEntity &creature, glm_type *gm_ptr)
{
    if (gm_ptr->muta_which == PlayerMutationType::COWARDICE) {
        if (creature.get_mutations().has(PlayerMutationType::FEARLESS)) {
            msg_print(_("恐れ知らずでなくなった。", "You no longer feel fearless."));
            creature.remove_mutation(PlayerMutationType::FEARLESS);
        }
    }

    if (gm_ptr->muta_which == PlayerMutationType::BEAK) {
        if (creature.get_mutations().has(PlayerMutationType::TRUNK)) {
            msg_print(_("あなたの鼻はもう象の鼻のようではなくなった。", "Your nose is no longer elephantine."));
            creature.remove_mutation(PlayerMutationType::TRUNK);
        }
    }

    if (gm_ptr->muta_which == PlayerMutationType::TRUNK) {
        if (creature.get_mutations().has(PlayerMutationType::BEAK)) {
            msg_print(_("硬いクチバシがなくなった。", "You no longer have a hard beak."));
            creature.remove_mutation(PlayerMutationType::BEAK);
        }
    }

    if (gm_ptr->muta_which == PlayerMutationType::HOMO_SEXUAL) {
        if (creature.get_mutations().has(PlayerMutationType::BI_SEXUAL)) {
            msg_print(_("あなたは同性しか興味を持たなくなった。", "You are only interested in the same sex."));
            creature.remove_mutation(PlayerMutationType::BI_SEXUAL);
        }
    }

    if (gm_ptr->muta_which == PlayerMutationType::BI_SEXUAL) {
        if (creature.get_mutations().has(PlayerMutationType::HOMO_SEXUAL)) {
            msg_print(_("あなたは同性のみならず、異性にも興味を持ち始めた。", "You have begun to be interested not only in the same sex but also in the opposite sex."));
            creature.remove_mutation(PlayerMutationType::HOMO_SEXUAL);
        }
    }
}

/*!
 * @brief プレイヤーに突然変異を与える
 * @param creature クリーチャーへの参照
 * @param choose_mut 与えたい突然変異のID、0ならばランダムに選択
 */
bool gain_mutation(CreatureEntity &creature, MUTATION_IDX choose_mut)
{
    glm_type tmp_gm;
    glm_type *gm_ptr = initialize_glm_type(&tmp_gm, choose_mut);
    sweep_gain_mutation(creature, gm_ptr);
    if (!gm_ptr->muta_chosen) {
        msg_print(_("普通になった気がする。", "You feel normal."));
        return false;
    }

    chg_virtue(creature, Virtue::CHANCE, 1);
    race_dependent_mutation(creature, gm_ptr);
    msg_print(_("突然変異した！", "You mutate!"));
    msg_print(gm_ptr->muta_desc);
    if (gm_ptr->muta_which != PlayerMutationType::MAX) {
        creature.add_mutation(gm_ptr->muta_which);
    }

    neutralize_base_status(creature, gm_ptr);
    neutralize_other_status(creature, gm_ptr);

    // 肛門破壊の突然変異を獲得した場合、尻の穴の装備を強制的に外す
    if (gm_ptr->muta_which == PlayerMutationType::DESTROYED_ASSHOLE) {
        auto &asshole_item = *creature.inventory[INVEN_ASSHOLE];
        if (asshole_item.is_valid()) {
            msg_print(_("肛門が破壊されたため、尻の穴の装備が外れた！", "Your asshole equipment has been removed due to destruction!"));
            (void)inven_takeoff(creature, INVEN_ASSHOLE, 255);
        }
    }

    creature.mutant_regenerate_mod = calc_mutant_regenerate_mod(creature);
    RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(creature);
    return true;
}

static void sweep_lose_mutation(CreatureEntity &creature, glm_type *glm_ptr)
{
    int attempts_left = 20;
    if (glm_ptr->choose_mut) {
        attempts_left = 1;
    }

    while (attempts_left--) {
        switch_lose_mutation(creature, glm_ptr);
        if (glm_ptr->muta_which != PlayerMutationType::MAX) {
            if (creature.get_mutations().has(glm_ptr->muta_which)) {
                glm_ptr->muta_chosen = true;
            }
        }

        if (glm_ptr->muta_chosen) {
            break;
        }
    }
}

/*!
 * @brief プレイヤーから突然変異を取り除く
 * @param creature クリーチャーへの参照
 * @param choose_mut 取り除きたい突然変異のID、0ならばランダムに消去
 */
bool lose_mutation(CreatureEntity &creature, MUTATION_IDX choose_mut)
{
    glm_type tmp_glm;
    glm_type *glm_ptr = initialize_glm_type(&tmp_glm, choose_mut);
    sweep_lose_mutation(creature, glm_ptr);
    if (!glm_ptr->muta_chosen) {
        return false;
    }

    msg_print(glm_ptr->muta_desc);
    if (glm_ptr->muta_which != PlayerMutationType::MAX) {
        creature.remove_mutation(glm_ptr->muta_which);
    }

    RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(creature);
    creature.mutant_regenerate_mod = calc_mutant_regenerate_mod(creature);
    return true;
}

void lose_all_mutations(CreatureEntity &creature)
{
    if (creature.get_mutations().any()) {
        chg_virtue(creature, Virtue::CHANCE, -5);
        msg_print(_("全ての突然変異が治った。", "You are cured of all mutations."));
        creature.clear_mutations();
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
        handle_stuff(creature);
        creature.mutant_regenerate_mod = calc_mutant_regenerate_mod(creature);
    }
}
