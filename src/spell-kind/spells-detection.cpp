#include "spell-kind/spells-detection.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon-flag-types.h"
#include "floor/floor-save-util.h"
#include "grid/grid.h"
#include "grid/trap.h"
#include "monster-race/monster-race-hook.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "object/object-mark-types.h"
#include "object/tval-types.h"
#include "realm/realm-song-numbers.h"
#include "realm/realm-song.h"
#include "spell-realm/spells-song.h"
#include "system/creature-entity.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain/terrain-definition.h"
#include "tracking/lore-tracker.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

/*!
 * @brief プレイヤー周辺の地形を感知する
 * @param creature クリーチャーへの参照
 * @param range 効果範囲
 * @param flag 特定地形ID
 * @param known 地形から危険フラグを外すならTRUE
 * @return 効力があった場合TRUEを返す
 */
static bool detect_feat_flag(CreatureEntity &creature, POSITION range, TerrainCharacteristics flag, bool known)
{
    auto &floor = *creature.get_floor();
    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS)) {
        range /= 3;
    }

    auto detect = false;
    for (const auto &pos : floor.get_area(FloorBoundary::OUTER_WALL_EXCLUSIVE)) {
        auto dist = Grid::calc_distance(creature.get_position(), pos);
        if (dist > range) {
            continue;
        }

        auto &grid = floor.get_grid(pos);
        if (flag == TerrainCharacteristics::TRAP) {
            /* Mark as detected */
            if (dist <= range && known) {
                if (dist <= range - 1) {
                    grid.info |= (CAVE_IN_DETECT);
                }

                grid.info &= ~(CAVE_UNSAFE);

                lite_spot(creature, pos);
            }
        }

        if (grid.has(flag)) {
            disclose_grid(creature, pos);
            grid.info |= (CAVE_MARK);
            lite_spot(creature, pos);
            detect = true;
        }
    }

    return detect;
}

/*!
 * @brief プレイヤー周辺のトラップを感知する / Detect all traps on current panel
 * @param creature クリーチャーへの参照
 * @param range 効果範囲
 * @param known 感知外範囲を超える警告フラグを立てる場合TRUEを返す
 * @return 効力があった場合TRUEを返す
 * @details
 * 吟遊詩人による感知についてはFALSEを返す
 */
bool detect_traps(CreatureEntity &creature, POSITION range, bool known)
{
    bool detect = detect_feat_flag(creature, range, TerrainCharacteristics::TRAP, known);
    if (!known && detect) {
        detect_feat_flag(creature, range, TerrainCharacteristics::TRAP, true);
    }

    if (known || detect) {
        creature.dtrap = true;
    }

    if (music_singing(creature, MUSIC_DETECT) && get_singing_count(creature) > 0) {
        detect = false;
    }

    if (detect) {
        msg_print(_("トラップの存在を感じとった！", "You sense the presence of traps!"));
    }

    return detect;
}

/*!
 * @brief プレイヤー周辺のドアを感知する / Detect all doors on current panel
 * @param creature クリーチャーへの参照
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_doors(CreatureEntity &creature, POSITION range)
{
    bool detect = detect_feat_flag(creature, range, TerrainCharacteristics::DOOR, true);

    if (music_singing(creature, MUSIC_DETECT) && get_singing_count(creature) > 0) {
        detect = false;
    }
    if (detect) {
        msg_print(_("ドアの存在を感じとった！", "You sense the presence of doors!"));
    }

    return detect;
}

/*!
 * @brief プレイヤー周辺の階段を感知する / Detect all stairs on current panel
 * @param creature クリーチャーへの参照
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_stairs(CreatureEntity &creature, POSITION range)
{
    bool detect = detect_feat_flag(creature, range, TerrainCharacteristics::STAIRS, true);

    if (music_singing(creature, MUSIC_DETECT) && get_singing_count(creature) > 0) {
        detect = false;
    }
    if (detect) {
        msg_print(_("階段の存在を感じとった！", "You sense the presence of stairs!"));
    }

    return detect;
}

/*!
 * @brief プレイヤー周辺の地形財宝を感知する / Detect any treasure on the current panel
 * @param creature クリーチャーへの参照
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_treasure(CreatureEntity &creature, POSITION range)
{
    bool detect = detect_feat_flag(creature, range, TerrainCharacteristics::HAS_GOLD, true);

    if (music_singing(creature, MUSIC_DETECT) && get_singing_count(creature) > 6) {
        detect = false;
    }
    if (detect) {
        msg_print(_("埋蔵された財宝の存在を感じとった！", "You sense the presence of buried treasure!"));
    }

    return detect;
}
/*!
 * @brief プレイヤー周辺のアイテム財宝を感知する / Detect all "gold" objects on the current panel
 * @param creature クリーチャーへの参照
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_objects_gold(CreatureEntity &creature, POSITION range)
{
    auto &floor = *creature.get_floor();
    POSITION range2 = range;
    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS)) {
        range2 /= 3;
    }

    /* Scan objects */
    auto detect = false;
    for (const auto &item_ptr : floor.o_list) {
        if (!item_ptr->is_valid()) {
            continue;
        }

        if (item_ptr->is_held_by_monster()) {
            continue;
        }

        const auto i_pos = item_ptr->get_position();
        if (Grid::calc_distance(creature.get_position(), i_pos) > range2) {
            continue;
        }

        if (item_ptr->bi_key.tval() == ItemKindType::GOLD) {
            item_ptr->marked.set(OmType::FOUND);
            lite_spot(creature, i_pos);
            detect = true;
        }
    }

    if (music_singing(creature, MUSIC_DETECT) && get_singing_count(creature) > 6) {
        detect = false;
    }
    if (detect) {
        msg_print(_("財宝の存在を感じとった！", "You sense the presence of treasure!"));
    }

    if (detect_monsters_string(creature, range, "$")) {
        detect = true;
    }

    return detect;
}

