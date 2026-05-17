/*!
 * @brief ウィザードモードの処理(特別処理中心) / Wizard commands
 * @date 2014/09/07
 * @author
 * Copyright (c) 1997 Ben Harrison, and others<br>
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.<br>
 * 2014 Deskull rearranged comment for Doxygen.<br>
 */

#include "wizard/wizard-special-process.h"
#include "artifact/fixed-art-generator.h"
#include "cmd-io/cmd-save.h"
#include "cmd-visual/cmd-draw.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "dungeon/quest.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-leaver.h"
#include "floor/floor-mode-changer.h"
#include "floor/floor-object.h"
#include "floor/object-allocator.h"
#include "game-option/birth-options.h"
#include "game-option/option-types-table.h"
#include "game-option/play-record-options.h"
#include "game-option/special-options.h"
#include "grid/grid.h"
#include "io/files-util.h"
#include "io/input-key-requester.h"
#include "io/write-diary.h"
#include "mind/mind-elementalist.h"
#include "monster-floor/monster-remover.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/item-magic-applier.h"
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/class-info.h"
#include "player-info/class-types.h"
#include "player-info/race-info.h"
#include "player-info/race-types.h"
#include "player-info/self-info.h"
#include "player-status/player-energy.h"
#include "player/digestion-processor.h"
#include "player/player-realm.h"
#include "player/player-skill.h"
#include "player/player-spell-status.h"
#include "player/player-status-table.h"
#include "player/player-status.h"
#include "player/race-info-table.h"
#include "spell-kind/spells-world.h"
#include "spell/spells-status.h"
#include "status/bad-status-setter.h"
#include "system/artifact-type-definition.h"
#include "system/creature-entity.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/floor/floor-info.h"
#include "system/floor/wilderness-grid.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "target/grid-selector.h"
#include "util/angband-files.h"
#include "util/candidate-selector.h"
#include "util/dice.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "wizard/spoiler-table.h"
#include "wizard/tval-descriptions-table.h"
#include "wizard/wizard-messages.h"
#include "world/world.h"
#include <algorithm>
#include <fstream>
#include <span>
#include <sstream>
#include <tl/optional.hpp>
#include <tuple>
#include <vector>

/*!
 * @brief プレイヤーを完全回復する
 */
void wiz_cure_all(CreatureEntity &creature)
{
    (void)life_stream(creature, false, false);
    (void)restore_mana(creature, true);
    (void)set_food(creature, PY_FOOD_MAX - 1);
    BadStatusSetter bss(creature);
    (void)bss.set_fear(0);
    (void)bss.set_deceleration(0, false);
    msg_print("You're fully cured by wizard command.");
}

static tl::optional<tval_desc> wiz_select_tval()
{
    CandidateSelector cs(_("アイテム種別を選んで下さい", "Get what type of object? "), 15);
    const auto choice = cs.select(tval_desc_list, [](const auto &tval) { return tval.desc; });

    return (choice != tval_desc_list.end()) ? tl::make_optional(*choice) : tl::nullopt;
}

static tl::optional<short> wiz_select_sval(const tval_desc &td)
{
    std::vector<short> bi_ids;
    for (const auto &baseitem : BaseitemList::get_instance()) {
        if (!baseitem.is_valid() || baseitem.bi_key.tval() != td.tval) {
            continue;
        }

        bi_ids.push_back(baseitem.idx);
    }

    const auto prompt = format(_("%s群の具体的なアイテムを選んで下さい", "What Kind of %s? "), td.desc);

    CandidateSelector cs(prompt, 15);
    const auto &baseitems = BaseitemList::get_instance();
    const auto choice = cs.select(bi_ids,
        [&baseitems](short bi_id) { return baseitems.get_baseitem(bi_id).stripped_name(); });
    return (choice != bi_ids.end()) ? tl::make_optional(*choice) : tl::nullopt;
}

/*!
 * @brief ベースアイテムのウィザード生成のために大項目IDと小項目IDを取得する /
 * Specify tval and sval (type and subtype of object) originally
 * @return ベースアイテムID
 * @details
 * by RAK, heavily modified by -Bernd-
 * This function returns the bi_id of an object type, or zero if failed
 * List up to 50 choices in three columns
 */
static tl::optional<short> wiz_create_itemtype()
{
    auto selection = wiz_select_tval();
    if (!selection) {
        return tl::nullopt;
    }

    return wiz_select_sval(*selection);
}

