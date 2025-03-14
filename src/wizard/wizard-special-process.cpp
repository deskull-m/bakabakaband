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
#include "artifact/fixed-art-types.h"
#include "birth/inventory-initializer.h"
#include "cmd-io/cmd-dump.h"
#include "cmd-io/cmd-help.h"
#include "cmd-io/cmd-save.h"
#include "cmd-visual/cmd-draw.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "dungeon/quest.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-leaver.h"
#include "floor/floor-mode-changer.h"
#include "floor/floor-object.h"
#include "game-option/birth-options.h"
#include "game-option/option-types-table.h"
#include "game-option/play-record-options.h"
#include "game-option/special-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "info-reader/fixed-map-parser.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "io/files-util.h"
#include "io/input-key-requester.h"
#include "io/write-diary.h"
#include "market/arena.h"
#include "monster-floor/monster-remover.h"
#include "monster-floor/monster-summon.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/smart-learn-types.h"
#include "mutation/mutation-investor-remover.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/item-magic-applier.h"
#include "object-enchant/trc-types.h"
#include "object-enchant/trg-types.h"
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "player-info/race-types.h"
#include "player-info/self-info.h"
#include "player-status/player-energy.h"
#include "player/digestion-processor.h"
#include "player/patron.h"
#include "player/player-realm.h"
#include "player/player-skill.h"
#include "player/player-spell-status.h"
#include "player/player-status-table.h"
#include "player/player-status.h"
#include "player/race-info-table.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell/spells-object.h"
#include "spell/spells-status.h"
#include "spell/spells-summon.h"
#include "status/bad-status-setter.h"
#include "status/experience.h"
#include "system/angband-system.h"
#include "system/angband-version.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "target/grid-selector.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "util/angband-files.h"
#include "util/bit-flags-calculator.h"
#include "util/candidate-selector.h"
#include "util/enum-converter.h"
#include "util/enum-range.h"
#include "util/finalizer.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "wizard/spoiler-table.h"
#include "wizard/tval-descriptions-table.h"
#include "wizard/wizard-messages.h"
#include "wizard/wizard-spells.h"
#include "wizard/wizard-spoiler.h"
#include "world/world.h"
#include <algorithm>
#include <fstream>
#include <optional>
#include <span>
#include <sstream>
#include <tuple>
#include <vector>

/*!
 * @brief プレイヤーを完全回復する
 */
void wiz_cure_all(PlayerType *player_ptr)
{
    (void)life_stream(player_ptr, false, false);
    (void)restore_mana(player_ptr, true);
    (void)set_food(player_ptr, PY_FOOD_MAX - 1);
    BadStatusSetter bss(player_ptr);
    (void)bss.set_fear(0);
    (void)bss.set_deceleration(0, false);
    msg_print("You're fully cured by wizard command.");
}

static std::optional<tval_desc> wiz_select_tval()
{
    CandidateSelector cs(_("アイテム種別を選んで下さい", "Get what type of object? "), 15);
    const auto choice = cs.select(tval_desc_list, [](const auto &tval) { return tval.desc; });

    return (choice != tval_desc_list.end()) ? std::make_optional(*choice) : std::nullopt;
}

static std::optional<short> wiz_select_sval(const tval_desc &td)
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
    return (choice != bi_ids.end()) ? std::make_optional(*choice) : std::nullopt;
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
static std::optional<short> wiz_create_itemtype()
{
    auto selection = wiz_select_tval();
    if (!selection) {
        return std::nullopt;
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
void wiz_create_item(PlayerType *player_ptr)
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

            (void)create_named_art(player_ptr, fa_id, player_ptr->y, player_ptr->x);
            msg_print("Allocated(INSTA_ART).");
            return;
        }
    }

    ItemEntity item;
    item.generate(*bi_id);
    ItemMagicApplier(player_ptr, &item, player_ptr->current_floor_ptr->dun_level, AM_NO_FIXED_ART).execute();
    (void)drop_near(player_ptr, &item, -1, player_ptr->y, player_ptr->x);
    msg_print("Allocated.");
}