/*!
 * @brief 通常のアイテムオブジェクトを感知する / Detect all "normal" objects on the current panel
 * @param creature クリーチャーへの参照
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_objects_normal(CreatureEntity &creature, POSITION range)
{
    auto &floor = *creature.get_floor();
    POSITION range2 = range;
    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS)) {
        range2 /= 3;
    }

    auto detect = false;
    for (const auto &item_ptr : floor.o_list) {
        if (!item_ptr->is_valid()) {
            continue;
        }
        if (item_ptr->is_held_by_monster()) {
            continue;
        }

        const auto i_pos = item_ptr->get_position();
        if (Grid::calc_distance(creature.get_position(), i_pos) > range2) {
            continue;
        }

        if (item_ptr->bi_key.tval() != ItemKindType::GOLD) {
            item_ptr->marked.set(OmType::FOUND);
            lite_spot(creature, i_pos);
            detect = true;
        }
    }

    if (music_singing(creature, MUSIC_DETECT) && get_singing_count(creature) > 6) {
        detect = false;
    }
    if (detect) {
        RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::FOUND_ITEMS);
        msg_print(_("アイテムの存在を感じとった！", "You sense the presence of objects!"));
    }

    if (detect_monsters_string(creature, range, "!=?|/`")) {
        detect = true;
    }

    return detect;
}

static bool is_object_magically(const ItemKindType tval)
{
    switch (tval) {
    case ItemKindType::WHISTLE:
    case ItemKindType::AMULET:
    case ItemKindType::RING:
    case ItemKindType::STAFF:
    case ItemKindType::WAND:
    case ItemKindType::ROD:
    case ItemKindType::SCROLL:
    case ItemKindType::POTION:
        return true;
    default:
        return false;
    }
}

/*!
 * @brief 魔法効果のあるのアイテムオブジェクトを感知する
 * @param creature クリーチャーへの参照
 * @param range 効果範囲
 * @return 1つ以上感知したか否か
 */
bool detect_objects_magic(CreatureEntity &creature, POSITION range)
{
    auto &floor = *creature.get_floor();
    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS)) {
        range /= 3;
    }

    auto detect = false;
    for (const auto &item_ptr : floor.o_list) {
        if (!item_ptr->is_valid() || item_ptr->is_held_by_monster()) {
            continue;
        }

        const auto i_pos = item_ptr->get_position();
        if (Grid::calc_distance(creature.get_position(), i_pos) > range) {
            continue;
        }

        auto has_bonus = item_ptr->to_a > 0;
        has_bonus |= item_ptr->to_h + item_ptr->to_d > 0;
        if (item_ptr->is_fixed_or_random_artifact() || item_ptr->is_ego() || is_object_magically(item_ptr->bi_key.tval()) || item_ptr->is_spell_book() || has_bonus) {
            item_ptr->marked.set(OmType::FOUND);
            lite_spot(creature, i_pos);
            detect = true;
        }
    }

    if (detect) {
        RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::FOUND_ITEMS);
        msg_print(_("魔法のアイテムの存在を感じとった！", "You sense the presence of magic objects!"));
    }

    return detect;
}

