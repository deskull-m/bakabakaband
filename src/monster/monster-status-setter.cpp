#include "monster/monster-status-setter.h"
#include "avatar/avatar.h"
#include "cmd-visual/cmd-draw.h"
#include "core/speed-table.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "dungeon/quest-completion-checker.h"
#include "monster-floor/monster-move.h"
#include "monster-race/monster-kind-mask.h"
#include "monster-race/race-brightness-mask.h"
#include "monster/monster-describer.h"
#include "monster/monster-processor.h"
#include "monster/monster-util.h"
#include "monster/smart-learn-types.h"
#include "player/player-status-flags.h"
#include "system/angband-system.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/projection-path-calculator.h"
#include "tracking/health-bar-tracker.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <string>

/*!
 * @brief モンスターをペットにする
 * @param creature クリーチャーへの参照
 * @param m_ptr モンスター情報構造体の参照ポインタ
 */
void set_pet(CreatureEntity &creature, CreatureEntity &target)
{
    QuestCompletionChecker(creature, target).complete();
    target.set_constant_flag(MonsterConstantFlagType::PET);
    target.set_alliance_idx(AllianceType::NONE);
    if (target.get_monrace().kind_flags.has_none_of(alignment_mask)) {
        target.set_sub_align(SUB_ALIGN_NEUTRAL);
    }
}

/*!
 * @brief モンスターを怒らせる
 * Anger the monster
 * @param m_ptr モンスター情報構造体の参照ポインタ
 */
void anger_monster(CreatureEntity &creature, CreatureEntity &target)
{
    if (AngbandSystem::get_instance().is_phase_out() || !target.is_friendly()) {
        return;
    }

    const auto m_name = monster_desc(creature, target, 0);
    msg_format(_("%s^\u306f\u6012\u3063\u305f\uff01", "%s^ gets angry!"), m_name.data());
    target.set_hostile();
    chg_virtue(creature, Virtue::INDIVIDUALISM, 1);
    chg_virtue(creature, Virtue::HONOUR, -1);
    chg_virtue(creature, Virtue::JUSTICE, -1);
    chg_virtue(creature, Virtue::COMPASSION, -1);
}

/*!
 * @brief モンスターの睡眠状態値をセットする。0で起きる
 * @param floor フロアへの参照
 * @param m_idx モンスター参照ID
 * @param v セットする値
 * @return 別途更新処理が必要な場合TRUEを返す
 */
bool set_monster_csleep(FloorType &floor, MONSTER_IDX m_idx, int v)
{
    if (!floor.set_monster_timed_effect(m_idx, CreatureTimedEffect::SLEEP_OR_PARALYSIS, v, 10000)) {
        return false;
    }

    auto &monster = floor.get_monster(m_idx);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (monster.is_visible_on_map()) {
        HealthBarTracker::get_instance().set_flag_if_tracking(m_idx);
        if (monster.is_riding()) {
            rfu.set_flag(MainWindowRedrawingFlag::UHEALTH);
        }
    }

    if (monster.get_monrace().brightness_flags.has_any_of(has_ld_mask)) {
        rfu.set_flag(StatusRecalculatingFlag::MONSTER_LITE);
    }

    return true;
}

/*!
 * @brief モンスターの加速状態値をセット
 * @param floor フロアへの参照
 * @param m_idx モンスター参照ID
 * @param v セットする値
 * @return 別途更新処理が必要な場合TRUEを返す
 */
bool set_monster_fast(FloorType &floor, MONSTER_IDX m_idx, int v)
{
    if (!floor.set_monster_timed_effect(m_idx, CreatureTimedEffect::ACCELERATION, v, 200)) {
        return false;
    }

    if (floor.get_monster(m_idx).is_riding()) {
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
    }

    return true;
}

/*
 * @brief モンスターの原則状態値をセット
 * @param floor フロアへの参照
 * @param m_idx モンスター参照ID
 * @param v セットする値
 * @return 別途更新処理が必要な場合TRUEを返す
 */
bool set_monster_slow(FloorType &floor, MONSTER_IDX m_idx, int v)
{
    if (!floor.set_monster_timed_effect(m_idx, CreatureTimedEffect::DECELERATION, v, 200)) {
        return false;
    }

    if (floor.get_monster(m_idx).is_riding()) {
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
    }

    return true;
}

/*!
 * @brief モンスターの朦朧状態値をセット
 * @param floor フロアへの参照
 * @param m_idx モンスター参照ID
 * @param v セットする値
 * @return 別途更新処理が必要な場合TRUEを返す
 */
bool set_monster_stunned(FloorType &floor, MONSTER_IDX m_idx, int v)
{
    return floor.set_monster_timed_effect(m_idx, CreatureTimedEffect::STUN, v, 200);
}

/*!
 * @brief モンスターの混乱状態値をセット
 * @param floor フロアへの参照
 * @param m_idx モンスター参照ID
 * @param v セットする値
 * @return 別途更新処理が必要な場合TRUEを返す
 */