/*!
 * @brief 指定したIDの固定アーティファクトの名称を取得する
 *
 * @param fa_id 固定アーティファクトのID
 * @return 固定アーティファクトの名称(Ex. ★ロング・ソード『リンギル』)を保持する std::string オブジェクト
 */
static std::string wiz_make_named_artifact_desc(PlayerType *player_ptr, FixedArtifactId fa_id)
{
    const auto &artifact = ArtifactList::get_instance().get_artifact(fa_id);
    ItemEntity item(artifact.bi_key);
    item.fa_id = fa_id;
    item.mark_as_known();
    return describe_flavor(player_ptr, item, OD_NAME_ONLY);
}

/**
 * @brief 固定アーティファクトをリストから選択する
 *
 * @param fa_ids 選択する候補となる固定アーティファクトのIDのリスト
 * @return 選択した固定アーティファクトのIDを返す。但しキャンセルした場合は std::nullopt を返す。
 */
static std::optional<FixedArtifactId> wiz_select_named_artifact(PlayerType *player_ptr, const std::vector<FixedArtifactId> &fa_ids)
{
    CandidateSelector cs("Which artifact: ", 15);

    auto describe_artifact = [player_ptr](FixedArtifactId fa_id) { return wiz_make_named_artifact_desc(player_ptr, fa_id); };
    const auto it = cs.select(fa_ids, describe_artifact);
    return (it != fa_ids.end()) ? std::make_optional(*it) : std::nullopt;
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
void wiz_create_named_art(PlayerType *player_ptr)
{
    screen_save();
    for (auto i = 0U; i < group_artifact_list.size(); ++i) {
        const auto &[tval_lit, name] = group_artifact_list[i];
        std::stringstream ss;
        ss << I2A(i) << ") " << name;
        term_erase(14, i + 1);
        put_str(ss.str(), i + 1, 15);
    }

    std::optional<FixedArtifactId> created_fa_id;
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
        created_fa_id = wiz_select_named_artifact(player_ptr, fa_ids);
    }

    screen_load();
    const auto &artifact = ArtifactList::get_instance().get_artifact(*created_fa_id);
    if (artifact.is_generated) {
        msg_print("It's already allocated.");
        return;
    }

    (void)create_named_art(player_ptr, *created_fa_id, player_ptr->y, player_ptr->x);
    msg_print("Allocated.");
}

static void wiz_change_status_max(PlayerType *player_ptr)
{
    for (auto i = 0; i < A_MAX; ++i) {
        player_ptr->stat_cur[i] = player_ptr->stat_max_max[i];
        player_ptr->stat_max[i] = player_ptr->stat_max_max[i];
    }

    for (auto tval : TV_WEAPON_RANGE) {
        for (auto &exp : player_ptr->weapon_exp[tval]) {
            exp = PlayerSkill::weapon_exp_at(PlayerSkillRank::MASTER);
        }
    }
    PlayerSkill(player_ptr).limit_weapon_skills_by_max_value();

    for (auto &[type, exp] : player_ptr->skill_exp) {
        exp = class_skills_info[enum2i(player_ptr->pclass)].s_max[type];
    }

    const std::span spells_exp_span(player_ptr->spell_exp);
    for (auto &exp : spells_exp_span.first(32)) {
        exp = PlayerSkill::spell_exp_at(PlayerSkillRank::MASTER);
    }
    for (auto &exp : spells_exp_span.last(32)) {
        exp = PlayerSkill::spell_exp_at(PlayerSkillRank::EXPERT);
    }

    player_ptr->au = 999999999;

    if (PlayerRace(player_ptr).equals(PlayerRaceType::ANDROID)) {
        return;
    }

    player_ptr->max_exp = 99999999;
    player_ptr->exp = 99999999;
    player_ptr->exp_frac = 0;
}

