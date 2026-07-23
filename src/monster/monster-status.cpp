#include "monster/monster-status.h"
#include "autopick/autopick-pref-processor.h"
#include "core/speed-table.h"
#include "floor/geometry.h"
#include "game-option/birth-options.h"
#include "game-option/text-display-options.h"
#include "grid/grid.h"
#include "monster-race/monster-kind-mask.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-special-flags.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-status-setter.h" //!< @todo 相互依存. 後で何とかする.
#include "monster/monster-update.h"
#include "system/angband-system.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monrace/monrace-service.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "tracking/health-bar-tracker.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#if JP
#else
#include "monster/monster-description-types.h"
#endif

static uint32_t csleep_noise;

/*!
 * @brief モンスターIDからPOWERFULフラグの有無を取得する /
 * @param floor フロアへの参照
 * @param m_idx モンスターID
 * @return POWERFULフラグがあればTRUE、なければFALSEを返す。
 */
bool monster_is_powerful(const FloorType &floor, MONSTER_IDX m_idx)
{
    const auto &monster = floor.get_monster(m_idx);
    const auto &monrace = monster.get_monrace();
    return monrace.misc_flags.has(MonsterMiscType::POWERFUL);
}

/*!
 * @brief モンスターIDからモンスターのレベルを取得する(ただし最低1を保証する) /
 * @param m_idx モンスターID
 * @return モンスターのレベル
 */
DEPTH monster_level_idx(const FloorType &floor, MONSTER_IDX m_idx)
{
    const auto &monster = floor.get_monster(m_idx);
    const auto &monrace = monster.get_monrace();
    return (monrace.level >= 1) ? monrace.level : 1;
}

/*!
 * @brief モンスターに与えたダメージの修正処理 /
 * Modify the physical damage done to the monster.
 * @param creature クリーチャーへの参照
 * @param m_ptr ダメージを受けるモンスターの構造体参照ポインタ
 * @param dam ダメージ基本値
 * @param is_psy_spear 攻撃手段が光の剣ならばTRUE
 * @return 修正を行った結果のダメージ量
 * @details RES_ALL持ちはAC軽減後のダメージを1/100に補正する. 光の剣は無敵を無効化する. 一定確率で無敵は貫通できる.
 */
int mon_damage_mod(CreatureEntity &creature, const CreatureEntity &target, int dam, bool is_psy_spear)
{
    const auto &monrace = target.get_monrace();
    if (monrace.resistance_flags.has(MonsterResistanceType::RESIST_ALL) && dam > 0) {
        dam /= 100;
        if ((dam == 0) && one_in_(3)) {
            dam = 1;
        }
    }

    auto &race_info = target.get_monrace();

    if (race_info.special_flags.has(MonsterSpecialType::DIMINISH_MAX_DAMAGE)) {
        race_info.r_special_flags.set(MonsterSpecialType::DIMINISH_MAX_DAMAGE);
        if (dam > target.hp / 10) {
            dam = std::max(target.hp / 10, target.maxhp * 7 / 500);
            msg_format(_("%s^は致命的なダメージを抑えた！", "%s^ resisted a critical damage!"), monster_desc(creature, target, 0).data());
        }
    }

    if (!target.is_invulnerable()) {
        return dam;
    }

    if (is_psy_spear) {
        if (!creature.is_blind() && is_seen(creature, target)) {
            msg_print(_("バリアを切り裂いた！", "The barrier is penetrated!"));
        }

        return dam;
    }

    return one_in_(PENETRATE_INVULNERABILITY) ? dam : 0;
}

/*!
 * @brief モンスターの各種状態値を時間経過により更新するサブルーチン
 * @param floor フロアへの参照
 * @param m_idx モンスター参照ID
 * @param mte 更新するモンスターの時限ステータスID
 */
