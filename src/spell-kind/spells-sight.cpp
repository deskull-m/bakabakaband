#include "spell-kind/spells-sight.h"
#include "avatar/avatar.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "game-option/birth-options.h"
#include "game-option/map-screen-options.h"
#include "grid/grid.h"
#include "io/cursor.h"
#include "io/input-key-acceptor.h"
#include "locale/english.h"
#include "monster-race/monster-kind-mask.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "monster/smart-learn-types.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/projection-path-calculator.h"
#include "term/screen-processor.h"
#include "tracking/lore-tracker.h"
#include "view/display-messages.h"

/*!
 * @brief 視界内モンスターに魔法効果を与える / Apply a "project()" directly to all viewable monsters
 * @param typ 属性効果
 * @param dam 効果量
 * @return 効力があった場合TRUEを返す
 * @details
 * <pre>
 * Note that affected monsters are NOT auto-tracked by this usage.
 *
 * To avoid misbehavior when monster deaths have side-effects,
 * this is done in two passes. -- JDL
 * </pre>
 */
bool project_all_los(CreatureEntity &creature, AttributeType typ, int dam)
{
    auto &floor = *creature.get_floor();
    const auto p_pos = creature.get_position();
    for (short i = 1; i < floor.m_max; i++) {
        auto &monster = floor.get_monster(i);
        if (!monster.is_valid()) {
            continue;
        }

        const auto m_pos = monster.get_position();
        if (!floor.has_los_at(m_pos) || !projectable(floor, p_pos, m_pos)) {
            continue;
        }

        monster.set_temporary_flag(MonsterTemporaryFlagType::LOS);
    }

    BIT_FLAGS flg = PROJECT_JUMP | PROJECT_KILL | PROJECT_HIDE;
    auto obvious = false;
    for (short i = 1; i < floor.m_max; i++) {
        auto &monster = floor.get_monster(i);
        if (!monster.has_temporary_flag(MonsterTemporaryFlagType::LOS)) {
            continue;
        }

        monster.reset_temporary_flag(MonsterTemporaryFlagType::LOS);
        const auto m_pos = monster.get_position();
        if (project(creature, 0, 0, m_pos.y, m_pos.x, dam, typ, flg).notice) {
            obvious = true;
        }
    }

    return obvious;
}

/*!
 * @brief 視界内モンスターを加速する処理 / Speed monsters
 * @param creature クリーチャーへの参照
 * @return 効力があった場合TRUEを返す
 */
bool speed_monsters(CreatureEntity &creature)
{
    return project_all_los(creature, AttributeType::OLD_SPEED, creature.level);
}

/*!
 * @brief 視界内モンスターを加速する処理 / Slow monsters
 * @param creature クリーチャーへの参照
 * @return 効力があった場合TRUEを返す
 */
bool slow_monsters(CreatureEntity &creature, int power)
{
    return project_all_los(creature, AttributeType::OLD_SLOW, power);
}

/*!
 * @brief 視界内モンスターを眠らせる処理 / Sleep monsters
 * @param creature クリーチャーへの参照
 * @return 効力があった場合TRUEを返す
 */
bool sleep_monsters(CreatureEntity &creature, int power)
{
    return project_all_los(creature, AttributeType::OLD_SLEEP, power);
}

/*!
 * @brief 視界内の邪悪なモンスターをテレポート・アウェイさせる処理 / Banish evil monsters
 * @param creature クリーチャーへの参照
 * @return 効力があった場合TRUEを返す
 */
bool banish_evil(CreatureEntity &creature, int dist)
{
    return project_all_los(creature, AttributeType::AWAY_EVIL, dist);
}

/*!
 * @brief 視界内のアンデッド・モンスターを恐怖させる処理 / Turn undead
 * @return 効力があった場合TRUEを返す
 */
bool turn_undead(CreatureEntity &creature)
{
    bool tester = (project_all_los(creature, AttributeType::TURN_UNDEAD, creature.level));
    if (tester) {
        chg_virtue(creature, Virtue::UNLIFE, -1);
    }
    return tester;
}