/*!
 * @brief 任意のベースアイテム生成のメインルーチン /
 * Wizard routine for creating objects		-RAK-
 * @details
 * Heavily modified to allow magification and artifactification  -Bernd-
 *
 * Note that wizards cannot create objects on top of other objects.
 *
 * Hack -- this routine always makes a "dungeon object", and applies
 * magic to it, and attempts to decline cursed items.
 */
void wiz_create_item(CreatureEntity &creature)
{
    screen_save();
    const auto bi_id = wiz_create_itemtype();
    screen_load();
    if (!bi_id) {
        return;
    }

    const auto &baseitem = BaseitemList::get_instance().get_baseitem(*bi_id);
    if (baseitem.gen_flags.has(ItemGenerationTraitType::INSTA_ART)) {
        for (const auto &[fa_id, artifact] : ArtifactList::get_instance()) {
            if (artifact.bi_key != baseitem.bi_key) {
                continue;
            }

            (void)create_named_art(creature, fa_id, creature.y, creature.x);
            msg_print("Allocated(INSTA_ART).");
            return;
        }
    }

    ItemEntity item;
    item.generate(*bi_id);
    ItemMagicApplier(creature, &item, creature.get_floor()->dun_level, AM_NO_FIXED_ART).execute();
    (void)drop_near(creature, item, creature.get_position());
    msg_print("Allocated.");
}

/*!
 * @brief 指定したIDの固定アーティファクトの名称を取得する
 *
 * @param fa_id 固定アーティファクトのID
 * @return 固定アーティファクトの名称(Ex. ★ロング・ソード『リンギル』)を保持する std::string オブジェクト
 */
static std::string wiz_make_named_artifact_desc(CreatureEntity &creature, FixedArtifactId fa_id)
{
    const auto &artifact = ArtifactList::get_instance().get_artifact(fa_id);
    ItemEntity item(artifact.bi_key);
    item.fa_id = fa_id;
    return describe_flavor(creature, item, OD_NAME_ONLY | OD_STORE);
}

/**
 * @brief 固定アーティファクトをリストから選択する
 *
 * @param fa_ids 選択する候補となる固定アーティファクトのIDのリスト
 * @return 選択した固定アーティファクトのIDを返す。但しキャンセルした場合は tl::nullopt を返す。
 */
static tl::optional<FixedArtifactId> wiz_select_named_artifact(CreatureEntity &creature, const std::vector<FixedArtifactId> &fa_ids)
{
    CandidateSelector cs("Which artifact: ", 15);

    auto describe_artifact = [&creature](FixedArtifactId fa_id) { return wiz_make_named_artifact_desc(creature, fa_id); };
    const auto it = cs.select(fa_ids, describe_artifact);
    return (it != fa_ids.end()) ? tl::make_optional(*it) : tl::nullopt;
}

/**
 * @brief 指定したカテゴリの固定アーティファクトのIDのリストを得る
 * @param group_artifact 固定アーティファクトのカテゴリ
 * @return 該当のカテゴリの固定アーティファクトのIDのリスト
 */
static std::vector<FixedArtifactId> wiz_collect_group_fa_ids(const grouper &group_artifact)
{
    const auto &[tvals, name] = group_artifact;
    std::vector<FixedArtifactId> fa_ids;
    for (const auto tval : tvals) {
        for (const auto &[fa_id, artifact] : ArtifactList::get_instance()) {
            if (artifact.bi_key.tval() == tval) {
                fa_ids.push_back(fa_id);
            }
        }
    }

    return fa_ids;
}

/*!
 * @brief 固定アーティファクトを生成する / Create the artifact
 */
void wiz_create_named_art(CreatureEntity &creature)
{
    screen_save();
    for (auto i = 0U; i < group_artifact_list.size(); ++i) {
        const auto &[tval_lit, name] = group_artifact_list[i];
        std::stringstream ss;
        ss << I2A(i) << ") " << name;
        term_erase(14, i + 1);
        put_str(ss.str(), i + 1, 15);
    }

    tl::optional<FixedArtifactId> created_fa_id;
    while (!created_fa_id) {
        const auto command = input_command("Kind of artifact: ");
        if (!command) {
            screen_load();
            return;
        }

        const auto idx = A2I(*command);
        if (idx >= group_artifact_list.size()) {
            continue;
        }

        auto fa_ids = wiz_collect_group_fa_ids(group_artifact_list[idx]);
        std::sort(fa_ids.begin(), fa_ids.end(), [](FixedArtifactId id1, FixedArtifactId id2) { return ArtifactList::get_instance().order(id1, id2); });
        created_fa_id = wiz_select_named_artifact(creature, fa_ids);
    }

    screen_load();
    const auto &artifact = ArtifactList::get_instance().get_artifact(*created_fa_id);
    if (artifact.is_generated) {
        msg_print("It's already allocated.");
        return;
    }

    (void)create_named_art(creature, *created_fa_id, creature.y, creature.x);
    msg_print("Allocated.");
}