/*!
 * @brief プレイヤーの現能力値を調整する / Change various "permanent" player variables.
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void wiz_change_status(PlayerType *player_ptr)
{
    const auto finalizer = util::make_finalizer([player_ptr]() {
        check_experience(player_ptr);
        do_cmd_redraw(player_ptr);
    });

    constexpr auto msg = _("全てのステータスを最大にしますか？", "Maximize all statuses? ");
    if (input_check_strict(player_ptr, msg, { UserCheck::NO_ESCAPE, UserCheck::NO_HISTORY })) {
        wiz_change_status_max(player_ptr);
        return;
    }

    for (int i = 0; i < A_MAX; i++) {
        const auto max_max_ability_score = player_ptr->stat_max_max[i];
        const auto max_ability_score = player_ptr->stat_max[i];
        const auto new_ability_score = input_numerics(stat_names[i], 3, max_max_ability_score, max_ability_score);
        if (!new_ability_score.has_value()) {
            return;
        }

        player_ptr->stat_cur[i] = *new_ability_score;
        player_ptr->stat_max[i] = *new_ability_score;
    }

    const auto unskilled = PlayerSkill::weapon_exp_at(PlayerSkillRank::UNSKILLED);
    const auto master = PlayerSkill::weapon_exp_at(PlayerSkillRank::MASTER);
    const auto proficiency = input_numerics(_("熟練度", "Proficiency"), unskilled, master, static_cast<short>(master));
    if (!proficiency) {
        return;
    }

    for (auto tval : TV_WEAPON_RANGE) {
        for (int i = 0; i < 64; i++) {
            player_ptr->weapon_exp[tval][i] = *proficiency;
        }
    }

    PlayerSkill(player_ptr).limit_weapon_skills_by_max_value();
    for (auto j : PLAYER_SKILL_KIND_TYPE_RANGE) {
        player_ptr->skill_exp[j] = *proficiency;
        auto short_pclass = enum2i(player_ptr->pclass);
        if (player_ptr->skill_exp[j] > class_skills_info[short_pclass].s_max[j]) {
            player_ptr->skill_exp[j] = class_skills_info[short_pclass].s_max[j];
        }
    }

    int k;
    for (k = 0; k < 32; k++) {
        player_ptr->spell_exp[k] = std::min(PlayerSkill::spell_exp_at(PlayerSkillRank::MASTER), *proficiency);
    }

    for (; k < 64; k++) {
        player_ptr->spell_exp[k] = std::min(PlayerSkill::spell_exp_at(PlayerSkillRank::EXPERT), *proficiency);
    }

    const auto gold = input_numerics("Gold: ", 0, MAX_INT, player_ptr->au);
    if (!gold.has_value()) {
        return;
    }

    player_ptr->au = *gold;
    if (PlayerRace(player_ptr).equals(PlayerRaceType::ANDROID)) {
        return;
    }

    const auto experience = input_numerics("Experience: ", 0, MAX_INT, player_ptr->max_exp);
    if (!experience) {
        return;
    }

    player_ptr->max_exp = *experience;
    player_ptr->exp = *experience;
    player_ptr->exp_frac = 0;
}

/*!
 * @brief 指定された地点の地形IDを変更する /
 * Create desired feature
 * @param creaturer_ptr プレイヤーへの参照ポインタ
 */