/*!
 * @brief 視界内のアンデッド・モンスターにダメージを与える処理 / Dispel undead monsters
 * @param creature クリーチャーへの参照
 * @return 効力があった場合TRUEを返す
 */
bool dispel_undead(CreatureEntity &creature, int dam)
{
    bool tester = (project_all_los(creature, AttributeType::DISP_UNDEAD, dam));
    if (tester) {
        chg_virtue(creature, Virtue::UNLIFE, -2);
    }
    return tester;
}

/*!
 * @brief 視界内の邪悪なモンスターにダメージを与える処理 / Dispel evil monsters
 * @param creature クリーチャーへの参照
 * @return 効力があった場合TRUEを返す
 */
bool dispel_evil(CreatureEntity &creature, int dam)
{
    return project_all_los(creature, AttributeType::DISP_EVIL, dam);
}

/*!
 * @brief 視界内の善良なモンスターにダメージを与える処理 / Dispel good monsters
 * @param creature クリーチャーへの参照
 * @return 効力があった場合TRUEを返す
 */
bool dispel_good(CreatureEntity &creature, int dam)
{
    return project_all_los(creature, AttributeType::DISP_GOOD, dam);
}

/*!
 * @brief 視界内のあらゆるモンスターにダメージを与える処理 / Dispel all monsters
 * @param creature クリーチャーへの参照
 * @return 効力があった場合TRUEを返す
 */
bool dispel_monsters(CreatureEntity &creature, int dam)
{
    return project_all_los(creature, AttributeType::DISP_ALL, dam);
}

/*!
 * @brief 視界内の生命のあるモンスターにダメージを与える処理 / Dispel 'living' monsters
 * @param creature クリーチャーへの参照
 * @return 効力があった場合TRUEを返す
 */
bool dispel_living(CreatureEntity &creature, int dam)
{
    return project_all_los(creature, AttributeType::DISP_LIVING, dam);
}

/*!
 * @brief 視界内の悪魔系モンスターにダメージを与える処理 / Dispel 'living' monsters
 * @param creature クリーチャーへの参照
 * @return 効力があった場合TRUEを返す
 */
bool dispel_demons(CreatureEntity &creature, int dam)
{
    return project_all_los(creature, AttributeType::DISP_DEMON, dam);
}

/*!
 * @brief 視界内のモンスターに「聖戦」効果を与える処理
 * @param creature クリーチャーへの参照
 * @return 効力があった場合TRUEを返す
 */
bool crusade(CreatureEntity &creature)
{
    return project_all_los(creature, AttributeType::CRUSADE, creature.level * 4);
}

/*!
 * @brief 視界内モンスターを怒らせる処理 / Wake up all monsters, and speed up "los" monsters.
 * @param creature クリーチャーへの参照
 * @param src_idx 怒らせる原因を起こしたモンスター(0ならばプレイヤー)
 */
void aggravate_monsters(CreatureEntity &creature, MONSTER_IDX src_idx)
{
    auto sleep = false;
    auto speed = false;
    auto &floor = *creature.get_floor();
    for (short i = 1; i < floor.m_max; i++) {
        auto &monster = floor.get_monster(i);
        if (!monster.is_valid()) {
            continue;
        }
        if (i == src_idx) {
            continue;
        }

        if (Grid::calc_distance(creature.get_position(), monster.get_position()) < MAX_PLAYER_SIGHT * 2) {
            if (monster.is_asleep()) {
                (void)set_monster_csleep(floor, i, 0);
                sleep = true;
            }

            if (!monster.is_pet()) {
                monster.get_monster_profile().mflag2.set(MonsterConstantFlagType::NOPET);
            }
        }

        if (floor.has_los_at({ monster.y, monster.x }) && !monster.is_pet()) {
            monster.get_monster_profile().mflag2.set(MonsterConstantFlagType::ANGER);
            (void)set_monster_fast(floor, i, monster.get_remaining_acceleration() + 100);
            speed = true;
        }
    }

    if (speed) {
        msg_print(_("付近で何かが突如興奮したような感じを受けた！", "You feel a sudden stirring nearby!"));
    } else if (sleep) {
        msg_print(_("何かが突如興奮したような騒々しい音が遠くに聞こえた！", "You hear a sudden stirring in the distance!"));
    }

    if (creature.riding) {
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
    }
}