static void wiz_change_status_max(CreatureEntity &creature)
{
    for (auto i = 0; i < A_MAX; ++i) {
        creature.set_stat_cur(i, creature.stat_max_max[i]);
        creature.set_stat_max(i, creature.stat_max_max[i]);
    }

    for (auto tval : TV_WEAPON_RANGE) {
        for (auto &exp : creature.weapon_exp[tval]) {
            exp = PlayerSkill::weapon_exp_at(PlayerSkillRank::MASTER);
        }
    }
    PlayerSkill(creature).limit_weapon_skills_by_max_value();

    for (auto &[type, exp] : creature.skill_exp) {
        exp = class_skills_info[enum2i(creature.pclass)].s_max[type];
    }

    const std::span spells_exp_span(creature.spell_exp);
    for (auto &exp : spells_exp_span.first(32)) {
        exp = PlayerSkill::spell_exp_at(PlayerSkillRank::MASTER);
    }
    for (auto &exp : spells_exp_span.last(32)) {
        exp = PlayerSkill::spell_exp_at(PlayerSkillRank::EXPERT);
    }

    creature.set_au(999999999);

    if (CreatureRace(&creature).equals(PlayerRaceType::ANDROID)) {
        return;
    }

    creature.set_max_exp(99999999);
    creature.set_exp(99999999);
    creature.exp_frac = 0;
}

/*!
 * @brief プレイヤーの現能力値を調整する / Change various "permanent" player variables.
 * @param creature クリーチャーへの参照
 */
void wiz_change_status(CreatureEntity &creature)
{
    const auto finalizer = util::make_finalizer([&creature]() {
        check_experience(creature);
        do_cmd_redraw(creature);
    });

    constexpr auto msg = _("全てのステータスを最大にしますか？", "Maximize all statuses? ");
    if (input_check_strict(creature, msg, { UserCheck::NO_ESCAPE, UserCheck::NO_HISTORY })) {
        wiz_change_status_max(creature);
        return;
    }

    for (int i = 0; i < A_MAX; i++) {
        const auto max_max_ability_score = creature.stat_max_max[i];
        const auto max_ability_score = creature.stat_max[i];
        const auto new_ability_score = input_numerics(stat_names[i], 3, max_max_ability_score, max_ability_score);
        if (!new_ability_score.has_value()) {
            return;
        }

        creature.set_stat_cur(i, *new_ability_score);
        creature.set_stat_max(i, *new_ability_score);
    }

    const auto unskilled = PlayerSkill::weapon_exp_at(PlayerSkillRank::UNSKILLED);
    const auto master = PlayerSkill::weapon_exp_at(PlayerSkillRank::MASTER);
    const auto proficiency = input_numerics(_("熟練度", "Proficiency"), unskilled, master, static_cast<short>(master));
    if (!proficiency) {
        return;
    }

    for (auto tval : TV_WEAPON_RANGE) {
        for (int i = 0; i < 64; i++) {
            creature.weapon_exp[tval][i] = *proficiency;
        }
    }

    PlayerSkill(creature).limit_weapon_skills_by_max_value();
    for (auto j : PLAYER_SKILL_KIND_TYPE_RANGE) {
        creature.skill_exp[j] = *proficiency;
        auto short_pclass = enum2i(creature.pclass);
        if (creature.get_skill_exp(j) > class_skills_info[short_pclass].s_max[j]) {
            creature.skill_exp[j] = class_skills_info[short_pclass].s_max[j];
        }
    }

    int k;
    for (k = 0; k < 32; k++) {
        creature.spell_exp[k] = std::min(PlayerSkill::spell_exp_at(PlayerSkillRank::MASTER), *proficiency);
    }

    for (; k < 64; k++) {
        creature.spell_exp[k] = std::min(PlayerSkill::spell_exp_at(PlayerSkillRank::EXPERT), *proficiency);
    }

    const auto gold = input_numerics("Gold: ", 0, MAX_INT, creature.get_au());
    if (!gold.has_value()) {
        return;
    }

    creature.set_au(*gold);
    if (CreatureRace(&creature).equals(PlayerRaceType::ANDROID)) {
        return;
    }

    const auto experience = input_numerics("Experience: ", 0, MAX_INT, creature.get_max_exp());
    if (!experience) {
        return;
    }

    creature.set_max_exp(*experience);
    creature.set_exp(*experience);
    creature.exp_frac = 0;
}

