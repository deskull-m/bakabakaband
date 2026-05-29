/*!
 * @file monster-ability-caster.cpp
 * @brief モンスター化したプレイヤーが種族固有能力 (ability_flags) を行使する処理の実装
 */

#include "mind/monster-ability-caster.h"
#include "action/action-limited.h"
#include "avatar/avatar.h"
#include "blue-magic/blue-magic-caster.h"
#include "blue-magic/learnt-power-getter.h"
#include "core/asking-player.h"
#include "game-option/disturbance-options.h"
#include "game-option/input-options.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-race/race-ability-mask.h"
#include "mspell/monster-power-table.h"
#include "player-status/player-energy.h"
#include "realm/realm-types.h"
#include "spell/spell-info.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/redrawing-flags-updater.h"
#include "util/candidate-selector.h"
#include "view/display-messages.h"
#include <vector>

namespace {
/*!
 * @brief プレイヤーの現在 monrace から発動可能な能力一覧を収集する
 * @details monster_powers (青魔法として扱える能力テーブル) に存在し、かつ
 *          monrace.ability_flags が立っている能力のみを castable とみなす。
 */
std::vector<MonsterAbilityType> collect_usable_abilities(const CreatureEntity &creature)
{
    std::vector<MonsterAbilityType> abilities;
    const auto &ability_flags = creature.get_monrace().ability_flags;
    for (const auto &[ability, _] : monster_powers) {
        if (ability_flags.has(ability)) {
            abilities.push_back(ability);
        }
    }
    return abilities;
}
}

bool can_use_monster_ability(const CreatureEntity &creature)
{
    if (creature.get_r_idx() == MonraceId::PLAYER) {
        return false;
    }
    return !collect_usable_abilities(creature).empty();
}

bool do_cmd_use_monster_ability(CreatureEntity &creature)
{
    if (cmd_limit_confused(creature)) {
        return false;
    }

    const auto abilities = collect_usable_abilities(creature);
    if (abilities.empty()) {
        msg_print(_("行使できる能力がない。", "You have no innate ability to use."));
        return false;
    }

    auto describer = [&creature](MonsterAbilityType ability) {
        const auto &power = monster_powers.at(ability);
        const auto need_mana = mod_need_mana(creature, power.smana, 0, RealmType::NONE);
        return format("%-30s MP:%2d", power.name, need_mana);
    };
    CandidateSelector cs(_("どの能力を行使しますか？", "Use which ability? "), 15);
    const auto choice = cs.select(abilities, describer);
    if (choice == abilities.end()) {
        return false;
    }

    const auto selected_ability = *choice;
    const auto &power = monster_powers.at(selected_ability);
    const auto need_mana = mod_need_mana(creature, power.smana, 0, RealmType::NONE);
    if (need_mana > creature.get_csp()) {
        msg_print(_("ＭＰが足りません。", "You do not have enough mana to use this power."));
        if (!over_exert) {
            return false;
        }

        if (!input_check(_("それでも挑戦しますか? ", "Attempt it anyway? "))) {
            return false;
        }
    }

    const auto chance = calculate_blue_magic_failure_probability(creature, power, need_mana);
    if (evaluate_percent(chance)) {
        if (flush_failure) {
            flush();
        }

        msg_print(_("うまく能力を行使できなかった。", "You failed to concentrate hard enough!"));
        sound(SoundKind::FAIL);
        if (RF_ABILITY_SUMMON_MASK.has(selected_ability)) {
            cast_learned_spell(creature, selected_ability, false);
        }
    } else {
        sound(SoundKind::ZAP);
        if (!cast_learned_spell(creature, selected_ability, true)) {
            return false;
        }
    }

    if (need_mana <= creature.get_csp()) {
        creature.sub_csp(need_mana);
    } else {
        const int oops = need_mana;
        creature.set_csp(0);
        creature.csp_frac = 0;
        msg_print(_("精神を集中しすぎて気を失ってしまった！", "You faint from the effort!"));
        (void)BadStatusSetter(creature).mod_paralysis(randnum1<short>(5 * oops + 1));
        chg_virtue(creature, Virtue::KNOWLEDGE, -10);
        if (one_in_(2)) {
            const auto perm = one_in_(4);
            msg_print(_("体を悪くしてしまった！", "You have damaged your health!"));
            (void)dec_stat(creature, A_CON, 15 + randint1(10), perm);
        }
    }

    PlayerEnergy(creature).set_player_turn_energy(100);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::MP);
    static constexpr auto flags = {
        SubWindowRedrawingFlag::PLAYER,
        SubWindowRedrawingFlag::SPELL,
    };
    rfu.set_flags(flags);
    return true;
}