/*!
 * @brief パニック・モンスター効果(プレイヤー視界範囲内) / Confuse monsters
 * @param creature クリーチャーへの参照
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool confuse_monsters(CreatureEntity &creature, int dam)
{
    return project_all_los(creature, AttributeType::OLD_CONF, dam);
}

/*!
 * @brief チャーム・モンスター効果(プレイヤー視界範囲内) / Charm monsters
 * @param creature クリーチャーへの参照
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool charm_monsters(CreatureEntity &creature, int dam)
{
    return project_all_los(creature, AttributeType::CHARM, dam);
}

/*!
 * @brief 動物魅了効果(プレイヤー視界範囲内) / Charm Animals
 * @param creature クリーチャーへの参照
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool charm_animals(CreatureEntity &creature, int dam)
{
    return project_all_los(creature, AttributeType::CONTROL_ANIMAL, dam);
}

/*!
 * @brief モンスター朦朧効果(プレイヤー視界範囲内) / Stun monsters
 * @param creature クリーチャーへの参照
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool stun_monsters(CreatureEntity &creature, int dam)
{
    return project_all_los(creature, AttributeType::STUN, dam);
}

/*!
 * @brief モンスター停止効果(プレイヤー視界範囲内) / Stasis monsters
 * @param creature クリーチャーへの参照
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool stasis_monsters(CreatureEntity &creature, int dam)
{
    return project_all_los(creature, AttributeType::STASIS, dam);
}

/*!
 * @brief モンスター精神攻撃効果(プレイヤー視界範囲内) / Mindblast monsters
 * @param creature クリーチャーへの参照
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool mindblast_monsters(CreatureEntity &creature, int dam)
{
    return project_all_los(creature, AttributeType::PSI, dam);
}

/*!
 * @brief モンスター追放効果(プレイヤー視界範囲内) / Banish all monsters
 * @param creature クリーチャーへの参照
 * @param dist 効力（距離）
 * @return 作用が実際にあった場合TRUEを返す
 */
bool banish_monsters(CreatureEntity &creature, int dist)
{
    return project_all_los(creature, AttributeType::AWAY_ALL, dist);
}

/*!
 * @brief 邪悪退散効果(プレイヤー視界範囲内) / Turn evil
 * @param creature クリーチャーへの参照
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool turn_evil(CreatureEntity &creature, int dam)
{
    return project_all_los(creature, AttributeType::TURN_EVIL, dam);
}

/*!
 * @brief 全モンスター退散効果(プレイヤー視界範囲内) / Turn everyone
 * @param creature クリーチャーへの参照
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool turn_monsters(CreatureEntity &creature, int dam)
{
    return project_all_los(creature, AttributeType::TURN_ALL, dam);
}

/*!
 * @brief 死の光線(プレイヤー視界範囲内) / Death-ray all monsters (note: OBSCENELY powerful)
 * @param creature クリーチャーへの参照
 * @return 作用が実際にあった場合TRUEを返す
 */
bool deathray_monsters(CreatureEntity &creature)
{
    return project_all_los(creature, AttributeType::DEATH_RAY, creature.level * 200);
}

/*!
 * @brief 調査したモンスターの情報を表示する
 * @param creature クリーチャーへの参照
 * @param m_ptr モンスター情報への参照ポインタ
 * @param r_ptr モンスター種族への参照ポインタ
 * @return 調査結果 善悪アライメント、最大HP、残りHP、AC、速度、ステータス
 */
