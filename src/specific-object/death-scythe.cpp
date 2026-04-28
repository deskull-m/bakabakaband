/*!
 * @brief 死の大鎌に特有の処理
 * @date 2020/05/23
 * @author Hourier
 * @details 現時点ではダメージ反射のみ。行数に注意して関数を追加しても良い.
 */

#include "specific-object/death-scythe.h"
#include "combat/attack-criticality.h"
#include "core/stuff-handler.h"
#include "inventory/inventory-slot-types.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object-enchant/tr-types.h"
#include "player-attack/player-attack.h"
#include "player-base/player-class.h"
#include "player-info/race-info.h"
#include "player/player-damage.h"
#include "player/player-status-flags.h"
#include "status/element-resistance.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "system/redrawing-flags-updater.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 死の大鎌ダメージが跳ね返ってきた時の、種族ごとのダメージ倍率を返す
 * @param creature クリーチャーへの参照
 * @return 倍率 (実際は1/10になる)
 */
static int calc_death_scythe_reflection_magnification_mimic_none(CreatureEntity &creature)
{
    switch (creature.prace) {
    case PlayerRaceType::YEEK:
    case PlayerRaceType::KLACKON:
    case PlayerRaceType::HUMAN:
    case PlayerRaceType::AMBERITE:
    case PlayerRaceType::DUNADAN:
    case PlayerRaceType::BARBARIAN:
    case PlayerRaceType::BEASTMAN:
        return 25;
    case PlayerRaceType::HALF_ORC:
    case PlayerRaceType::HALF_TROLL:
    case PlayerRaceType::HALF_OGRE:
    case PlayerRaceType::HALF_GIANT:
    case PlayerRaceType::HALF_TITAN:
    case PlayerRaceType::CYCLOPS:
    case PlayerRaceType::IMP:
    case PlayerRaceType::SKELETON:
    case PlayerRaceType::ZOMBIE:
    case PlayerRaceType::VAMPIRE:
    case PlayerRaceType::SPECTRE:
    case PlayerRaceType::BALROG:
    case PlayerRaceType::DRACONIAN:
        return 30;
    default:
        return 10;
    }
}

/*!
 * @brief 死の大鎌ダメージが跳ね返ってきた時の、変身中の種族も考慮したダメージ倍率を返す
 * @param creature クリーチャーへの参照
 * @return 倍率 (実際は1/10になる)
 */
static int calc_death_scythe_reflection_magnification(CreatureEntity &creature)
{
    switch (creature.get_mimic_form()) {
    case MimicKindType::NONE:
        return calc_death_scythe_reflection_magnification_mimic_none(creature);
    case MimicKindType::DEMON:
    case MimicKindType::DEMON_LORD:
    case MimicKindType::VAMPIRE:
        return 30;
    default:
        return 10;
    }
}

/*!
 * @brief 耐性等に応じて死の大鎌による反射ダメージ倍率を補正する
 * @param creature クリーチャーへの参照
 * @param magnification ダメージ倍率
 * @param death_scythe_flags 死の大鎌に関するオブジェクトフラグ配列
 */
static void compensate_death_scythe_reflection_magnification(CreatureEntity &creature, int *magnification, const TrFlags &death_scythe_flags)
{
    if ((creature.alignment < 0) && (*magnification < 20)) {
        *magnification = 20;
    }

    if (!(creature.has_resist_acid() || is_oppose_acid(creature) || creature.has_immune_acid()) && (*magnification < 25)) {
        *magnification = 25;
    }

    if (!(creature.has_resist_elec() || is_oppose_elec(creature) || creature.has_immune_elec()) && (*magnification < 25)) {
        *magnification = 25;
    }

    if (!(creature.has_resist_fire() || is_oppose_fire(creature) || creature.has_immune_fire()) && (*magnification < 25)) {
        *magnification = 25;
    }

    if (!(creature.has_resist_cold() || is_oppose_cold(creature) || creature.has_immune_cold()) && (*magnification < 25)) {
        *magnification = 25;
    }

    if (!(creature.has_resist_pois() || is_oppose_pois(creature)) && (*magnification < 25)) {
        *magnification = 25;
    }

    if (!CreatureClass(creature).equals(PlayerClassType::SAMURAI) && (death_scythe_flags.has(TR_FORCE_WEAPON)) && (creature.csp > (creature.msp / 30))) {
        creature.csp -= (1 + (creature.msp / 30));
        RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::MP);
        *magnification = *magnification * 3 / 2 + 20;
    }
}

/*!
 * @brief 死の大鎌による反射ダメージ倍率を更に上げる
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 */
static void death_scythe_reflection_critial_hit(player_attack_type *pa_ptr)
{
    if (!one_in_(6)) {
        return;
    }

    int more_magnification = 2;
    msg_format(_("グッサリ切り裂かれた！", "Your weapon cuts deep into yourself!"));
    while (one_in_(4)) {
        more_magnification++;
    }

    pa_ptr->attack_damage *= (int)more_magnification;
}

/*!
 * @brief 死の大鎌によるダメージ反射のメインルーチン
 * @param creature クリーチャーへの参照
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 */
void process_death_scythe_reflection(CreatureEntity &creature, player_attack_type *pa_ptr)
{
    sound(SoundKind::HIT);
    msg_format(_("ミス！ %sにかわされた。", "You miss %s."), pa_ptr->m_name);
    msg_print(_("振り回した大鎌が自分自身に返ってきた！", "Your scythe returns to you!"));

    auto *o_ptr = creature.inventory[INVEN_MAIN_HAND + pa_ptr->hand].get();
    const auto death_scythe_flags = o_ptr->get_flags();
    const auto num = o_ptr->damage_dice.num + creature.damage_dice_bonus[pa_ptr->hand].num;
    const auto sides = o_ptr->damage_dice.sides + creature.damage_dice_bonus[pa_ptr->hand].sides;
    pa_ptr->attack_damage = Dice::roll(num, sides);
    int magnification = calc_death_scythe_reflection_magnification(creature);
    compensate_death_scythe_reflection_magnification(creature, &magnification, death_scythe_flags);
    pa_ptr->attack_damage *= (int)magnification;
    pa_ptr->attack_damage /= 10;
    pa_ptr->attack_damage = critical_norm(creature, o_ptr->weight, o_ptr->to_h, pa_ptr->attack_damage, creature.to_h[pa_ptr->hand], pa_ptr->mode);
    death_scythe_reflection_critial_hit(pa_ptr);
    pa_ptr->attack_damage += (creature.to_d[pa_ptr->hand] + o_ptr->to_d);
    if (pa_ptr->attack_damage < 0) {
        pa_ptr->attack_damage = 0;
    }

    take_hit(creature, DAMAGE_FORCE, pa_ptr->attack_damage, _("死の大鎌", "Death scythe"));
    handle_stuff(creature);
}