/*!
 * @brief 指定された地点の地形IDを変更する /
 * Create desired feature
 * @param creature クリーチャーへの参照
 */
void wiz_create_feature(CreatureEntity &creature)
{
    const auto pos = point_target(creature);
    if (!pos) {
        return;
    }

    auto &grid = creature.get_floor()->get_grid(*pos);
    const int max = TerrainList::get_instance().size() - 1;
    const auto f_val1 = input_numerics(_("実地形ID", "FeatureID"), 0, max, grid.feat);
    if (!f_val1.has_value()) {
        return;
    }

    const auto f_val2 = input_numerics(_("偽装地形ID", "FeatureID"), 0, max, *f_val1);
    if (!f_val2) {
        return;
    }

    set_terrain_id_to_grid(creature, *pos, *f_val1);
    grid.mimic = *f_val2;
    const auto &terrain = grid.get_terrain(TerrainKind::MIMIC);
    if (terrain.flags.has(TerrainCharacteristics::RUNE_PROTECTION) || terrain.flags.has(TerrainCharacteristics::RUNE_EXPLOSION)) {
        grid.info |= CAVE_OBJECT;
    } else if (terrain.flags.has(TerrainCharacteristics::MIRROR)) {
        grid.info |= CAVE_GLOW | CAVE_OBJECT;
    }

    note_spot(creature, *pos);
    lite_spot(creature, *pos);
    RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::FLOW);
}

/*!
 * @brief デバッグ帰還のダンジョンを選ぶ
 */
static tl::optional<DungeonId> select_debugging_dungeon()
{
    const auto &dungeons = DungeonList::get_instance();
    auto describer = [&](DungeonId id) { return dungeons.get_dungeon(id).name; };

    CandidateSelector cs("Jump to which dungeon: ", 15);
    const auto choice = cs.select(DUNGEON_IDS, describer);

    return (choice != DUNGEON_IDS.end()) ? tl::make_optional(*choice) : tl::nullopt;
}

/*
 * @brief 選択したダンジョンの任意レベルを選択する
 * @param dungeon_id ダンジョン番号
 * @return レベルを選択したらその値、キャンセルならnullopt
 */
static tl::optional<int> select_debugging_floor(const FloorType &floor, DungeonId dungeon_id)
{
    const auto &dungeon = DungeonList::get_instance().get_dungeon(dungeon_id);
    const auto max_depth = dungeon.maxdepth;
    const auto min_depth = dungeon.mindepth;
    const auto is_current_dungeon = floor.dungeon_id == dungeon_id;
    auto initial_depth = floor.dun_level;
    if (!is_current_dungeon) {
        initial_depth = min_depth;
    }

    return input_numerics("Jump to level", min_depth, max_depth, initial_depth);
}

/*!
 * @brief 任意のダンジョン及び階層に飛ぶtための選択処理
 * Go to any level
 */
void wiz_jump_to_dungeon(CreatureEntity &creature)
{
    const auto &floor = *creature.get_floor();
    const auto dungeon_id = select_debugging_dungeon();
    if (!dungeon_id) {
        return;
    }

    if (dungeon_id == DungeonId::WILDERNESS) {
        if (floor.is_underground() && input_check("Jump to the ground? ")) {
            jump_floor(creature, DungeonId::WILDERNESS, 0);
        }
        return;
    }

    const auto level = select_debugging_floor(floor, *dungeon_id);
    if (!level) {
        return;
    }

    msg_format("You jump to dungeon level %d.", *level);
    if (autosave_l) {
        do_cmd_save_game(creature, true);
    }

    jump_floor(creature, *dungeon_id, *level);
}

/*!
 * @brief 全ベースアイテムを鑑定済みにする
 * @param creature クリーチャーへの参照
 */