static void process_monsters_timed_effect_aux(CreatureEntity &creature, MONSTER_IDX m_idx, CreatureTimedEffect mte)
{
    auto &floor = *creature.get_floor();
    const auto &monster = floor.get_monster(m_idx);
    const auto cdis = Grid::calc_distance(creature.get_position(), monster.get_position());
    switch (mte) {
    case CreatureTimedEffect::SLEEP_OR_PARALYSIS: {
        auto &monrace = monster.get_monrace();
        auto is_wakeup = false;
        if (cdis < MAX_MONSTER_SENSING) {
            /* Handle "sensing radius" */
            if (cdis <= (monster.is_pet() ? ((monrace.aaf > MAX_PLAYER_SIGHT) ? MAX_PLAYER_SIGHT : monrace.aaf) : monrace.aaf)) {
                is_wakeup = true;
            }

            /* Handle "sight" and "aggravation" */
            else if ((cdis <= MAX_PLAYER_SIGHT) && floor.has_los_at({ monster.y, monster.x })) {
                is_wakeup = true;
            }

            // [提案C3第2弾] テレパシー持ちは感知半径 (MAX_MONSTER_SENSING) 内なら aaf/視線を問わず
            // プレイヤーを感知しうる (壁越し・遠距離)。ただし下の notice 確率判定は据え置きのため
            // 即覚醒ではなく、プレイヤーのステルスも引き続き効く。opt-in・既定OFF。
            else if (monster.has_race_granted_telepathy()) {
                is_wakeup = true;
            }
        }

        if (!is_wakeup) {
            break;
        }

        auto notice = randnum0<uint32_t>(1024);

        /* Nightmare monsters are more alert */
        if (ironman_nightmare) {
            notice /= 2;
        }

        /* Hack -- See if monster "notices" player */
        if ((notice * notice * notice) > csleep_noise) {
            break;
        }

        /* Hack -- amount of "waking" */
        /* Wake up faster near the player */
        auto d = (cdis < MAX_MONSTER_SENSING / 2) ? (MAX_MONSTER_SENSING / cdis) : 1;

        /* Hack -- amount of "waking" is affected by speed of player */
        d = (d * speed_to_energy(static_cast<byte>(creature.get_speed()))) / 10;
        if (d < 0) {
            d = 1;
        }

        /* Monster wakes up "a little bit" */

        /* Still asleep */
        if (!set_monster_csleep(floor, m_idx, monster.get_remaining_sleep() - d)) {
            /* Notice the "not waking up" */
            if (is_original_ap_and_seen(creature, monster)) {
                /* Hack -- Count the ignores */
                if (monrace.r_ignore < MAX_UCHAR) {
                    monrace.r_ignore++;
                }
            }

            break;
        }

        /* Notice the "waking up" */
        if (monster.is_visible_on_map()) {
            const auto m_name = monster_desc(creature, monster, 0);
            msg_format(_("%s^が目を覚ました。", "%s^ wakes up."), m_name.data());
        }

        if (is_original_ap_and_seen(creature, monster)) {
            /* Hack -- Count the wakings */
            if (monrace.r_wake < MAX_UCHAR) {
                monrace.r_wake++;
            }
        }

        break;
    }
    case CreatureTimedEffect::ACCELERATION:
        /* Reduce by one, note if expires */
        if (set_monster_fast(floor, m_idx, monster.get_remaining_acceleration() - 1)) {
            if (is_seen(creature, monster)) {
                const auto m_name = monster_desc(creature, monster, 0);
                msg_format(_("%s^はもう加速されていない。", "%s^ is no longer fast."), m_name.data());
            }
        }

        break;
    case CreatureTimedEffect::DECELERATION:
        /* Reduce by one, note if expires */
        if (set_monster_slow(floor, m_idx, monster.get_remaining_deceleration() - 1)) {
            if (is_seen(creature, monster)) {
                const auto m_name = monster_desc(creature, monster, 0);
                msg_format(_("%s^はもう減速されていない。", "%s^ is no longer slow."), m_name.data());
            }
        }

        break;
    case CreatureTimedEffect::STUN: {
        int rlev = monster.get_monrace().level;

        /* Recover from stun */
        if (set_monster_stunned(floor, m_idx, (randint0(10000) <= rlev * rlev) ? 0 : (monster.get_remaining_stun() - 1))) {
            /* Message if visible */
            if (is_seen(creature, monster)) {
                const auto m_name = monster_desc(creature, monster, 0);
                msg_format(_("%s^は朦朧状態から立ち直った。", "%s^ is no longer stunned."), m_name.data());
            }
        }

        break;
    }
    case CreatureTimedEffect::CONFUSION: {
        /* Reduce the confusion */
        if (!set_monster_confused(floor, m_idx, monster.get_remaining_confusion() - randint1(monster.get_monrace().level / 20 + 1))) {
            break;
        }

        /* Message if visible */
        if (is_seen(creature, monster)) {
            const auto m_name = monster_desc(creature, monster, 0);
            msg_format(_("%s^は混乱から立ち直った。", "%s^ is no longer confused."), m_name.data());
        }

        break;
    }
    case CreatureTimedEffect::FEAR: {
        /* Reduce the fear */
        if (!set_monster_monfear(floor, m_idx, monster.get_remaining_fear() - randint1(monster.get_monrace().level / 20 + 1))) {
            break;
        }

        /* Visual note */
        if (is_seen(creature, monster)) {
            const auto m_name = monster_desc(creature, monster, 0);
#ifdef JP
#else
            /* Acquire the monster possessive */
            const auto m_poss = monster_desc(creature, monster, MD_PRON_VISIBLE | MD_POSSESSIVE);
#endif
#ifdef JP
            msg_format("%s^は勇気を取り戻した。", m_name.data());
#else
            msg_format("%s^ recovers %s courage.", m_name.data(), m_poss.data());
#endif
        }

        break;
    }
    case CreatureTimedEffect::INVULNERABILITY: {
        /* Reduce by one, note if expires */
        if (!set_monster_invulner(floor, m_idx, monster.get_remaining_invulnerability() - 1, true)) {
            break;
        }

        if (is_seen(creature, monster)) {
            const auto m_name = monster_desc(creature, monster, 0);
            msg_format(_("%s^はもう無敵でない。", "%s^ is no longer invulnerable."), m_name.data());
        }

        break;
    }
    default:
        THROW_EXCEPTION(std::logic_error, format("Invalid CreatureTimedEffect is specified! %d", enum2i(mte)));
    }
}

