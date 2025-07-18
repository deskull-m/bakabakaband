/*!
 * @brief 素手で攻撃することに補正のある職業 (修行僧、狂戦士、練気術師)の打撃処理
 * @date 2020/05/23
 * @author Hourier
 * @details 練気術師は騎乗していない時
 */

#include "mind/monk-attack.h"
#include "cmd-action/cmd-attack.h"
#include "combat/attack-criticality.h"
#include "core/speed-table.h"
#include "core/stuff-handler.h"
#include "floor/geometry.h"
#include "game-option/cheat-options.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-force-trainer.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "player-attack/player-attack.h"
#include "player-base/player-class.h"
#include "player-info/monk-data-type.h"
#include "player/attack-defense-types.h"
#include "player/special-defense-types.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "timed-effect/timed-effects.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief 朦朧への抵抗値を計算する
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @return 朦朧への抵抗値
 */
static int calc_stun_resistance(player_attack_type *pa_ptr)
{
    const auto &monrace = pa_ptr->m_ptr->get_monrace();
    int resist_stun = 0;
    if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
        resist_stun += 88;
    }

    if (monrace.resistance_flags.has(MonsterResistanceType::NO_STUN)) {
        resist_stun += 66;
    }

    if (monrace.resistance_flags.has(MonsterResistanceType::NO_CONF)) {
        resist_stun += 33;
    }

    if (monrace.resistance_flags.has(MonsterResistanceType::NO_SLEEP)) {
        resist_stun += 33;
    }

    if (monrace.kind_flags.has(MonsterKindType::UNDEAD) || monrace.kind_flags.has(MonsterKindType::NONLIVING)) {
        resist_stun += 66;
    }

    return resist_stun;
}

/*!
 * @brief 技のランダム選択回数を決定する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 技のランダム選択回数
 * @details ランダム選択は一番強い技が最終的に選択されるので、回数が多いほど有利
 */
static int calc_max_blow_selection_times(PlayerType *player_ptr)
{
    PlayerClass pc(player_ptr);
    if (pc.monk_stance_is(MonkStanceType::BYAKKO)) {
        return player_ptr->lev < 3 ? 1 : player_ptr->lev / 3;
    }

    if (pc.monk_stance_is(MonkStanceType::SUZAKU)) {
        return 1;
    }

    if (pc.monk_stance_is(MonkStanceType::GENBU)) {
        return 1;
    }

    return player_ptr->lev < 7 ? 1 : player_ptr->lev / 7;
}

/*!
 * @brief プレイヤーのレベルと技の難度を加味しつつ、確率で一番強い技を選ぶ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 技のランダム選択回数
 * @return 技の行使に必要な最低レベル
 */
static int select_blow(PlayerType *player_ptr, player_attack_type *pa_ptr, int max_blow_selection_times)
{
    int min_level = 1;
    const martial_arts *old_ptr = &ma_blows[0];
    const auto is_wizard = AngbandWorld::get_instance().wizard;
    for (int times = 0; times < max_blow_selection_times; times++) {
        do {
            pa_ptr->ma_ptr = &rand_choice(ma_blows);
            if (PlayerClass(player_ptr).equals(PlayerClassType::FORCETRAINER) && (pa_ptr->ma_ptr->min_level > 1)) {
                min_level = pa_ptr->ma_ptr->min_level + 3;
            } else {
                min_level = pa_ptr->ma_ptr->min_level;
            }
        } while ((min_level > player_ptr->lev) || (randint1(player_ptr->lev) < pa_ptr->ma_ptr->chance));

        const auto effects = player_ptr->effects();
        const auto is_stunned = effects->stun().is_stunned();
        const auto is_confused = effects->confusion().is_confused();
        if ((pa_ptr->ma_ptr->min_level <= old_ptr->min_level) || is_stunned || is_confused) {
            pa_ptr->ma_ptr = old_ptr;
            continue;
        }

        old_ptr = pa_ptr->ma_ptr;
        if (is_wizard && cheat_xtra) {
            msg_print(_("攻撃を再選択しました。", "Attack re-selected."));
        }
    }

    if (PlayerClass(player_ptr).equals(PlayerClassType::FORCETRAINER)) {
        min_level = std::max(1, pa_ptr->ma_ptr->min_level - 3);
    } else {
        min_level = pa_ptr->ma_ptr->min_level;
    }

    return min_level;
}