void wiz_create_feature(PlayerType *player_ptr)
{
    POSITION y, x;
    if (!tgt_pt(player_ptr, &x, &y)) {
        return;
    }

    const Pos2D pos(y, x);
    auto &grid = player_ptr->current_floor_ptr->get_grid(pos);
    const int max = TerrainList::get_instance().size() - 1;
    const auto f_val1 = input_numerics(_("実地形ID", "FeatureID"), 0, max, grid.feat);
    if (!f_val1.has_value()) {
        return;
    }

    const auto f_val2 = input_numerics(_("偽装地形ID", "FeatureID"), 0, max, *f_val1);
    if (!f_val2) {
        return;
    }

    cave_set_feat(player_ptr, y, x, *f_val1);
    grid.mimic = *f_val2;
    const auto &terrain = grid.get_terrain_mimic();
    if (terrain.flags.has(TerrainCharacteristics::RUNE_PROTECTION) || terrain.flags.has(TerrainCharacteristics::RUNE_EXPLOSION)) {
        grid.info |= CAVE_OBJECT;
    } else if (terrain.flags.has(TerrainCharacteristics::MIRROR)) {
        grid.info |= CAVE_GLOW | CAVE_OBJECT;
    }

    note_spot(player_ptr, y, x);
    lite_spot(player_ptr, y, x);
    RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::FLOW);
}

/*!
 * @brief デバッグ帰還のダンジョンを選ぶ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details 範囲外の値が選択されたら再入力を促す
 */
static std::optional<int> select_debugging_dungeon(int initial_dungeon_id)
{
    if (command_arg > 0) {
        return std::clamp(static_cast<int>(command_arg), DUNGEON_ANGBAND, DUNGEON_MAX);
    }

    return input_numerics("Jump which dungeon", DUNGEON_ANGBAND, DUNGEON_MAX, initial_dungeon_id);
}

/*
 * @brief 選択したダンジョンの任意レベルを選択する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dungeon_id ダンジョン番号
 * @return レベルを選択したらその値、キャンセルならnullopt
 */
static std::optional<int> select_debugging_floor(const FloorType &floor, int dungeon_id)
{
    const auto &dungeon = dungeons_info[dungeon_id];
    const auto max_depth = dungeon.maxdepth;
    const auto min_depth = dungeon.mindepth;
    const auto is_current_dungeon = floor.dungeon_idx == dungeon_id;
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
void wiz_jump_to_dungeon(PlayerType *player_ptr)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto is_in_dungeon = floor.is_in_underground();
    const auto dungeon_idx = is_in_dungeon ? floor.dungeon_idx : DUNGEON_ANGBAND;
    const auto dungeon_id = select_debugging_dungeon(dungeon_idx);
    if (!dungeon_id) {
        if (!is_in_dungeon) {
            return;
        }

        if (input_check(("Jump to the ground?"))) {
            jump_floor(player_ptr, 0, 0);
        }

        return;
    }

    const auto level = select_debugging_floor(floor, *dungeon_id);
    if (!level) {
        return;
    }

    msg_format("You jump to dungeon level %d.", *level);
    if (autosave_l) {
        do_cmd_save_game(player_ptr, true);
    }

    jump_floor(player_ptr, *dungeon_id, *level);
}

/*!
 * @brief 全ベースアイテムを鑑定済みにする
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void wiz_learn_items_all(PlayerType *player_ptr)
{
    for (const auto &baseitem : BaseitemList::get_instance()) {
        if (baseitem.is_valid() && baseitem.level <= command_arg) {
            ItemEntity item(baseitem.idx);
            object_aware(player_ptr, item);
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

static std::optional<RealmType> wiz_select_realm(const RealmChoices &choices, const std::string &msg)
{
    if (choices.count() <= 1) {
        return choices.first().value_or(RealmType::NONE);
    }

    std::vector<RealmType> candidates;
    RealmChoices::get_flags(choices, std::back_inserter(candidates));
    auto describe_realm = [](auto realm) { return PlayerRealm::get_name(realm).string(); };

    CandidateSelector cs(msg, 15);
    const auto choice = cs.select(candidates, describe_realm);
    return (choice != candidates.end()) ? std::make_optional(*choice) : std::nullopt;
}

static std::optional<std::pair<RealmType, RealmType>> wiz_select_realms(PlayerClassType pclass)
{
    const auto realm1 = wiz_select_realm(PlayerRealm::get_realm1_choices(pclass), "1st realm: ");
    if (!realm1) {
        return std::nullopt;
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
        return std::nullopt;
    }

    return std::make_pair(*realm1, *realm2);
}

/*!
 * @brief プレイヤーの種族を変更する
 */