void wiz_learn_items_all(CreatureEntity &creature)
{
    for (const auto &baseitem : BaseitemList::get_instance()) {
        if (baseitem.is_valid() && baseitem.level <= command_arg) {
            ItemEntity item(baseitem.idx);
            object_aware(creature, item);
        }
    }
}

static void change_birth_flags()
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::HP,
        StatusRecalculatingFlag::MP,
        StatusRecalculatingFlag::SPELLS,
    };
    rfu.set_flag(SubWindowRedrawingFlag::PLAYER);
    rfu.set_flags(flags_srf);
    static constexpr auto flags_mwrf = {
        MainWindowRedrawingFlag::BASIC,
        MainWindowRedrawingFlag::HP,
        MainWindowRedrawingFlag::MP,
        MainWindowRedrawingFlag::ABILITY_SCORE,
    };
    rfu.set_flags(flags_mwrf);
}

static tl::optional<ElementRealmType> wiz_select_element_realm()
{
    constexpr EnumRange element_realms(ElementRealmType::FIRE, ElementRealmType::MAX);
    CandidateSelector cs("Which realm: ", 15);

    const auto chosen_realm = cs.select(element_realms, get_element_title);
    return (chosen_realm != element_realms.end()) ? tl::make_optional(*chosen_realm) : tl::nullopt;
}

static tl::optional<RealmType> wiz_select_realm(const RealmChoices &choices, const std::string &msg)
{
    if (choices.count() <= 1) {
        return choices.first().value_or(RealmType::NONE);
    }

    std::vector<RealmType> candidates;
    RealmChoices::get_flags(choices, std::back_inserter(candidates));
    auto describe_realm = [](auto realm) { return PlayerRealm::get_name(realm).string(); };

    CandidateSelector cs(msg, 15);
    const auto choice = cs.select(candidates, describe_realm);
    return (choice != candidates.end()) ? tl::make_optional(*choice) : tl::nullopt;
}

static tl::optional<std::tuple<RealmType, RealmType, ElementRealmType>> wiz_select_realms(PlayerClassType pclass)
{
    if (pclass == PlayerClassType::ELEMENTALIST) {
        const auto realm = wiz_select_element_realm();
        if (!realm) {
            return tl::nullopt;
        }

        return std::make_tuple(RealmType::NONE, RealmType::NONE, *realm);
    }

    const auto realm1 = wiz_select_realm(PlayerRealm::get_realm1_choices(pclass), "1st realm: ");
    if (!realm1) {
        return tl::nullopt;
    }

    auto realm2_choices = PlayerRealm::get_realm2_choices(pclass).reset(*realm1);
    if (pclass == PlayerClassType::PRIEST) {
        if (PlayerRealm::Realm(*realm1).is_good_attribute()) {
            realm2_choices.reset({ RealmType::DEATH, RealmType::DAEMON });
        } else {
            realm2_choices.reset({ RealmType::LIFE, RealmType::CRUSADE });
        }
    }

    const auto realm2 = wiz_select_realm(realm2_choices, "2nd realm: ");
    if (!realm2) {
        return tl::nullopt;
    }

    return std::make_tuple(*realm1, *realm2, ElementRealmType::NONE);
}

/*!
 * @brief プレイヤーの種族を変更する
 */
void wiz_reset_race(CreatureEntity &creature)
{
    CandidateSelector cs("Which race: ", 15);
    constexpr EnumRange races(PlayerRaceType::HUMAN, PlayerRaceType::MAX);
    auto describe_race = [](auto player_race) { return race_info[enum2i(player_race)].title.string(); };

    const auto chosen_race = cs.select(races, describe_race);
    if (chosen_race == races.end()) {
        return;
    }

    creature.prace = *chosen_race;
    creature.race = &race_info[enum2i(creature.prace)];
    change_birth_flags();
    handle_stuff(creature);
}

/*!
 * @brief プレイヤーの職業を変更する
 * @todo 魔法領域の再選択などがまだ不完全、要実装。
 */