/*!
 * @brief 全モンスターの各種状態値を時間経過により更新するメインルーチン
 * @param mte 更新するモンスターの時限ステータスID
 * @param creature クリーチャーへの参照
 * @details
 * Process the counters of monsters (once per 10 game turns)\n
 * These functions are to process monsters' counters same as player's.
 */
void process_monsters_timed_effect(CreatureEntity &creature, CreatureTimedEffect mte)
{
    const auto &floor = *creature.get_floor();
    const auto &cur_mproc_list = floor.mproc_list.at(mte);

    /* Hack -- calculate the "player noise" */
    if (mte == CreatureTimedEffect::SLEEP_OR_PARALYSIS) {
        csleep_noise = (1U << (30 - creature.get_skill_stealth()));
    }

    /* Process the monsters (backwards) */
    for (auto i = floor.mproc_max.at(mte) - 1; i >= 0; i--) {
        const auto m_idx = cur_mproc_list[i];
        process_monsters_timed_effect_aux(creature, m_idx, mte);
        HealthBarTracker::get_instance().set_flag_if_tracking(m_idx);
    }
}

/*!
 * @brief モンスターへの魔力消去処理
 * @param creature クリーチャーへの参照
 * @param m_idx 魔力消去を受けるモンスターの参照ID
 */
void dispel_monster_status(CreatureEntity &creature, MONSTER_IDX m_idx)
{
    auto &floor = *creature.get_floor();
    const auto &monster = floor.get_monster(m_idx);
    const auto m_name = monster_desc(creature, monster, 0);
    if (set_monster_invulner(floor, m_idx, 0, true)) {
        if (monster.is_visible_on_map()) {
            msg_format(_("%sはもう無敵ではない。", "%s^ is no longer invulnerable."), m_name.data());
        }
    }

    if (set_monster_fast(floor, m_idx, 0)) {
        if (monster.is_visible_on_map()) {
            msg_format(_("%sはもう加速されていない。", "%s^ is no longer fast."), m_name.data());
        }
    }

    if (set_monster_slow(floor, m_idx, 0)) {
        if (monster.is_visible_on_map()) {
            msg_format(_("%sはもう減速されていない。", "%s^ is no longer slow."), m_name.data());
        }
    }
}

