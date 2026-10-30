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
#include "system/creature-entity.h"
#include "system/deferred-update-requests.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/terrain/terrain-definition.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

namespace {
/*!
 * @brief 感知のメッセージを出すかを返す
 * @param notice メッセージを出す条件
 * @param detected 対象を 1 つでも感知したか
 * @param has_new 新しく感知した対象があったか
 * @return メッセージを出すならtrue
 */
bool should_notify(DetectionNotice notice, bool detected, bool has_new)
{
    switch (notice) {
    case DetectionNotice::ALWAYS:
        return detected;
    case DetectionNotice::NEW_ONLY:
        return has_new;
    case DetectionNotice::NEVER:
        return false;
    }

    return false;
}

/*!
 * @brief 地形の感知の結果
 */
struct FeatDetectionResult {
    bool detected = false; //!< 対象の地形が 1 つでもあったか
    bool has_disclosed = false; //!< ほかの地形に見せかけていたものを暴いたか
};
}

/*!
 * @brief プレイヤー周辺の地形を感知する
 * @param creature クリーチャーへの参照
 * @param range 効果範囲
 * @param flag 特定地形ID
 * @param known 地形から危険フラグを外すならTRUE
 * @return 感知の結果
 */
static FeatDetectionResult detect_feat_flag(CreatureEntity &creature, POSITION range, TerrainCharacteristics flag, bool known)
{
    auto &floor = *creature.get_floor();
    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS)) {
        range /= 3;
    }

    FeatDetectionResult result;
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
            if (grid.is_hidden()) {
                result.has_disclosed = true;
            }

            disclose_grid(creature, pos);
            grid.info |= (CAVE_MARK);
            lite_spot(creature, pos);
            result.detected = true;
        }
    }

    return result;
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
bool detect_traps(CreatureEntity &creature, POSITION range, bool known, DetectionNotice notice)
{
    const auto result = detect_feat_flag(creature, range, TerrainCharacteristics::TRAP, known);
    const auto detect = result.detected;
    if (!known && detect) {
        detect_feat_flag(creature, range, TerrainCharacteristics::TRAP, true);
    }

    if (known || detect) {
        creature.set_dtrap(true);
    }

    if (should_notify(notice, detect, result.has_disclosed)) {
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
bool detect_doors(CreatureEntity &creature, POSITION range, DetectionNotice notice)
{
    const auto result = detect_feat_flag(creature, range, TerrainCharacteristics::DOOR, true);
    const auto detect = result.detected;
    if (should_notify(notice, detect, result.has_disclosed)) {
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
bool detect_stairs(CreatureEntity &creature, POSITION range, DetectionNotice notice)
{
    const auto result = detect_feat_flag(creature, range, TerrainCharacteristics::STAIRS, true);
    const auto detect = result.detected;
    if (should_notify(notice, detect, result.has_disclosed)) {
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
    const auto detect = detect_feat_flag(creature, range, TerrainCharacteristics::HAS_GOLD, true).detected;
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
bool detect_objects_gold(CreatureEntity &creature, POSITION range, DetectionNotice notice)
{
    auto &floor = *creature.get_floor();
    POSITION range2 = range;
    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS)) {
        range2 /= 3;
    }

    /* Scan objects */
    auto detect = false;
    auto is_newly_found = false;
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
            is_newly_found |= item_ptr->marked.has_not(OmType::FOUND);
            item_ptr->marked.set(OmType::FOUND);
            lite_spot(creature, i_pos);
            detect = true;
        }
    }

    if (should_notify(notice, detect, is_newly_found)) {
        msg_print(_("財宝の存在を感じとった！", "You sense the presence of treasure!"));
    }

    if (detect_monsters_string(creature, range, "$", notice)) {
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
bool detect_objects_normal(CreatureEntity &creature, POSITION range, DetectionNotice notice)
{
    auto &floor = *creature.get_floor();
    POSITION range2 = range;
    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS)) {
        range2 /= 3;
    }

    auto detect = false;
    auto is_newly_found = false;
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
            is_newly_found |= item_ptr->marked.has_not(OmType::FOUND);
            item_ptr->marked.set(OmType::FOUND);
            lite_spot(creature, i_pos);
            detect = true;
        }
    }

    // 歌い続けている間は毎ターン感知するので、新しく見つけたときだけ描き直す
    if (is_newly_found) {
        DeferredUpdateRequests::get_instance().request_floor_item_lists_redrawing();
    }

    if (should_notify(notice, detect, is_newly_found)) {
        msg_print(_("アイテムの存在を感じとった！", "You sense the presence of objects!"));
    }

    if (detect_monsters_string(creature, range, "!=?|/`", notice)) {
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
    auto is_newly_found = false;
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
            is_newly_found |= item_ptr->marked.has_not(OmType::FOUND);
            item_ptr->marked.set(OmType::FOUND);
            lite_spot(creature, i_pos);
            detect = true;
        }
    }

    if (is_newly_found) {
        DeferredUpdateRequests::get_instance().request_floor_item_lists_redrawing();
    }

    if (detect) {
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
bool detect_monsters_normal(CreatureEntity &creature, POSITION range, DetectionNotice notice)
{
    auto &floor = *creature.get_floor();
    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS)) {
        range /= 3;
    }

    bool flag = false;
    auto has_new = false;
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
        if (!monster.has_constant_flag(MonsterConstantFlagType::MARK)) {
            has_new = true;
        }
        monster.set_constant_flags({ MonsterConstantFlagType::MARK, MonsterConstantFlagType::SHOW });
        update_monster(creature, i, false);
        flag = true;
    }

    if (should_notify(notice, flag, has_new)) {
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
bool detect_monsters_invis(CreatureEntity &creature, POSITION range, DetectionNotice notice)
{
    auto &floor = *creature.get_floor();
    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS)) {
        range /= 3;
    }

    auto flag = false;
    auto has_new = false;
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
        if (!monster.has_constant_flag(MonsterConstantFlagType::MARK)) {
            has_new = true;
        }
        monster.set_constant_flags({ MonsterConstantFlagType::MARK, MonsterConstantFlagType::SHOW });
        update_monster(creature, i, false);
        flag = true;
    }

    if (should_notify(notice, flag, has_new)) {
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
bool detect_monsters_evil(CreatureEntity &creature, POSITION range, DetectionNotice notice)
{
    auto &floor = *creature.get_floor();
    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS)) {
        range /= 3;
    }

    auto flag = false;
    auto has_new = false;
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
        }

        if (!monster.has_constant_flag(MonsterConstantFlagType::MARK)) {
            has_new = true;
        }

        monster.set_constant_flags({ MonsterConstantFlagType::MARK, MonsterConstantFlagType::SHOW });
        update_monster(creature, i, false);
        flag = true;
    }

    if (flag && ((notice == DetectionNotice::ALWAYS) || has_new)) {
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
bool detect_monsters_string(CreatureEntity &creature, POSITION range, concptr Match, DetectionNotice notice)
{
    auto &floor = *creature.get_floor();
    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS)) {
        range /= 3;
    }

    auto flag = false;
    auto has_new = false;
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
        if (!monster.has_constant_flag(MonsterConstantFlagType::MARK)) {
            has_new = true;
        }
        monster.set_constant_flags({ MonsterConstantFlagType::MARK, MonsterConstantFlagType::SHOW });
        update_monster(creature, i, false);
        flag = true;
    }

    if (should_notify(notice, flag, has_new)) {
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

/*!
 * @brief 感知の印が付いている範囲内のモンスターの印を、次の行動まで保つ
 * @param creature クリーチャーへの参照
 * @param range 効果範囲
 * @details
 * 感知の印 (MARK) は、SHOW が付いていなければ行動ごとに外れる。感知し続ける効果で、感知し直すまでの間に
 * 印が外れないよう SHOW を付け直す。新しくは感知しない。
 */
void keep_detected_monsters(CreatureEntity &creature, POSITION range)
{
    auto &floor = *creature.get_floor();
    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS)) {
        range /= 3;
    }

    const auto p_pos = creature.get_position();
    for (MONSTER_IDX i = 1; i < floor.m_max; i++) {
        auto &monster = floor.m_list[i];
        if (!monster.is_valid() || !monster.has_constant_flag(MonsterConstantFlagType::MARK)) {
            continue;
        }

        if (Grid::calc_distance(p_pos, monster.get_position()) > range) {
            continue;
        }

        monster.set_constant_flag(MonsterConstantFlagType::SHOW);
    }
}