void wiz_reset_class(CreatureEntity &creature)
{
    CandidateSelector cs("Which class: ", 15);
    constexpr EnumRange classes(PlayerClassType::WARRIOR, PlayerClassType::MAX);
    auto describe_class = [](auto player_class) { return class_info.at(player_class).title.string(); };

    const auto chosen_class = cs.select(classes, describe_class);
    if (chosen_class == classes.end()) {
        return;
    }

    const auto chosen_realms = wiz_select_realms(*chosen_class);
    if (!chosen_realms) {
        return;
    }

    creature.pclass = *chosen_class;
    cp_ptr = &class_info.at(creature.pclass);
    creature.pclass_ref = &class_info.at(creature.pclass);
    mp_ptr = &class_magics_info[enum2i(creature.pclass)];
    CreatureClass(creature).init_specific_data();
    PlayerRealm pr(creature);
    pr.reset();
    const auto &[realm1, realm2, element_realm] = *chosen_realms;
    if (realm1 != RealmType::NONE) {
        pr.set(realm1, realm2);
    }
    creature.element_realm = element_realm;
    PlayerSpellStatus pss(creature);
    pss.realm1().initialize();
    pss.realm2().initialize();
    creature.learned_spells = 0;
    change_birth_flags();
    handle_stuff(creature);
}

/*!
 * @brief プレイヤーの領域を変更する
 * @todo 存在有無などは未判定。そのうちすべき。
 */
void wiz_reset_realms(CreatureEntity &creature)
{
    const auto chosen_realms = wiz_select_realms(creature.pclass);
    if (!chosen_realms) {
        return;
    }

    PlayerRealm pr(creature);
    pr.reset();
    const auto &[realm1, realm2, element_realm] = *chosen_realms;
    if (realm1 != RealmType::NONE) {
        pr.set(realm1, realm2);
    }
    creature.element_realm = element_realm;
    PlayerSpellStatus pss(creature);
    pss.realm1().initialize();
    pss.realm2().initialize();
    creature.learned_spells = 0;
    change_birth_flags();
    handle_stuff(creature);
}

/*!
 * @brief 現在のオプション設定をダンプ出力する
 */
void wiz_dump_options()
{
    const auto path = path_build(ANGBAND_DIR_USER, "opt_info.txt");
    const auto &filename = path.string();
    std::ofstream ofs(path);
    if (ofs.bad()) {
        msg_format(_("ファイル %s を開けませんでした。", "Failed to open file %s."), filename.data());
        msg_erase();
        return;
    }

    constexpr auto num_o_set = 8;
    constexpr auto num_o_bit = 32;

    std::vector<std::vector<int>> exist(num_o_set, std::vector<int>(num_o_bit));
    auto option_count = 0;
    for (const auto &option : option_info) {
        exist[option.flag_position][option.offset] = option_count + 1;
        option_count++;
    }

    ofs << "[Option bits usage on %s\n]", AngbandSystem::get_instance().build_version_expression(VersionExpression::FULL);
    ofs << "Set - Bit (Page) Option Name\n";
    ofs << "------------------------------------------------\n";
    for (auto i = 0; i < num_o_set; i++) {
        for (auto j = 0; j < num_o_bit; j++) {
            if (exist[i][j]) {
                const auto &option = option_info[exist[i][j] - 1];
                const auto page = option.page ? enum2i(*option.page) : 255; //!< @details かつてnulloptではなかった頃の値.
                ofs << format("  %d -  %02d (%4d) %s\n", i, j, page, option.text.data());
            } else {
                ofs << format("  %d -  %02d\n", i, j);
            }
        }

        ofs << '\n';
    }

    msg_format(_("オプションbit使用状況をファイル %s に書き出しました。", "Option bits usage dump saved to file %s."), filename.data());
}

/*!
 * @brief プレイヤー近辺の全モンスターを消去する / Delete all nearby monsters
 */
void wiz_zap_surrounding_monsters(CreatureEntity &creature)
{
    const auto &floor = *creature.get_floor();
    // [提案 14b]
    const auto p_pos = creature.get_position();
    const auto targets = creature.collect_creatures([&](const CreatureEntity &mon) {
        return Grid::calc_distance(p_pos, mon.get_position()) <= MAX_PLAYER_SIGHT;
    });
    const auto riding_idx = creature.get_riding();
    for (auto i : targets) {
        if (i == riding_idx) {
            continue;
        }
        const auto &monster = floor.get_monster(i);
        if (record_named_pet && monster.is_named_pet()) {
            const auto m_name = monster_desc(creature, monster, MD_INDEF_VISIBLE);
            exe_write_diary(floor, DiaryKind::NAMED_PET, RECORD_NAMED_PET_WIZ_ZAP, m_name);
        }

        delete_monster_idx(creature, i);
    }
}