bool set_monster_confused(FloorType &floor, MONSTER_IDX m_idx, int v)
{
    return floor.set_monster_timed_effect(m_idx, CreatureTimedEffect::CONFUSION, v, 200);
}

/*!
 * @brief モンスターの恐慌状態値をセット
 * @param floor フロアへの参照
 * @param m_idx モンスター参照ID
 * @param v セットする値
 * @return 別途更新処理が必要な場合TRUEを返す
 */
bool set_monster_monfear(FloorType &floor, MONSTER_IDX m_idx, int v)
{
    auto &monster = floor.get_monster(m_idx);

    // 狂乱状態のモンスターは恐怖しない
    if (monster.is_frenzied() && v > 0) {
        return false;
    }

    if (!floor.set_monster_timed_effect(m_idx, CreatureTimedEffect::FEAR, v, 200)) {
        return false;
    }

    if (monster.is_visible_on_map()) {
        HealthBarTracker::get_instance().set_flag_if_tracking(m_idx);
        if (monster.is_riding()) {
            RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::UHEALTH);
        }
    }

    return true;
}

/*!
 * @brief モンスターの無敵状態値をセット
 * @param floor フロアへの参照
 * @param m_idx モンスター参照ID
 * @param v セットする値
 * @param energy_need TRUEならば無敵解除時に行動ターン消費を行う
 * @return 別途更新処理が必要な場合TRUEを返す
 */
bool set_monster_invulner(FloorType &floor, MONSTER_IDX m_idx, int v, bool energy_need)
{
    auto &monster = floor.get_monster(m_idx);
    const auto notice = floor.set_monster_timed_effect(m_idx, CreatureTimedEffect::INVULNERABILITY, v, 200);

    // 無敵が解除された (notice かつ v<=0) 場合、行動ターン消費のオプション処理。
    if (notice && (v <= 0) && energy_need && !AngbandWorld::get_instance().is_wild_mode()) {
        monster.add_energy_need(ENERGY_NEED());
    }

    if (!notice) {
        return false;
    }

    if (monster.is_visible_on_map()) {
        HealthBarTracker::get_instance().set_flag_if_tracking(m_idx);
        if (monster.is_riding()) {
            RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::UHEALTH);
        }
    }

    return true;
}

/*!
 * @brief モンスターの時間停止処理
 * @param creature クリーチャーへの参照
 * @param m_idx 時間停止を行う敵のモンスターID
 * @param num 時間停止を行った敵が行動できる回数
 * @param vs_player TRUEならば時間停止開始処理を行う
 * @return 時間停止が行われている状態ならばTRUEを返す
 * @details monster_desc() は視認外のモンスターについて「何か」と返してくるので、この関数ではLOSや透明視等を判定する必要はない
 */
bool set_monster_timewalk(CreatureEntity &creature, MONSTER_IDX m_idx, int num, bool vs_player)
{
    auto &floor = *creature.get_floor();
    auto &monster = floor.get_monster(m_idx);
    auto &world = AngbandWorld::get_instance();
    const auto &monrace = monster.get_real_monrace();
    if (world.timewalk_m_idx) {
        return false;
    }

    if (vs_player) {
        const auto m_name = monster_desc(creature, monster, 0);
        const auto time_message = monrace.get_message(m_name, MonsterMessageType::MESSAGE_TIMESTOP);
        if (time_message) {
            msg_print(*time_message);
        }
        msg_erase();
    }

    if (creature.has_resist_time()) {
        msg_print(_("しかし、あなたは時を止める力を打ち消した！", "But, you have countered power of time stop!"));
        return false;
    }

    world.timewalk_m_idx = m_idx;
    if (vs_player) {
        do_cmd_redraw(creature);
    }

    while (num--) {
        if (!monster.is_valid()) {
            break;
        }

        process_monster(creature, world.timewalk_m_idx);
        monster.reset_target();
        handle_stuff(creature);
        if (vs_player) {
            term_xtra(TERM_XTRA_DELAY, 500);
        }
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::MAP);
    rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
    static constexpr auto flags = {
        SubWindowRedrawingFlag::OVERHEAD,
        SubWindowRedrawingFlag::DUNGEON,
    };
    rfu.set_flags(flags);
    world.timewalk_m_idx = 0;
    const auto p_pos = creature.get_position();
    const auto m_pos = monster.get_position();
    auto should_output_message = floor.has_los_at(m_pos);
    should_output_message &= projectable(floor, p_pos, m_pos);
    if (vs_player || should_output_message) {
        const auto m_name = monster_desc(creature, monster, 0);
        const auto time_message = monrace.get_message(m_name, MonsterMessageType::MESSAGE_TIMESTART);
        if (time_message) {
            msg_print(*time_message);
        }
        msg_erase();
    }

    handle_stuff(creature);
    return true;
}