/*!
 * @brief モンスターが蓄積した経験値に応じてレベルアップ (HP成長) する
 * @param creature プレイヤーへの参照 (可視判定・メッセージ表示用)
 * @param monster 対象モンスター
 * @return レベルアップ (HP成長) が発生したら true
 * @details 種族本来のレベル (monrace.level/2) を基準に、蓄積経験値が自種族の
 *          殺害時経験値 (mexp) の整数倍に達するごとに 1 レベル成長する。成長量は
 *          基準レベル分まで (実効レベルは最大で基準の 2 倍) に制限し暴走を防ぐ。
 *          進化を持つモンスターは next_exp 到達で進化 (exp リセット) するため、
 *          進化前に到達できる成長レベルは next_exp/mexp までに自然に制限される。
 *          実HP成長は CreatureEntity::grow_hp_table_to_level() (テーブル式) で行う。
 *          レベルアップした際、そのモンスターがプレイヤーの視界に入っていれば
 *          「～はレベルアップしたようだ」と通知する。
 */
static bool try_monster_level_up(CreatureEntity &creature, CreatureEntity &monster)
{
    const auto &monrace = monster.get_monrace();
    const auto base_level = std::clamp<int>(monrace.level / 2, 1, PY_MAX_LEVEL);
    const auto exp_unit = std::max<EXP>(1, monrace.mexp);
    const auto bonus = std::min<int>(static_cast<int>(monster.get_exp() / exp_unit), base_level);
    const auto target_level = std::min<int>(base_level + bonus, PY_MAX_LEVEL);
    if (target_level <= monster.get_level()) {
        return false;
    }

    // grow_hp_table_to_level() が set_level() で実効レベルを更新するため、獲得レベル数は
    // その前に確定させておく。
    const auto old_level = monster.get_level();

    // [提案 C2] grows_stats が立つ個体は能力値成長を先に行う (既定 OFF でバランス不変)。
    // grow_hp_table_to_level() の CON→HP 補正は呼出時点の CON を参照するため、能力値成長を
    // 先に反映することで、今回獲得したレベル分の HP に成長後の CON が反映され整合する。
    if (monrace.grows_stats) {
        monster.grow_stats_by_levels(target_level - old_level);
    }

    monster.grow_hp_table_to_level(target_level);

    // レベルアップしたモンスターがプレイヤーの視界に入っていれば通知する。
    if (monster.is_visible_on_map()) {
        const auto m_name = monster_desc(creature, monster, 0);
        msg_format(_("%sはレベルアップしたようだ。", "%s^ seems to have leveled up."), m_name.data());
    }

    return true;
}

/*!
 * @brief モンスターの経験値取得処理
 * @param creature クリーチャーへの参照
 * @param m_idx 経験値を得るモンスターの参照ID
 * @param monrace_id 撃破されたモンスター種族ID
 */
