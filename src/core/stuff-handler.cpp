#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "player/player-status.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/redrawing-flags-updater.h"
#include "tracking/health-bar-tracker.h"
#include "tracking/lore-tracker.h"

/*!
 * @brief 全更新処理をチェックして処理していく
 */
void handle_stuff(CreatureEntity &creature)
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (rfu.any_stats()) {
        update_creature(creature);
    }

    if (rfu.any_main()) {
        redraw_stuff(creature);
    }

    if (rfu.any_sub()) {
        window_stuff(creature);
    }
}

/*
 * Track the given monster race
 */
void monster_race_track(CreatureEntity &creature, MonraceId r_idx)
{
    (void)creature;
    LoreTracker::get_instance().set_trackee(r_idx);
    RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::MONSTER_LORE);
}

/*
 * Track the given object kind
 */
void object_kind_track(CreatureEntity &creature, short bi_id)
{
    creature.set_tracking_bi_id(bi_id);
    RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::ITEM_KNOWLEDGE);
}

/*
 * Track a new monster
 * @param creature クリーチャーへの参照
 * @param m_idx トラッキング対象のモンスターID。0の時キャンセル
 * @param なし
 */
void health_track(CreatureEntity &creature, short m_idx)
{
    const auto &floor = *creature.get_floor();
    const auto &monster = floor.get_monster(m_idx);
    if (monster.is_riding()) {
        return;
    }

    HealthBarTracker::get_instance().set_trackee(m_idx);
}

bool update_player()
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::COMBINATION,
        StatusRecalculatingFlag::REORDER,
    };
    rfu.set_flags(flags_srf);
    rfu.set_flag(SubWindowRedrawingFlag::INVENTORY);
    return true;
}

bool redraw_player(CreatureEntity &creature)
{
    if (creature.get_current_mp() > creature.get_max_mp()) {
        creature.set_current_mp(creature.get_max_mp());
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::MP);
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::COMBINATION,
        StatusRecalculatingFlag::REORDER,
    };
    rfu.set_flags(flags_srf);
    rfu.set_flag(SubWindowRedrawingFlag::INVENTORY);
    return true;
}