/*!
 * @brief フロアに存在する全モンスターを消去する / Delete all monsters
 * @param creature クリーチャーへの参照
 */
void wiz_zap_floor_monsters(CreatureEntity &creature)
{
    const auto &floor = *creature.get_floor();
    // [提案 14b]
    const auto targets = creature.collect_creatures([](const CreatureEntity &mon) {
        return !mon.is_riding();
    });
    for (auto i : targets) {
        const auto &monster = floor.get_monster(i);
        if (record_named_pet && monster.is_named_pet()) {
            const auto m_name = monster_desc(creature, monster, MD_INDEF_VISIBLE);
            exe_write_diary(floor, DiaryKind::NAMED_PET, RECORD_NAMED_PET_WIZ_ZAP, m_name);
        }

        delete_monster_idx(creature, i);
    }
}

/* @brief 死を欺く仕様(馬鹿馬鹿蛮怒独自実装) */
void cheat_death(CreatureEntity &creature, bool no_penalty)
{
    if (!no_penalty) {

        switch (randint0(4)) {

        case 0: {
            auto blank_years = Dice::roll(8, 10);
            creature.divide_prestige(2);
            creature.add_age(static_cast<int16_t>(blank_years));

            creature.set_max_max_exp((creature.get_max_max_exp() * 6 / (randint1(3) + 6)));
            creature.set_max_exp(creature.get_max_max_exp());
            creature.set_exp(creature.get_max_max_exp());
            creature.divide_au(2);

            msg_print(_("『ぬわああああん、疲れたなもおおおおん！』", "\"Aaaaaah! I'm hellish tireeeed!\""));
            msg_format(_("あなたは死んだ罰として＠人墓場でイェンダーの魔法使い共に%d年間奴隷労働を強いられた！",
                           "As a death penalry, you were forced to work like a slave by the wizards of Yendor at the '@' Stigmatic Graveyard for %d years!"),
                blank_years);
            break;
        }

        case 1: {
            auto blank_years = Dice::roll(2, 10);
            creature.divide_prestige(2);
            creature.add_age(static_cast<int16_t>(blank_years));
            msg_print(_("『猿先生何も考えてないと思うよ』", "\"I think that Mr.Sawatari thinks nothing.\""));
            msg_format(_("あなたは連載%d年の間猿空間に迷い込んでいた！ついでに死んだ設定も忘れ去られていた！",
                           "You have been lost in the *S*A*R*U* space for %d years! By the way, the dead setting was also forgotten!"),
                blank_years);
            break;
        }

        case 2:
            msg_print(_("『ああ＾～生き返るわぁ＾～』", "\"Ahh~ I'm reviving~.\""));
            msg_print(_("あなたは偉大なるカッチャマの神なる御言葉で無事蘇生した！", "You revived by a holt word from great mom!"));
            break;

        case 3:
            msg_format(_("王大人『%s 死亡確認』", "\"Lord Wang confirmed that %s is dead.\""), creature.name.data());
            break;

        default:
            break;
        }
    }

    auto &world = AngbandWorld::get_instance();
    world.noscore |= 0x0001;
    msg_erase();

    creature.is_dead_ = false;
    (void)life_stream(creature, false, false);
    (void)restore_mana(creature, true);
    (void)recall_player(creature, 0);
    reserve_alter_reality(creature, 0);

    creature.died_from = _("死の欺き", "Cheating death");
    (void)set_food(creature, PY_FOOD_MAX - 1);

    auto &floor = *creature.get_floor();
    floor.dun_level = 0;
    floor.inside_arena = false;
    AngbandSystem::get_instance().set_phase_out(false);
    leaving_quest = QuestId::NONE;
    floor.quest_number = QuestId::NONE;
    if (floor.is_underground()) {
        creature.recall_dungeon = floor.dungeon_id;
    }

    floor.reset_dungeon_index();
    auto &wilderness = WildernessGrids::get_instance();
    wilderness.initialize_position();
    if (vanilla_town) {
        creature.oldpy = 10;
        creature.oldpx = 34;
    } else {
        creature.oldpy = 33;
        creature.oldpx = 131;
    }

    world.set_wild_mode(false);
    creature.leaving = true;
    constexpr auto note = _("                            しかし、生き返った。", "                            but revived.");
    exe_write_diary(floor, DiaryKind::DESCRIPTION, 1, note);
    leave_floor(creature);
}
