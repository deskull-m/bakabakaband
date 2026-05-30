/*!
 * @brief モンスターをフロアに1体配置する処理
 * @date 2020/06/13
 * @author Hourier
 */

#include "alliance/alliance.h"
#include "birth/birth-body-spec.h"
#include "birth/birth-stat.h"
#include "core/disturbance.h"
#include "core/speed-table.h"
#include "dungeon/quest.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-save-util.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-types.h"
#include "grid/grid.h"
#include "monster-attack/monster-attack-player.h"
#include "monster-attack/monster-attack-table.h"
#include "monster-floor/monster-drop-generator.h"
#include "monster-floor/monster-move.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-kind-mask.h"
#include "monster-race/race-brightness-mask.h"
#include "monster-race/race-misc-flags.h"
#include "monster/monster-describer.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-timed-effects.h"
#include "monster/monster-update.h"
#include "monster/monster-util.h"
#include "object/warning.h"
#include "player-ability/player-ability-types.h"
#include "player/player-sex.h"
#include "player/player-status.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/redrawing-flags-updater.h"
#include "view/display-messages.h"
#include "wizard/wizard-messages.h"
#include "world/world.h"
#include <algorithm>
#include <range/v3/algorithm.hpp>
#include <time.h>

/*!
 * @param creature クリーチャーへの参照
 * @brief モンスターの表層IDを設定する / Set initial racial appearance of a monster
 * @param r_idx モンスター種族ID
 * @return モンスター種族の表層ID
 */
static MonraceId initial_r_appearance(CreatureEntity &creature, MonraceId r_idx, BIT_FLAGS generate_mode)
{
    if (creature.is_chargeman() && any_bits(generate_mode, PM_JURAL) && none_bits(generate_mode, PM_MULTIPLY | PM_KAGE)) {
        return MonraceId::ALIEN_JURAL;
    }

    if (MonraceList::get_instance().get_monrace(r_idx).misc_flags.has_not(MonsterMiscType::TANUKI)) {
        return r_idx;
    }

    get_mon_num_prep_enum(creature, MonraceHook::TANUKI);
    auto attempts = 1000;
    const auto &floor = *creature.get_floor();
    auto min = std::min(floor.base_level - 5, 50);
    while (--attempts) {
        auto ap_r_idx = get_mon_num(creature, 0, floor.base_level + 10, PM_NONE);
        if (MonraceList::get_instance().get_monrace(ap_r_idx).level >= min) {
            return ap_r_idx;
        }
    }

    return r_idx;
}

/*!
 * @brief ユニークが生成可能か評価する
 * @param creature クリーチャーへの参照
 * @param r_idx 生成モンスター種族
 * @return ユニークの生成が不可能な条件ならFALSE、それ以外はTRUE
 */
static bool check_unique_placeable(const FloorType &floor, MonraceId r_idx, BIT_FLAGS mode)
{
    if (AngbandSystem::get_instance().is_phase_out()) {
        return true;
    }

    if (any_bits(mode, PM_CLONE)) {
        return true;
    }

    auto *r_ptr = &MonraceList::get_instance().get_monrace(r_idx);
    auto is_unique = r_ptr->kind_flags.has(MonsterKindType::UNIQUE) || r_ptr->population_flags.has(MonsterPopulationType::NAZGUL);
    is_unique &= r_ptr->cur_num > 0;
    is_unique &= r_ptr->mob_num <= 0;
    if (is_unique) {
        return false;
    }

    if (r_ptr->population_flags.has(MonsterPopulationType::ONLY_ONE) && r_ptr->has_entity()) {
        return false;
    }

    if (!MonraceList::get_instance().is_selectable(r_idx)) {
        return false;
    }

    const auto is_deep = r_ptr->misc_flags.has(MonsterMiscType::FORCE_DEPTH) && (floor.dun_level < r_ptr->level);
    const auto is_questor = !ironman_nightmare || r_ptr->misc_flags.has(MonsterMiscType::QUESTOR);
    return !is_deep || !is_questor;
}