/*!
 * @brief 一般のモンスターを感知する / Detect all "normal" monsters on the current panel
 * @param creature クリーチャーへの参照
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_normal(CreatureEntity &creature, POSITION range)
{
    auto &floor = *creature.get_floor();
    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS)) {
        range /= 3;
    }

    bool flag = false;
    // [提案 14b] AI ターゲット選定共通化を活用
    const auto p_pos = creature.get_position();
    const auto sees_invis = creature.can_see_invisible();
    const auto matched = creature.collect_creatures([&](const CreatureEntity &mon) {
        if (Grid::calc_distance(p_pos, mon.get_position()) > range) {
            return false;
        }
        return mon.get_monrace().misc_flags.has_not(MonsterMiscType::INVISIBLE) || sees_invis;
    });
    for (auto i : matched) {
        auto &monster = floor.m_list[i];
        monster.set_constant_flags({ MonsterConstantFlagType::MARK, MonsterConstantFlagType::SHOW });
        update_monster(creature, i, false);
        flag = true;
    }

    if (music_singing(creature, MUSIC_DETECT) && get_singing_count(creature) > 3) {
        flag = false;
    }
    if (flag) {
        msg_print(_("モンスターの存在を感じとった！", "You sense the presence of monsters!"));
    }

    return flag;
}

/*!
 * @brief 不可視のモンスターを感知する / Detect all "invisible" monsters around the creature
 * @param creature クリーチャーへの参照
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_invis(CreatureEntity &creature, POSITION range)
{
    auto &floor = *creature.get_floor();
    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS)) {
        range /= 3;
    }

    const auto &tracker = LoreTracker::get_instance();
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    auto flag = false;
    // [提案 14b]
    const auto p_pos = creature.get_position();
    const auto matched = creature.collect_creatures([&](const CreatureEntity &mon) {
        if (Grid::calc_distance(p_pos, mon.get_position()) > range) {
            return false;
        }
        return mon.get_monrace().misc_flags.has(MonsterMiscType::INVISIBLE);
    });
    for (auto i : matched) {
        auto &monster = floor.m_list[i];
        if (tracker.is_tracking(monster.r_idx)) {
            rfu.set_flag(SubWindowRedrawingFlag::MONSTER_LORE);
        }
        monster.set_constant_flags({ MonsterConstantFlagType::MARK, MonsterConstantFlagType::SHOW });
        update_monster(creature, i, false);
        flag = true;
    }

    if (music_singing(creature, MUSIC_DETECT) && get_singing_count(creature) > 3) {
        flag = false;
    }
    if (flag) {
        msg_print(_("透明な生物の存在を感じとった！", "You sense the presence of invisible creatures!"));
    }

    return flag;
}

/*!
 * @brief 邪悪なモンスターを感知する / Detect all "evil" monsters on current panel
 * @param creature クリーチャーへの参照
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_evil(CreatureEntity &creature, POSITION range)
{
    auto &floor = *creature.get_floor();
    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS)) {
        range /= 3;
    }

    const auto &tracker = LoreTracker::get_instance();
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    auto flag = false;
    // [提案 14b]
    const auto p_pos = creature.get_position();
    const auto matched = creature.collect_creatures([&](const CreatureEntity &mon) {
        if (Grid::calc_distance(p_pos, mon.get_position()) > range) {
            return false;
        }
        return mon.get_monrace().kind_flags.has(MonsterKindType::EVIL);
    });
    for (auto i : matched) {
        auto &monster = floor.m_list[i];
        auto &monrace = monster.get_monrace();
        if (monster.is_original_ap()) {
            monrace.r_kind_flags.set(MonsterKindType::EVIL);
            if (tracker.is_tracking(monster.r_idx)) {
                rfu.set_flag(SubWindowRedrawingFlag::MONSTER_LORE);
            }
        }
        monster.set_constant_flags({ MonsterConstantFlagType::MARK, MonsterConstantFlagType::SHOW });
        update_monster(creature, i, false);
        flag = true;
    }

    if (flag) {
        msg_print(_("邪悪なる生物の存在を感じとった！", "You sense the presence of evil creatures!"));
    }

    return flag;
}

/*!
 * @brief 無生命のモンスターを感知する(アンデッド、悪魔系を含む) / Detect all "nonliving", "undead" or "demonic" monsters on current panel
 * @param creature クリーチャーへの参照
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_nonliving(CreatureEntity &creature, POSITION range)
{
    auto &floor = *creature.get_floor();
    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS)) {
        range /= 3;
    }

    const auto &tracker = LoreTracker::get_instance();
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    auto flag = false;
    // [提案 14b]
    const auto p_pos = creature.get_position();
    const auto matched = creature.collect_creatures([&](const CreatureEntity &mon) {
        if (Grid::calc_distance(p_pos, mon.get_position()) > range) {
            return false;
        }
        return !mon.has_living_flag();
    });
    for (auto i : matched) {
        auto &monster = floor.m_list[i];
        if (tracker.is_tracking(monster.r_idx)) {
            rfu.set_flag(SubWindowRedrawingFlag::MONSTER_LORE);
        }
        monster.set_constant_flags({ MonsterConstantFlagType::MARK, MonsterConstantFlagType::SHOW });
        update_monster(creature, i, false);
        flag = true;
    }

    if (flag) {
        msg_print(_("自然でないモンスターの存在を感じた！", "You sense the presence of unnatural beings!"));
    }

    return flag;
}

/*!
 * @brief 精神のあるモンスターを感知する / Detect all monsters it has mind on current panel
 * @param creature クリーチャーへの参照
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_mind(CreatureEntity &creature, POSITION range)
{
    auto &floor = *creature.get_floor();
    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS)) {
        range /= 3;
    }

    const auto &tracker = LoreTracker::get_instance();
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    auto flag = false;
    // [提案 14b]
    const auto p_pos = creature.get_position();
    const auto matched = creature.collect_creatures([&](const CreatureEntity &mon) {
        if (Grid::calc_distance(p_pos, mon.get_position()) > range) {
            return false;
        }
        return mon.get_monrace().misc_flags.has_not(MonsterMiscType::EMPTY_MIND);
    });
    for (auto i : matched) {
        auto &monster = floor.m_list[i];
        if (tracker.is_tracking(monster.r_idx)) {
            rfu.set_flag(SubWindowRedrawingFlag::MONSTER_LORE);
        }
        monster.set_constant_flags({ MonsterConstantFlagType::MARK, MonsterConstantFlagType::SHOW });
        update_monster(creature, i, false);
        flag = true;
    }

    if (flag) {
        msg_print(_("殺気を感じとった！", "You sense the presence of someone's mind!"));
    }

    return flag;
}

/*!
 * @brief 該当シンボルのモンスターを感知する / Detect all (string) monsters on current panel
 * @param creature クリーチャーへの参照
 * @param range 効果範囲
 * @param Match 対応シンボルの混じったモンスター文字列(複数指定化)
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_string(CreatureEntity &creature, POSITION range, concptr Match)
{
    auto &floor = *creature.get_floor();
    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS)) {
        range /= 3;
    }

    const auto &tracker = LoreTracker::get_instance();
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    auto flag = false;
    // [提案 14b]
    const auto p_pos = creature.get_position();
    const auto matched = creature.collect_creatures([&](const CreatureEntity &mon) {
        if (Grid::calc_distance(p_pos, mon.get_position()) > range) {
            return false;
        }
        return angband_strchr(Match, mon.get_monrace().symbol_definition.character) != nullptr;
    });
    for (auto i : matched) {
        auto &monster = floor.m_list[i];
        if (tracker.is_tracking(monster.r_idx)) {
            rfu.set_flag(SubWindowRedrawingFlag::MONSTER_LORE);
        }
        monster.set_constant_flags({ MonsterConstantFlagType::MARK, MonsterConstantFlagType::SHOW });
        update_monster(creature, i, false);
        flag = true;
    }

    if (music_singing(creature, MUSIC_DETECT) && get_singing_count(creature) > 3) {
        flag = false;
    }
    if (flag) {
        msg_print(_("モンスターの存在を感じとった！", "You sense the presence of monsters!"));
    }

    return flag;
}

/*!
 * @brief 全感知処理 / Detect everything
 * @param creature クリーチャーへの参照
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_all(CreatureEntity &creature, POSITION range)
{
    bool detect = false;
    if (detect_traps(creature, range, true)) {
        detect = true;
    }
    if (detect_doors(creature, range)) {
        detect = true;
    }
    if (detect_stairs(creature, range)) {
        detect = true;
    }
    if (detect_objects_gold(creature, range)) {
        detect = true;
    }
    if (detect_objects_normal(creature, range)) {
        detect = true;
    }
    if (detect_monsters_invis(creature, range)) {
        detect = true;
    }
    if (detect_monsters_normal(creature, range)) {
        detect = true;
    }
    return detect;
}