void wiz_reset_race(PlayerType *player_ptr)
{
    CandidateSelector cs("Which race: ", 15);
    constexpr EnumRange races(PlayerRaceType::HUMAN, PlayerRaceType::MAX);
    auto describe_race = [](auto player_race) { return race_info[enum2i(player_race)].title.string(); };

    const auto chosen_race = cs.select(races, describe_race);
    if (chosen_race == races.end()) {
        return;
    }

    player_ptr->prace = *chosen_race;
    rp_ptr = &race_info[enum2i(player_ptr->prace)];
    change_birth_flags();
    handle_stuff(player_ptr);
}

/*!
 * @brief プレイヤーの職業を変更する
 * @todo 魔法領域の再選択などがまだ不完全、要実装。
 */
void wiz_reset_class(PlayerType *player_ptr)
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

    player_ptr->pclass = *chosen_class;
    cp_ptr = &class_info.at(player_ptr->pclass);
    mp_ptr = &class_magics_info[enum2i(player_ptr->pclass)];
    PlayerClass(player_ptr).init_specific_data();
    PlayerRealm pr(player_ptr);
    pr.reset();
    if (chosen_realms->first != RealmType::NONE) {
        pr.set(chosen_realms->first, chosen_realms->second);
    }
    PlayerSpellStatus pss(player_ptr);
    pss.realm1().initialize();
    pss.realm2().initialize();
    player_ptr->learned_spells = 0;
    change_birth_flags();
    handle_stuff(player_ptr);
}

/*!
 * @brief プレイヤーの領域を変更する
 * @todo 存在有無などは未判定。そのうちすべき。
 */
void wiz_reset_realms(PlayerType *player_ptr)
{
    const auto chosen_realms = wiz_select_realms(player_ptr->pclass);
    if (!chosen_realms) {
        return;
    }

    PlayerRealm pr(player_ptr);
    pr.reset();
    if (chosen_realms->first != RealmType::NONE) {
        pr.set(chosen_realms->first, chosen_realms->second);
    }
    PlayerSpellStatus pss(player_ptr);
    pss.realm1().initialize();
    pss.realm2().initialize();
    player_ptr->learned_spells = 0;
    change_birth_flags();
    handle_stuff(player_ptr);
}

/*!
 * @brief 現在のオプション設定をダンプ出力する
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void wiz_dump_options()
{
    const auto path = path_build(ANGBAND_DIR_USER, "opt_info.txt");
    const auto &filename = path.string();
    std::ofstream ofs(path);
    if (ofs.bad()) {
        msg_format(_("ファイル %s を開けませんでした。", "Failed to open file %s."), filename.data());
        msg_print(nullptr);
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
void wiz_zap_surrounding_monsters(PlayerType *player_ptr)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    for (MONSTER_IDX i = 1; i < floor.m_max; i++) {
        const auto &monster = floor.m_list[i];
        if (!monster.is_valid() || (i == player_ptr->riding) || (monster.cdis > MAX_PLAYER_SIGHT)) {
            continue;
        }

        if (record_named_pet && monster.is_named_pet()) {
            const auto m_name = monster_desc(player_ptr, &monster, MD_INDEF_VISIBLE);
            exe_write_diary(floor, DiaryKind::NAMED_PET, RECORD_NAMED_PET_WIZ_ZAP, m_name);
        }

        delete_monster_idx(player_ptr, i);
    }
}

/*!
 * @brief フロアに存在する全モンスターを消去する / Delete all monsters
 * @param player_ptr 術者の参照ポインタ
 */