static int process_monk_additional_effect(player_attack_type *pa_ptr, int *stun_effect)
{
    int special_effect = 0;
    auto *r_ptr = &pa_ptr->m_ptr->get_monrace();
    if (pa_ptr->ma_ptr->effect == MA_KNEE) {
        if (is_male(*r_ptr)) {
            msg_format(_("%sに金的膝蹴りをくらわした！", "You hit %s in the groin with your knee!"), pa_ptr->m_name);
            sound(SoundKind::PAIN);
            special_effect = MA_KNEE;
        } else {
            msg_format(pa_ptr->ma_ptr->desc, pa_ptr->m_name);
        }
    }

    else if (pa_ptr->ma_ptr->effect == MA_SLOW) {
        if (!(r_ptr->behavior_flags.has(MonsterBehaviorType::NEVER_MOVE) || r_ptr->symbol_char_is_any_of("~#{}.UjmeEv$,DdsbBFIJQSXclnw!=?"))) {
            msg_format(_("%sの足首に関節蹴りをくらわした！", "You kick %s in the ankle."), pa_ptr->m_name);
            special_effect = MA_SLOW;
        } else {
            msg_format(pa_ptr->ma_ptr->desc, pa_ptr->m_name);
        }
    } else {
        if (pa_ptr->ma_ptr->effect) {
            *stun_effect = (pa_ptr->ma_ptr->effect / 2) + randint1(pa_ptr->ma_ptr->effect / 2);
        }

        msg_format(pa_ptr->ma_ptr->desc, pa_ptr->m_name);
    }

    return special_effect;
}

/*!
 * @brief 攻撃の重さ (修行僧と練気術師における武器重量)を決定する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 重さ
 */
WEIGHT calc_monk_attack_weight(PlayerType *player_ptr)
{
    WEIGHT weight = 8;
    PlayerClass pc(player_ptr);
    if (pc.monk_stance_is(MonkStanceType::SUZAKU)) {
        weight = 4;
    }

    if (pc.equals(PlayerClassType::FORCETRAINER) && (get_current_ki(player_ptr) != 0)) {
        weight += (get_current_ki(player_ptr) / 30);
        if (weight > 20) {
            weight = 20;
        }
    }

    return weight;
}

/*!
 * @brief 急所攻撃による追加効果を与える
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param stun_effect 朦朧の残りターン
 * @param resist_stun 朦朧への抵抗値
 * @param special_effect 技を繰り出した時の追加効果
 */
static void process_attack_vital_spot(PlayerType *player_ptr, player_attack_type *pa_ptr, int *stun_effect, int *resist_stun, const int special_effect)
{
    const auto &monrace = pa_ptr->m_ptr->get_monrace();
    if ((special_effect == MA_KNEE) && ((pa_ptr->attack_damage + player_ptr->to_d[pa_ptr->hand]) < pa_ptr->m_ptr->hp)) {
        msg_format(_("%s^は苦痛にうめいている！", "%s^ moans in agony!"), pa_ptr->m_name);
        *stun_effect = 7 + randint1(13);
        *resist_stun /= 3;
        return;
    }

    if ((special_effect == MA_SLOW) && ((pa_ptr->attack_damage + player_ptr->to_d[pa_ptr->hand]) < pa_ptr->m_ptr->hp)) {
        const auto is_unique = monrace.kind_flags.has_not(MonsterKindType::UNIQUE);
        if (is_unique && (randint1(player_ptr->lev) > monrace.level) && (pa_ptr->m_ptr->mspeed > STANDARD_SPEED - 50)) {
            msg_format(_("%s^は足をひきずり始めた。", "You've hobbled %s."), pa_ptr->m_name);
            pa_ptr->m_ptr->mspeed -= 10;
        }
    }
}