std::string probed_monster_info(CreatureEntity &creature, CreatureEntity &target, const MonraceDefinition &monrace)
{
    if (!target.is_original_ap()) {
        if (target.is_kage()) {
            target.get_monster_profile().mflag2.reset(MonsterConstantFlagType::KAGE);
        }

        target.ap_r_idx = target.r_idx;
        lite_spot(creature, target.get_position());
    }

    const auto m_name = monster_desc(creature, target, MD_IGNORE_HALLU | MD_INDEF_HIDDEN);

    concptr align;
    if (monrace.kind_flags.has_all_of(alignment_mask)) {
        align = _("善悪", "good&evil");
    } else if (monrace.kind_flags.has(MonsterKindType::EVIL)) {
        align = _("邪悪", "evil");
    } else if (monrace.kind_flags.has(MonsterKindType::GOOD)) {
        align = _("善良", "good");
    } else if ((target.get_sub_align() & (SUB_ALIGN_EVIL | SUB_ALIGN_GOOD)) == (SUB_ALIGN_EVIL | SUB_ALIGN_GOOD)) {
        align = _("中立(善悪)", "neutral(good&evil)");
    } else if (target.get_sub_align() & SUB_ALIGN_EVIL) {
        align = _("中立(邪悪)", "neutral(evil)");
    } else if (target.get_sub_align() & SUB_ALIGN_GOOD) {
        align = _("中立(善良)", "neutral(good)");
    } else {
        align = _("中立", "neutral");
    }

    const auto speed = target.get_temporary_speed() - STANDARD_SPEED;
    constexpr auto mes = _("%s ... 属性:%s HP:%d/%d AC:%d 速度:%s%d 経験:", "%s ... align:%s HP:%d/%d AC:%d speed:%s%d exp:");
    auto result = format(mes, m_name.data(), align, (int)target.hp, (int)target.maxhp, target.get_ac(), (speed > 0) ? "+" : "", speed);

    if (monrace.get_next().is_valid()) {
        result.append(format("%d/%d ", target.exp, monrace.next_exp));
    } else {
        result.append("xxx ");
    }

    if (target.is_asleep()) {
        result.append(_("睡眠 ", "sleeping "));
    }
    if (target.is_stunned()) {
        result.append(_("朦朧 ", "stunned "));
    }
    if (target.is_fearful()) {
        result.append(_("恐怖 ", "scared "));
    }
    if (target.is_confused()) {
        result.append(_("混乱 ", "confused "));
    }
    if (target.is_invulnerable()) {
        result.append(_("無敵 ", "invulnerable "));
    }
    return result;
}

/*!
 * @brief 周辺モンスターを調査する / Probe nearby monsters
 * @return 効力があった場合TRUEを返す
 */
bool probing(CreatureEntity &creature)
{
    bool cu = game_term->scr->cu;
    bool cv = game_term->scr->cv;
    game_term->scr->cu = 0;
    game_term->scr->cv = 1;

    auto &floor = *creature.get_floor();
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    auto probe = false;
    for (short i = 1; i < floor.m_max; i++) {
        auto &monster = floor.get_monster(i);
        auto &monrace = monster.get_monrace();
        if (!monster.is_valid()) {
            continue;
        }
        if (!floor.has_los_at({ monster.y, monster.x })) {
            continue;
        }
        if (!monster.is_visible_on_map()) {
            continue;
        }

        if (!probe) {
            msg_print(_("調査中...", "Probing..."));
        }
        msg_erase();

        const auto probe_result = probed_monster_info(creature, monster, monrace);
        prt(probe_result, 0, 0);

        message_add(probe_result);
        rfu.set_flag(SubWindowRedrawingFlag::MESSAGE);
        handle_stuff(creature);
        move_cursor_relative(monster.y, monster.x);
        inkey();
        term_erase(0, 0);
        const auto mes = monrace.probe_lore();
        if (mes) {
            msg_print(*mes);
            msg_erase();
            if (LoreTracker::get_instance().is_tracking(monster.r_idx)) {
                RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::MONSTER_LORE);
            }
        }

        probe = true;
    }

    game_term->scr->cu = cu;
    game_term->scr->cv = cv;
    term_fresh();

    if (probe) {
        chg_virtue(creature, Virtue::KNOWLEDGE, 1);
        msg_print(_("これで全部です。", "That's all."));
    }

    return probe;
}