void wiz_zap_floor_monsters(PlayerType *player_ptr)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    for (MONSTER_IDX i = 1; i < floor.m_max; i++) {
        const auto &monster = floor.m_list[i];
        if (!monster.is_valid() || monster.is_riding()) {
            continue;
        }

        if (record_named_pet && monster.is_named_pet()) {
            const auto m_name = monster_desc(player_ptr, &monster, MD_INDEF_VISIBLE);
            exe_write_diary(floor, DiaryKind::NAMED_PET, RECORD_NAMED_PET_WIZ_ZAP, m_name);
        }

        delete_monster_idx(player_ptr, i);
    }
}

/* @brief 死を欺く仕様(馬鹿馬鹿蛮怒独自実装) */
void cheat_death(PlayerType *player_ptr, bool no_penalty)
{
    if (!no_penalty) {

        switch (randint0(4)) {

        case 0: {
            auto blank_years = Dice::roll(8, 10);
            player_ptr->prestige /= 2;
            player_ptr->age += static_cast<int16_t>(blank_years);

            player_ptr->max_max_exp = (player_ptr->max_max_exp * 6 / (randint1(3) + 6));
            player_ptr->max_exp = player_ptr->max_max_exp;
            player_ptr->exp = player_ptr->max_max_exp;
            player_ptr->au /= 2;

            msg_print(_("『ぬわああああん、疲れたなもおおおおん！』", "\"Aaaaaah! I'm hellish tireeeed!\""));
            msg_format(_("あなたは死んだ罰として＠人墓場でイェンダーの魔法使い共に%d年間奴隷労働を強いられた！",
                           "As a death penalry, you were forced to work like a slave by the wizards of Yendor at the '@' Stigmatic Graveyard for %d years!"),
                blank_years);
            break;
        }

        case 1: {
            auto blank_years = Dice::roll(2, 10);
            player_ptr->prestige /= 2;
            player_ptr->age += static_cast<int16_t>(blank_years);
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
            msg_format(_("王大人『%s 死亡確認』", "\"Lord Wang confirmed that %s is dead.\""), player_ptr->name);
            break;

        default:
            break;
        }
    }

    auto &world = AngbandWorld::get_instance();
    AngbandWorld::get_instance().noscore |= 0x0001;

    player_ptr->is_dead = false;
    (void)life_stream(player_ptr, false, false);
    (void)restore_mana(player_ptr, true);
    (void)recall_player(player_ptr, 0);
    reserve_alter_reality(player_ptr, 0);

    player_ptr->died_from = _("死の欺き", "Cheating death");
    (void)set_food(player_ptr, PY_FOOD_MAX - 1);

    auto &floor = *player_ptr->current_floor_ptr;
    floor.dun_level = 0;
    floor.inside_arena = false;
    AngbandSystem::get_instance().set_phase_out(false);
    leaving_quest = QuestId::NONE;
    floor.quest_number = QuestId::NONE;
    if (floor.dungeon_idx) {
        player_ptr->recall_dungeon = floor.dungeon_idx;
    }

    floor.reset_dungeon_index();
    if (lite_town || vanilla_town) {
        player_ptr->wilderness_y = 1;
        player_ptr->wilderness_x = 1;
        if (vanilla_town) {
            player_ptr->oldpy = 10;
            player_ptr->oldpx = 34;
        } else {
            player_ptr->oldpy = 33;
            player_ptr->oldpx = 131;
        }
    } else {
        player_ptr->wilderness_y = 48;
        player_ptr->wilderness_x = 5;
        player_ptr->oldpy = 33;
        player_ptr->oldpx = 131;
    }

    world.set_wild_mode(false);
    player_ptr->leaving = true;
    constexpr auto note = _("                            しかし、生き返った。", "                            but revived.");
    exe_write_diary(floor, DiaryKind::DESCRIPTION, 1, note);
    leave_floor(player_ptr);
}