void monster_gain_exp(CreatureEntity &creature, MONSTER_IDX m_idx, MonraceId monrace_id)
{
    if (m_idx <= 0 || !MonraceList::is_valid(monrace_id)) {
        return;
    }

    auto &floor = *creature.get_floor();
    auto &monster = floor.get_monster(m_idx);
    if (!monster.is_valid()) {
        return;
    }

    auto &old_monrace = monster.get_monrace(); //!< @details 現在 (進化前)の種族.
    const auto &monraces = MonraceList::get_instance();
    const auto &monrace_defeated = monraces.get_monrace(monrace_id);
    if (AngbandSystem::get_instance().is_phase_out()) {
        return;
    }

    auto new_exp = monrace_defeated.mexp * monrace_defeated.level / (old_monrace.level + 2);
    if (monster.is_riding()) {
        new_exp = (new_exp + 1) / 2;
    }

    if (!floor.is_underground()) {
        new_exp /= 5;
    }

    monster.add_exp(new_exp);
    if (monster.is_chameleon()) {
        return;
    }

    // 蓄積経験値に応じてレベルアップ (HP成長) する。進化先の有無に関わらず発火する。
    const auto leveled_up = try_monster_level_up(creature, monster);

    auto &rfu = RedrawingFlagsUpdater::get_instance();

    // レベルアップで最大HP等が変化した場合は、進化しなくても騎乗中ならボーナス再計算、
    // 視認中なら再描画を行う (進化パスと同じ共有更新)。
    if (leveled_up) {
        if (monster.is_riding()) {
            rfu.set_flag(StatusRecalculatingFlag::BONUS);
        }

        update_monster(creature, m_idx, false);
        lite_spot(creature, monster.get_position());
    }

    // 以降は進化処理。進化先を持たないモンスターはレベルアップのみで終了する。
    if (old_monrace.next_exp == 0) {
        return;
    }

    if (monster.get_exp() < old_monrace.next_exp) {
        if (monster.is_riding()) {
            rfu.set_flag(StatusRecalculatingFlag::BONUS);
        }

        return;
    }

    const auto old_hp = monster.hp;
    const auto old_maxhp = monster.max_maxhp;
    const auto old_sub_align = monster.get_sub_align();

    /* Hack -- Reduce the racial counter of previous monster */
    monster.get_real_monrace().decrement_current_numbers();

    const auto m_name = monster_desc(creature, monster, 0);
    monster.set_r_idx(old_monrace.next_r_idx);

    /* Count the monsters on the level */
    monster.get_real_monrace().increment_current_numbers();
    monster.set_ap_r_idx(monster.get_r_idx());

    // 進化前にレベルアップ (HP成長) していた場合、level フィールドに旧レベルが残る。
    // 進化後は別種族として基準レベルから振り直すため、level を 0 にリセットして
    // get_level() が進化後種族の monrace.level/2 を返すようにする。
    monster.set_level(0);

    const auto &new_monrace = monster.get_monrace(); //!< @details 進化後の種族.
    // 進化後の最大HPも生成時 (place_monster_one) と同じテーブル式で算出する。
    // hit_dice を進化後種族のものに揃えてから roll_monster_hp_table() を呼ぶ。
    monster.hit_dice = new_monrace.hit_dice;
    monster.max_maxhp = monster.roll_monster_hp_table(new_monrace.misc_flags.has(MonsterMiscType::FORCE_MAXHP));
    if (ironman_nightmare) {
        const auto hp = monster.max_maxhp * 2;
        monster.max_maxhp = std::min(MONSTER_MAXHP, hp);
    }

    monster.maxhp = monster.max_maxhp;
    monster.hp = old_hp * monster.maxhp / old_maxhp;

    /* dealt damage is 0 at initial*/
    monster.set_dealt_damage(0);

    /* Extract the monster base speed */
    monster.set_individual_speed(floor.inside_arena);

    /* Sub-alignment of a monster */
    if (!monster.is_pet() && new_monrace.kind_flags.has_none_of(alignment_mask)) {
        monster.set_sub_align(old_sub_align);
    } else {
        monster.set_sub_align(SUB_ALIGN_NEUTRAL);
        if (new_monrace.kind_flags.has(MonsterKindType::EVIL)) {
            monster.add_sub_align(SUB_ALIGN_EVIL);
        }

        if (new_monrace.kind_flags.has(MonsterKindType::GOOD)) {
            monster.add_sub_align(SUB_ALIGN_GOOD);
        }
    }

    monster.set_exp(0);
    if (monster.is_pet() || monster.is_visible_on_map()) {
        const auto is_hallucinated = creature.is_hallucinated();
        if (!ignore_unview || player_can_see_bold(creature, monster.y, monster.x)) {
            if (is_hallucinated) {
                const auto ids = MonraceService::search([](const auto &monrace) { return monrace.kind_flags.has_not(MonsterKindType::UNIQUE); });
                const auto &monrace_hallucinated = monraces.get_monrace(rand_choice(ids));
                auto mes_evolution = _("%sは%sに進化した。", "%s^ evolved into %s.");
                auto mes_degeneration = _("%sは%sに退化した。", "%s^ degenerated into %s.");
                auto mes = randint0(2) == 0 ? mes_evolution : mes_degeneration;
                msg_format(mes, m_name.data(), monrace_hallucinated.name.data());
            } else {
                msg_format(_("%sは%sに進化した。", "%s^ evolved into %s."), m_name.data(), new_monrace.name.data());
            }
        }

        if (!is_hallucinated) {
            old_monrace.r_can_evolve = true;
        }

        /* Now you feel very close to this pet. */
        monster.set_parent_m_idx(0);
    }

    update_monster(creature, m_idx, false);
    lite_spot(creature, monster.get_position());

    if (monster.is_riding()) {
        rfu.set_flag(StatusRecalculatingFlag::BONUS);
    }
}