/*!
 * @brief クエスト内に生成可能か評価する
 * @param floor フロアへの参照
 * @param r_idx 生成モンスター種族
 * @return 生成が可能ならTRUE、不可能ならFALSE
 */
static bool check_quest_placeable(const FloorType &floor, MonraceId r_idx)
{
    if (!inside_quest(floor.get_quest_id())) {
        return true;
    }

    const auto &quests = QuestList::get_instance();
    const auto quest_id = floor.get_quest_id();
    const auto &quest = quests.get_quest(quest_id);
    if ((quest.type != QuestKindType::KILL_LEVEL) && (quest.type != QuestKindType::RANDOM)) {
        return true;
    }
    if (r_idx != quest.r_idx) {
        return true;
    }
    const auto has_quest_monrace = [&](const Pos2D &pos) {
        const auto &grid = floor.get_grid(pos);
        return grid.has_monster() && (floor.m_list[grid.m_idx].get_r_idx() == quest.r_idx);
    };
    const auto number_mon = ranges::count_if(floor.get_area(), has_quest_monrace);

    if (number_mon + quest.cur_num >= quest.max_num) {
        return false;
    }
    return true;
}

/*!
 * @brief 守りのルーン上にモンスターの配置を試みる
 * @param creature クリーチャーへの参照
 * @param r_idx 生成モンスター種族
 * @param y 生成位置y座標
 * @param x 生成位置x座標
 * @return 生成が可能ならTRUE、不可能ならFALSE
 */
static bool check_procection_rune(CreatureEntity &creature, MonraceId monrace_id, const Pos2D &pos)
{
    auto &grid = creature.get_floor()->get_grid(pos);
    if (!grid.is_rune_protection()) {
        return true;
    }

    auto &monrace = MonraceList::get_instance().get_monrace(monrace_id);
    if (randint1(BREAK_RUNE_PROTECTION) >= (monrace.level + 20)) {
        return false;
    }

    if (any_bits(grid.info, CAVE_MARK)) {
        msg_print(_("守りのルーンが壊れた！", "The rune of protection is broken!"));
    }

    reset_bits(grid.info, CAVE_MARK);
    reset_bits(grid.info, CAVE_OBJECT);
    grid.mimic = 0;
    note_spot(creature, pos);
    return true;
}

static void warn_unique_generation(CreatureEntity &creature, MonraceId r_idx)
{
    if (!creature.has_warning_flag() || !AngbandWorld::get_instance().character_dungeon) {
        return;
    }

    const auto &monrace = MonraceList::get_instance().get_monrace(r_idx);
    if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE)) {
        return;
    }

    std::string color;
    if (monrace.level > creature.get_level() + 30) {
        color = _("黒く", "black");
    } else if (monrace.level > creature.get_level() + 15) {
        color = _("紫色に", "purple");
    } else if (monrace.level > creature.get_level() + 5) {
        color = _("ルビー色に", "deep red");
    } else if (monrace.level > creature.get_level() - 5) {
        color = _("赤く", "red");
    } else if (monrace.level > creature.get_level() - 15) {
        color = _("ピンク色に", "pink");
    } else {
        color = _("白く", "white");
    }

    const auto &item = choose_warning_item(creature);
    if (item) {
        const auto item_name = describe_flavor(creature, *item, (OD_OMIT_PREFIX | OD_NAME_ONLY));
        msg_format(_("%sは%s光った。", "%s glows %s."), item_name.data(), color.data());
    } else {
        msg_format(_("%s光る物が頭に浮かんだ。", "A %s image forms in your mind."), color.data());
    }
}

/*!
 * @brief モンスターを一体生成する / Attempt to place a monster of the given race at the given location.
 * @param player プレイヤーへの参照
 * @param y 生成位置y座標
 * @param x 生成位置x座標
 * @param r_idx 生成モンスター種族
 * @param mode 生成オプション
 * @param summoner_m_idx モンスターの召喚による場合、召喚主のモンスターID
 * @return 生成に成功したらモンスターID、失敗したらtl::nullopt
 */