/*!
 * @brief 朦朧効果を受けたモンスターのステータス表示
 * @param player_ptr プレイヤーの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param grid グリッドへの参照
 * @param stun_effect 朦朧の残りターン
 * @param resist_stun 朦朧への抵抗値
 */
static void print_stun_effect(PlayerType *player_ptr, player_attack_type *pa_ptr, const int stun_effect, const int resist_stun)
{
    const auto &monrace = pa_ptr->m_ptr->get_monrace();
    if (stun_effect && ((pa_ptr->attack_damage + player_ptr->to_d[pa_ptr->hand]) < pa_ptr->m_ptr->hp)) {
        if (player_ptr->lev > randint1(monrace.level + resist_stun + 10)) {
            if (set_monster_stunned(player_ptr, pa_ptr->g_ptr->m_idx, stun_effect + pa_ptr->m_ptr->get_remaining_stun())) {
                msg_format(_("%s^はフラフラになった。", "%s^ is stunned."), pa_ptr->m_name);
            } else {
                msg_format(_("%s^はさらにフラフラになった。", "%s^ is more stunned."), pa_ptr->m_name);
            }
        }
    }
}

/*!
 * @brief 強力な素手攻撃ができる職業 (修行僧、狂戦士、練気術師)の素手攻撃処理メインルーチン
 * @param player_ptr プレイヤーの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param grid グリッドへの参照
 */
void process_monk_attack(PlayerType *player_ptr, player_attack_type *pa_ptr)
{
    int resist_stun = calc_stun_resistance(pa_ptr);
    int max_blow_selection_times = calc_max_blow_selection_times(player_ptr);
    int min_level = select_blow(player_ptr, pa_ptr, max_blow_selection_times);

    const auto num = pa_ptr->ma_ptr->damage_dice.num + player_ptr->damage_dice_bonus[pa_ptr->hand].num;
    const auto sides = pa_ptr->ma_ptr->damage_dice.sides + player_ptr->damage_dice_bonus[pa_ptr->hand].sides;
    pa_ptr->attack_damage = Dice::roll(num, sides);
    if (player_ptr->special_attack & ATTACK_SUIKEN) {
        pa_ptr->attack_damage *= 2;
    }

    int stun_effect = 0;
    int special_effect = process_monk_additional_effect(pa_ptr, &stun_effect);
    WEIGHT weight = calc_monk_attack_weight(player_ptr);
    pa_ptr->attack_damage = critical_norm(player_ptr, player_ptr->lev * weight, min_level, pa_ptr->attack_damage, player_ptr->to_h[0], HISSATSU_NONE);
    process_attack_vital_spot(player_ptr, pa_ptr, &stun_effect, &resist_stun, special_effect);
    print_stun_effect(player_ptr, pa_ptr, stun_effect, resist_stun);
}

bool double_attack(PlayerType *player_ptr)
{
    const auto dir = get_rep_dir(player_ptr);
    if (!dir) {
        return false;
    }

    const auto pos = player_ptr->get_neighbor(dir);
    const auto &grid = player_ptr->current_floor_ptr->get_grid(pos);
    const auto has_monster = grid.has_monster();
    if (!has_monster) {
        msg_print(_("その方向にはモンスターはいません。", "You don't see any monster in this direction"));
        msg_erase();
        return true;
    }

    if (one_in_(3)) {
        msg_print(_("あーたたたたたたたたたたたたたたたたたたたたたた！！！", "Ahhhtatatatatatatatatatatatatatataatatatatattaaaaa!!!!"));
    } else if (one_in_(2)) {
        msg_print(_("無駄無駄無駄無駄無駄無駄無駄無駄無駄無駄無駄無駄！！！", "Mudamudamudamudamudamudamudamudamudamudamudamudamuda!!!!"));
    } else {
        msg_print(_("オラオラオラオラオラオラオラオラオラオラオラオラ！！！", "Oraoraoraoraoraoraoraoraoraoraoraoraoraoraoraoraora!!!!"));
    }

    do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_NONE);
    if (has_monster) {
        handle_stuff(player_ptr);
        do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_NONE);
    }

    player_ptr->energy_need += ENERGY_NEED();
    return true;
}