tl::optional<MONSTER_IDX> place_monster_one(CreatureEntity &player, POSITION y, POSITION x, MonraceId r_idx, BIT_FLAGS mode, tl::optional<MONSTER_IDX> summoner_m_idx)
{
    auto &floor = *player.get_floor();
    auto pos = Pos2D(y, x);
    auto *g_ptr = &floor.grid_array[y][x];
    auto &monrace = MonraceList::get_instance().get_monrace(r_idx);
    const auto &world = AngbandWorld::get_instance();
    if (world.is_wild_mode() || !floor.contains(pos, FloorBoundary::OUTER_WALL_EXCLUSIVE) || !MonraceList::is_valid(r_idx)) {
        return tl::nullopt;
    }

    if (none_bits(mode, PM_IGNORE_TERRAIN) && (g_ptr->has(TerrainCharacteristics::PATTERN) || !monster_can_enter(player, pos.y, pos.x, monrace, 0))) {
        return tl::nullopt;
    }

    if (!check_unique_placeable(floor, r_idx, mode) || !check_quest_placeable(floor, r_idx) || !check_procection_rune(const_cast<CreatureEntity &>(player), r_idx, pos)) {
        return tl::nullopt;
    }

    msg_format_wizard(player, CHEAT_MONSTER, _("%s(Lv%d)を生成しました。", "%s(Lv%d) was generated."), monrace.name.data(), monrace.level);
    if (monrace.kind_flags.has(MonsterKindType::UNIQUE) || monrace.population_flags.has(MonsterPopulationType::NAZGUL) || (monrace.level < 10)) {
        reset_bits(mode, PM_KAGE);
    }

    const auto m_idx = floor.pop_empty_index_monster();
    g_ptr->m_idx = m_idx;
    if (!g_ptr->has_monster()) {
        return tl::nullopt;
    }

    CreatureEntity *m_ptr;
    m_ptr = &floor.m_list[g_ptr->m_idx];
    m_ptr->wipe(); // モンスターを初期化（古いデータをクリア）

    // モンスターの能力値をランダムに初期化
    get_stats(*m_ptr);

    m_ptr->set_alliance_idx(monrace.alliance_idx);

    m_ptr->clear_temporary_flags();
    m_ptr->clear_constant_flags();
    m_ptr->set_floor(player.get_floor());

    if (monrace.misc_flags.has(MonsterMiscType::CHAMELEON)) {
        m_ptr->set_r_idx(r_idx);
        choose_chameleon_polymorph(player, g_ptr->m_idx, g_ptr->get_terrain_id(), summoner_m_idx);
        m_ptr->set_constant_flag(MonsterConstantFlagType::CHAMELEON);
    } else if (any_bits(mode, PM_CHAMELEON_FINAL_SUMMON)) {
        m_ptr->polymorph_to(r_idx);
        m_ptr->set_constant_flag(MonsterConstantFlagType::CHAMELEON);
    } else {
        m_ptr->set_r_idx(r_idx);
        if (any_bits(mode, PM_KAGE) && none_bits(mode, PM_FORCE_PET)) {
            m_ptr->set_ap_r_idx(MonraceId::KAGE);
            m_ptr->set_constant_flag(MonsterConstantFlagType::KAGE);
        } else {
            m_ptr->set_ap_r_idx(initial_r_appearance(const_cast<CreatureEntity &>(player), r_idx, mode));
        }
    }

    const auto &new_monrace = m_ptr->get_monrace();
    // 種族側で能力値補正が指定されていれば、ロール結果に加算する。
    // 補正値は内部 10 単位 (表示 1.0 = 10) で格納されており、tl::nullopt の能力値は補正しない。
    constexpr short stat_min = STAT_MIN_VALUE;
    constexpr short stat_max = STAT_MAX_VALUE;
    for (auto stat = 0; stat < A_MAX; ++stat) {
        const auto &mod = new_monrace.stat_modifiers[stat];
        if (!mod.has_value()) {
            continue;
        }
        auto adjusted = static_cast<int>(m_ptr->get_stat_max(stat)) + *mod;
        adjusted = std::clamp(adjusted, static_cast<int>(stat_min), static_cast<int>(stat_max));
        m_ptr->set_stat_max(stat, static_cast<short>(adjusted));
        m_ptr->set_stat_cur(stat, static_cast<short>(adjusted));
        if (m_ptr->get_stat_max_max(stat) < m_ptr->get_stat_max(stat)) {
            m_ptr->set_stat_max_max(stat, m_ptr->get_stat_max(stat));
        }
        m_ptr->set_stat_use(stat, m_ptr->get_stat_max(stat));
    }
    const auto is_summoned = summoner_m_idx.has_value();
    const CreatureEntity &summoner = floor.m_list[summoner_m_idx.value_or(0)];

    auto same_appearance_as_parent = !m_ptr->is_chameleon();
    same_appearance_as_parent &= any_bits(mode, PM_MULTIPLY);
    same_appearance_as_parent &= is_summoned && !summoner.is_original_ap();

    if (same_appearance_as_parent) {
        m_ptr->set_ap_r_idx(summoner.get_ap_r_idx());
        if (summoner.is_kage()) {
            m_ptr->set_constant_flag(MonsterConstantFlagType::KAGE);
        }
    }

    time_t now = time(nullptr);
    struct tm *t = localtime(&now);
    if (t->tm_mon == 11 && t->tm_mday >= 24 && t->tm_mday <= 25 && one_in_(6)) {
        if (none_bits(mode, PM_MULTIPLY | PM_KAGE) && m_ptr->get_r_idx() != MonraceId::SANTA) {
            m_ptr->set_constant_flag(MonsterConstantFlagType::SANTA);
        }
    }

    if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE) && one_in_(100)) {
        m_ptr->set_constant_flag(MonsterConstantFlagType::HUGE);
    } else if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE) && !m_ptr->is_huge() && one_in_(15)) {
        m_ptr->set_constant_flag(MonsterConstantFlagType::LARGE);
    }

    if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE) && !m_ptr->is_large() && !m_ptr->is_huge() && one_in_(18)) {
        m_ptr->set_constant_flag(MonsterConstantFlagType::SMALL);
    }

    if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE) &&
        monrace.kind_flags.has_not(MonsterKindType::NONLIVING) && one_in_(20)) {
        m_ptr->set_constant_flag(MonsterConstantFlagType::FAT);
    } else if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE) &&
               monrace.kind_flags.has_not(MonsterKindType::NONLIVING) && !m_ptr->is_fat() && one_in_(25)) {
        m_ptr->set_constant_flag(MonsterConstantFlagType::GAUNT);
    }

    if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE) &&
        monrace.kind_flags.has(MonsterKindType::HUMAN) && one_in_(80)) {
        m_ptr->set_constant_flag(MonsterConstantFlagType::NAKED);
    }

    if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE) &&
        monrace.kind_flags.has_not(MonsterKindType::NONLIVING) &&
        monrace.kind_flags.has_not(MonsterKindType::UNDEAD) && one_in_(50)) {
        m_ptr->set_constant_flag(MonsterConstantFlagType::ZOMBIFIED);
    }

    if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE) &&
        monrace.misc_flags.has(MonsterMiscType::NO_WAIFUZATION) && one_in_(20)) {
        m_ptr->set_constant_flag(MonsterConstantFlagType::WAIFUIZED);
    }

    // 違法改造フラグの付与
    if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE) &&
        (monrace.kind_flags.has(MonsterKindType::GOLEM) || monrace.kind_flags.has(MonsterKindType::ROBOT)) &&
        one_in_(40)) {
        m_ptr->set_constant_flag(MonsterConstantFlagType::ILLEGAL_MODIFIED);
    }

    // 軽量化フラグの付与
    if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE) &&
        monrace.kind_flags.has(MonsterKindType::GOLEM) &&
        one_in_(15)) {
        m_ptr->set_constant_flag(MonsterConstantFlagType::LIGHTWEIGHT);
    }

    if (!m_ptr->is_chameleon() && is_summoned && new_monrace.kind_flags.has_none_of(alignment_mask)) {
        m_ptr->set_sub_align(summoner.get_sub_align());
    } else if (m_ptr->is_chameleon() && new_monrace.kind_flags.has(MonsterKindType::UNIQUE) && !is_summoned) {
        m_ptr->set_sub_align(SUB_ALIGN_NEUTRAL);
    } else {
        m_ptr->set_sub_align(SUB_ALIGN_NEUTRAL);
        if (new_monrace.kind_flags.has(MonsterKindType::EVIL)) {
            m_ptr->add_sub_align(SUB_ALIGN_EVIL);
        }
        if (new_monrace.kind_flags.has(MonsterKindType::GOOD)) {
            m_ptr->add_sub_align(SUB_ALIGN_GOOD);
        }
    }

    m_ptr->y = y;
    m_ptr->x = x;
    m_ptr->set_floor(&floor);

    // 種族の MALE/FEMALE 指定からモンスターの性別を決定する。
    // データソースは 2 系統:
    //   1) MonraceDefinition::sex (MonsterSex enum)         例: 女王ベトベト
    //   2) MonraceDefinition::kind_flags の MALE/FEMALE     例: 一般人間モンスター
    // 両系統で OR 集約し、両方真→両性、片方→該当、なし→無性とする。
    {
        const auto has_male = new_monrace.kind_flags.has(MonsterKindType::MALE) || (new_monrace.sex == MonsterSex::MALE);
        const auto has_female = new_monrace.kind_flags.has(MonsterKindType::FEMALE) || (new_monrace.sex == MonsterSex::FEMALE);
        if (has_male && has_female) {
            m_ptr->psex = SEX_BISEXUAL;
        } else if (has_male) {
            m_ptr->psex = SEX_MALE;
        } else if (has_female) {
            m_ptr->psex = SEX_FEMALE;
        } else {
            m_ptr->psex = SEX_ASEXUAL;
        }
    }

    for (const auto mte : MONSTER_TIMED_EFFECT_LIST) {
        m_ptr->set_timed_effect(mte, 0);
    }

    m_ptr->reset_target();
    // UNIQUE モンスターはペットでなくとも creature.name に種族名を保持し、
    // ペットの個体名と同じ変数 (CreatureEntity::name) で名前を扱えるようにする。
    if (new_monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
        m_ptr->name = new_monrace.name.string();
    } else {
        m_ptr->name.clear();
    }
    m_ptr->set_exp(0);

    if (is_summoned) {
        m_ptr->set_parent_m_idx(*summoner_m_idx);
        if (summoner.get_monrace().kind_flags.has(MonsterKindType::QUYLTHLUG)) {
            m_ptr->set_constant_flag(MonsterConstantFlagType::QUYLTHLUG_BORN);
        }
    } else {
        m_ptr->set_parent_m_idx(0);
    }

    // 変身情報のコピー
    m_ptr->set_transform_r_idx(new_monrace.transform_r_idx);
    m_ptr->set_transform_hp_threshold(new_monrace.transform_hp_threshold);
    m_ptr->set_has_transformed(false);

    if (any_bits(mode, PM_CLONE)) {
        m_ptr->set_constant_flag(MonsterConstantFlagType::CLONED);
    }

    if (any_bits(mode, PM_NO_PET)) {
        m_ptr->set_constant_flag(MonsterConstantFlagType::NOPET);
    }

    // モンスターのフラグに基づいて対応するプレイヤー種族IDと職業IDを初期化
    m_ptr->initialize_equivalent_player_races();
    m_ptr->initialize_equivalent_player_classes();

    // 非プレイヤーにも性格・魔法領域を設定する
    // (性格はいかさま以外ランダム、領域は領域持ち職業のみ全領域から完全ランダム)
    m_ptr->assign_random_personality();
    m_ptr->assign_random_realm();

    // 種族が指定されている場合、身長・体重を設定
    get_height_weight(*m_ptr);

    // 所持金を初期化（能力値に基づいて計算）
    get_money_for_creature(*m_ptr);

    m_ptr->set_visible_on_map(false);
    if (any_bits(mode, PM_FORCE_PET)) {
        set_pet(player, *m_ptr);
    } else {
        auto should_be_friendly = !is_summoned && new_monrace.behavior_flags.has(MonsterBehaviorType::FRIENDLY);
        should_be_friendly |= is_summoned && summoner.is_friendly();
        should_be_friendly |= any_bits(mode, PM_FORCE_FRIENDLY);
        auto force_hostile = monster_has_hostile_to_player(player, 0, -1, new_monrace);
        force_hostile |= floor.inside_arena;
        if (m_ptr->get_alliance_idx() != AllianceType::NONE) {
            should_be_friendly |= alliance_list.at(m_ptr->get_alliance_idx())->isFriendly(player);
        }
        if (should_be_friendly && !force_hostile) {
            m_ptr->set_friendly();
        }
    }

    m_ptr->set_timed_effect(CreatureTimedEffect::SLEEP_OR_PARALYSIS, 0);
    if (any_bits(mode, PM_ALLOW_SLEEP) && new_monrace.sleep && !ironman_nightmare) {
        int val = new_monrace.sleep;
        (void)set_monster_csleep(floor, g_ptr->m_idx, (val * 2) + randint1(val * 10));
    }

    if (new_monrace.misc_flags.has(MonsterMiscType::FORCE_MAXHP)) {
        m_ptr->max_maxhp = new_monrace.hit_dice.maxroll();
    } else {
        m_ptr->max_maxhp = new_monrace.hit_dice.roll();
    }

    if (m_ptr->is_huge()) {
        m_ptr->max_maxhp *= (randint1(8) + 15) / 8;
        m_ptr->max_maxhp = std::min(MONSTER_MAXHP, m_ptr->max_maxhp);
    } else if (m_ptr->is_large()) {
        m_ptr->max_maxhp *= (randint1(5) + 10) / 8;
        m_ptr->max_maxhp = std::min(MONSTER_MAXHP, m_ptr->max_maxhp);
    }
    if (m_ptr->is_small()) {
        m_ptr->max_maxhp *= (randint1(2) + 4) / 8;
        m_ptr->max_maxhp = std::max(1, m_ptr->max_maxhp);
    }
    if (m_ptr->is_fat()) {
        m_ptr->max_maxhp *= (randint1(3) + 8) / 8;
        m_ptr->max_maxhp = std::min(MONSTER_MAXHP, m_ptr->max_maxhp);
    }
    if (m_ptr->is_gaunt()) {
        m_ptr->max_maxhp *= (randint1(3) + 4) / 8;
        m_ptr->max_maxhp = std::max(1, m_ptr->max_maxhp);
    }
    if (m_ptr->is_zombified()) {
        m_ptr->max_maxhp *= (randint1(3) + 5) / 8;
        m_ptr->max_maxhp = std::max(1, m_ptr->max_maxhp);
        // ゾンビ化したモンスターにUNDEADフラグを付与
        m_ptr->get_monrace().kind_flags.set(MonsterKindType::UNDEAD);
    }
    if (m_ptr->is_illegal_modified()) {
        m_ptr->max_maxhp *= (randint1(3) + 10) / 8; // 1.5倍～1.75倍のHPボーナス
        m_ptr->max_maxhp = std::min(MONSTER_MAXHP, m_ptr->max_maxhp);
    }
    if (m_ptr->is_lightweight()) {
        m_ptr->max_maxhp *= (randint1(2) + 5) / 8; // 0.625倍～0.75倍のHP減少
        m_ptr->max_maxhp = std::max(1, m_ptr->max_maxhp);
    }

    // Set MALE kind flag based on monster's sex
    if (m_ptr->is_male()) {
        m_ptr->get_monrace().kind_flags.set(MonsterKindType::MALE);
    }

    // Set FEMALE kind flag based on monster's sex
    if (m_ptr->is_female()) {
        m_ptr->get_monrace().kind_flags.set(MonsterKindType::FEMALE);
    }

    if (ironman_nightmare) {
        auto hp = m_ptr->max_maxhp * 2;
        m_ptr->max_maxhp = std::min(MONSTER_MAXHP, hp);
    }

    m_ptr->maxhp = m_ptr->max_maxhp;
    if (new_monrace.cur_hp_per != 0) {
        m_ptr->hp = m_ptr->maxhp * new_monrace.cur_hp_per / 100;
    } else {
        m_ptr->hp = m_ptr->maxhp;
    }

    m_ptr->set_dealt_damage(0);
    if (monrace.suicide_dice_num && monrace.suicide_dice_side) {
        m_ptr->set_death_count(Dice::roll(monrace.suicide_dice_num, monrace.suicide_dice_side));
    }

    // [Phase 2] body_structure 由来の拡張装備スロットを初期化
    m_ptr->init_extended_inventory();

    m_ptr->set_individual_speed(floor.inside_arena);

    // Initialize AC from monster race
    m_ptr->set_ac(new_monrace.ac);
    if (m_ptr->is_illegal_modified()) {
        m_ptr->set_ac(m_ptr->ac + randint1(10) + 5); // +6～+15のACボーナス
    }

    if (m_ptr->is_huge()) {
        m_ptr->speed -= randint1(2) + 2;
    }
    if (m_ptr->is_gaunt()) {
        m_ptr->speed -= randint1(3);
    }
    if (m_ptr->is_small()) {
        m_ptr->speed += randint1(2) + 1;
    }
    if (m_ptr->is_illegal_modified()) {
        m_ptr->speed += randint1(5) + 3; // +3～+7の加速ボーナス
    }
    if (m_ptr->is_lightweight()) {
        m_ptr->speed += randint1(3) + 2; // +2～+4の加速ボーナス
    }

    if (any_bits(mode, PM_HASTE)) {
        (void)set_monster_fast(floor, g_ptr->m_idx, 100);
    }

    if (!ironman_nightmare) {
        m_ptr->energy_need = ENERGY_NEED() - randnum0<short>(100);
    } else {
        m_ptr->energy_need = ENERGY_NEED() - randnum0<short>(100) * 2;
    }

    if (!ironman_nightmare) {
        m_ptr->set_temporary_flag(MonsterTemporaryFlagType::PREVENT_MAGIC);
    }

    auto is_awake_lightning_monster = new_monrace.brightness_flags.has_any_of(self_ld_mask);
    is_awake_lightning_monster |= new_monrace.brightness_flags.has_any_of(has_ld_mask) && !m_ptr->is_asleep();
    if (is_awake_lightning_monster) {
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::MONSTER_LITE);
    }

    update_monster(const_cast<CreatureEntity &>(player), g_ptr->m_idx, true);
    m_ptr->get_real_monrace().increment_current_numbers();

    // [ドロップ品移行] 一般ドロップ品を生成時に所持品として前生成する。
    // 死亡時は drop_all_inventory() でまとめて床へ放出される。
    generate_monster_drop_items(const_cast<CreatureEntity &>(player), *m_ptr);

    if (any_bits(mode, PM_AMBUSH)) {
        auto m_name = monster_desc(player, *m_ptr, 0);
        msg_format(_("突如%sがあなたに襲い掛かってきた！", "Suddenly %s has ambushed you!"), m_name.data());
        disturb(player, false, true);
        MonsterAttackPlayer(player, g_ptr->m_idx).make_attack_normal();
    }

    /*
     * Memorize location of the unique monster in saved floors.
     * A unique monster move from old saved floor.
     */
    if (world.character_dungeon && (new_monrace.kind_flags.has(MonsterKindType::UNIQUE) || new_monrace.population_flags.has(MonsterPopulationType::NAZGUL))) {
        m_ptr->get_real_monrace().floor_id = player.floor_id;
    }

    if (new_monrace.misc_flags.has(MonsterMiscType::MULTIPLY)) {
        floor.num_repro++;
    }

    warn_unique_generation(const_cast<CreatureEntity &>(player), r_idx);
    activate_explosive_rune(player, pos, new_monrace);
    return m_ptr->is_valid() ? tl::make_optional(g_ptr->m_idx) : tl::nullopt;
}
