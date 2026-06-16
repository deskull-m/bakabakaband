#include "monster-floor/monster-death.h"
#include "alliance/alliance.h"
#include "artifact/fixed-art-generator.h"
#include "artifact/fixed-art-types.h"
#include "cmd-visual/cmd-draw.h"
#include "dungeon/quest-completion-checker.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/floor-object.h"
#include "game-option/birth-options.h"
#include "game-option/game-play-options.h"
#include "game-option/play-record-options.h"
#include "io/input-key-acceptor.h"
#include "io/write-diary.h"
#include "main/music-definitions-table.h"
#include "main/sound-of-music.h"
#include "market/arena-entry.h"
#include "monster-floor/monster-death-util.h"
#include "monster-floor/special-death-switcher.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/race-brightness-mask.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/item-magic-applier.h"
#include "pet/pet-fall-off.h"
#include "player/patron.h"
#include "player/process-death.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-scroll-types.h"
#include "system/angband-system.h"
#include "system/artifact-type-definition.h"
#include "system/artifact/artifact-record.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/building-type-definition.h"
#include "system/creature-entity.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/redrawing-flags-updater.h"
#include "system/services/baseitem-monrace-service.h"
#include "system/system-variables.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world-collapsion.h"
#include "world/world.h"
#include <algorithm>

static void write_pet_death(CreatureEntity &creature, MonsterDeath *md_ptr)
{
    md_ptr->md_y = md_ptr->m_ptr->y;
    md_ptr->md_x = md_ptr->m_ptr->x;
    if (record_named_pet && md_ptr->m_ptr->is_named_pet()) {
        const auto m_name = monster_desc(creature, *md_ptr->m_ptr, MD_INDEF_VISIBLE);
        exe_write_diary(*creature.get_floor(), DiaryKind::NAMED_PET, 3, m_name);
    }
}

static void on_dead_explosion(CreatureEntity &creature, MonsterDeath *md_ptr)
{
    for (const auto &blow : md_ptr->monrace->blows) {
        if (blow.method != RaceBlowMethodType::EXPLODE) {
            continue;
        }

        BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
        AttributeType typ = mbe_info[enum2i(blow.effect)].explode_type;
        const auto damage = blow.damage_dice.roll();
        (void)project(creature, md_ptr->m_idx, 3, md_ptr->md_y, md_ptr->md_x, damage, typ, flg);
        break;
    }
}

static void on_defeat_arena_monster(CreatureEntity &creature, MonsterDeath *md_ptr)
{
    const auto &floor = *creature.get_floor();
    if (!floor.inside_arena || md_ptr->m_ptr->is_pet()) {
        return;
    }

    AngbandWorld::get_instance().set_arena(true);
    auto &entries = ArenaEntryList::get_instance();
    const auto is_true_victor = entries.is_player_true_victor();
    if (is_true_victor) {
        msg_print(_("素晴らしい！君こそ真の勝利者だ。", "You are a Genuine Champion!"));
    } else {
        msg_print(_("勝利！チャンピオンへの道を進んでいる。", "Victorious! You're on your way to becoming Champion."));
    }

    const auto &bi_key = entries.get_bi_key();
    if (bi_key.is_valid()) {
        ItemEntity item(bi_key);
        ItemMagicApplier(creature, &item, floor.object_level, AM_NO_FIXED_ART).execute();
        (void)drop_near(creature, item, md_ptr->get_position());
    }

    if (is_true_victor) {
        entries.increment_entry();
    }

    entries.increment_entry();
    if (!record_arena) {
        return;
    }

    const auto m_name = monster_desc(creature, *md_ptr->m_ptr, MD_WRONGDOER_NAME);
    exe_write_diary(floor, DiaryKind::ARENA, 0, m_name);
}

static void drop_corpse(CreatureEntity &creature, MonsterDeath *md_ptr)
{
    const auto &floor = *creature.get_floor();
    auto is_drop_corpse = one_in_(md_ptr->monrace->kind_flags.has(MonsterKindType::UNIQUE) ? 1 : 4);
    is_drop_corpse &= md_ptr->monrace->drop_flags.has_any_of({ MonsterDropType::DROP_CORPSE, MonsterDropType::DROP_SKELETON });
    is_drop_corpse &= !(floor.inside_arena || AngbandSystem::get_instance().is_phase_out() || md_ptr->cloned || ((md_ptr->m_ptr->get_r_idx() == AngbandWorld::get_instance().today_mon) && md_ptr->m_ptr->is_pet()));
    if (!is_drop_corpse) {
        return;
    }

    bool corpse = false;
    if (md_ptr->monrace->drop_flags.has_not(MonsterDropType::DROP_SKELETON)) {
        corpse = true;
    } else if (md_ptr->monrace->drop_flags.has(MonsterDropType::DROP_CORPSE) && md_ptr->monrace->kind_flags.has(MonsterKindType::UNIQUE)) {
        corpse = true;
    } else if (md_ptr->monrace->drop_flags.has(MonsterDropType::DROP_CORPSE)) {
        if ((0 - ((md_ptr->m_ptr->maxhp) / 4)) > md_ptr->m_ptr->hp) {
            if (one_in_(5)) {
                corpse = true;
            }
        } else {
            if (!one_in_(5)) {
                corpse = true;
            }
        }
    }

    ItemEntity item({ ItemKindType::MONSTER_REMAINS, (corpse ? SV_CORPSE : SV_SKELETON) });
    ItemMagicApplier(creature, &item, floor.object_level, AM_NO_FIXED_ART).execute();
    item.pval = enum2i(md_ptr->m_ptr->get_r_idx());
    (void)drop_near(creature, item, md_ptr->get_position());

    try {
        if (one_in_(md_ptr->monrace->kind_flags.has(MonsterKindType::UNIQUE) ? 1 : 4)) {
            item.generate(BaseitemList::get_instance().lookup_baseitem_id({ ItemKindType::MONSTER_REMAINS, SV_SOUL }));
            item.pval = enum2i(md_ptr->m_ptr->get_r_idx());
            (void)drop_near(creature, item, md_ptr->get_position());
        }
    } catch (const std::exception &e) {
        msg_format(_("エラー:ソウルドロップの処理に失敗", "Error: Failed to drop a soul."), e.what());
#ifndef WIN_DEBUG
        throw;
#endif
    }

    if (md_ptr->monrace->drop_flags.has(MonsterDropType::DROP_JUNK)) {
        ItemEntity item_junk({ ItemKindType::MONSTER_REMAINS, SV_JUNK });
        item_junk.pval = enum2i(md_ptr->m_ptr->get_r_idx());
        (void)drop_near(creature, item_junk, md_ptr->get_position());
    }
}

/*!
 * @brief アーティファクトのドロップ判定処理
 * @param creature クリーチャーへの参照
 * @param md_ptr モンスター死亡構造体への参照ポインタ
 * @return 何かドロップするならドロップしたアーティファクトのID、何もドロップしないなら0
 */
static void drop_artifact_from_unique(CreatureEntity &creature, MonsterDeath *md_ptr)
{
    const auto is_wizard = AngbandWorld::get_instance().wizard;
    for (const auto &[a_idx, chance] : md_ptr->monrace->get_drop_artifacts()) {
        if (!is_wizard && !evaluate_percent(chance)) {
            continue;
        }

        if (drop_single_artifact(creature, md_ptr, a_idx)) {
            return;
        }
    }
}

/*!
 * @brief 特定アーティファクトのドロップ処理
 * @param creature クリーチャーへの参照
 * @param md_ptr モンスター死亡構造体への参照ポインタ
 * @param a_ix ドロップを試みるアーティファクトID
 * @return ドロップするならtrue
 */
bool drop_single_artifact(CreatureEntity &creature, MonsterDeath *md_ptr, FixedArtifactId a_idx)
{
    if (ArtifactRecords::get_instance().get_generated(a_idx)) {
        return false;
    }

    return create_named_art(creature, a_idx, md_ptr->md_y, md_ptr->md_x);
}

static tl::optional<short> drop_dungeon_final_artifact(CreatureEntity &creature, MonsterDeath *md_ptr)
{
    const auto &dungeon = creature.get_floor()->get_dungeon_definition();
    const auto has_reward = dungeon.final_object > 0;
    const auto bi_id = has_reward ? dungeon.final_object : BaseitemList::get_instance().lookup_baseitem_id({ ItemKindType::SCROLL, SV_SCROLL_ACQUIREMENT });
    if (dungeon.final_artifact == FixedArtifactId::NONE) {
        return bi_id;
    }

    const auto a_idx = dungeon.final_artifact;
    if (ArtifactRecords::get_instance().get_generated(a_idx)) {
        return bi_id;
    }

    create_named_art(creature, a_idx, md_ptr->md_y, md_ptr->md_x);
    return dungeon.final_object ? tl::make_optional<short>(bi_id) : tl::nullopt;
}

static void drop_artifacts(CreatureEntity &creature, MonsterDeath *md_ptr)
{
    if (!md_ptr->drop_chosen_item) {
        return;
    }

    drop_artifact_from_unique(creature, md_ptr);
    const auto &floor = *creature.get_floor();
    const auto &dungeon = floor.get_dungeon_definition();
    if (md_ptr->monrace->misc_flags.has_not(MonsterMiscType::GUARDIAN) || (dungeon.final_guardian != md_ptr->m_ptr->get_r_idx())) {
        return;
    }

    const auto bi_id = drop_dungeon_final_artifact(creature, md_ptr);
    if (bi_id) {
        ItemEntity item(*bi_id);
        ItemMagicApplier(creature, &item, floor.object_level, AM_NO_FIXED_ART | AM_GOOD).execute();
        (void)drop_near(creature, item, md_ptr->get_position());
    }

    msg_format(_("あなたは%sを制覇した！", "You have conquered %s!"), dungeon.name.data());
}

/*!
 * @brief 最終ボス(混沌のサーペント)を倒したときの処理
 * @param creature クリーチャーへの参照
 */
static void on_defeat_last_boss(CreatureEntity &creature)
{
    auto &world = AngbandWorld::get_instance();
    world.total_winner = true;
    world.add_winner_class(creature.pclass);
    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::TITLE);
    play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_FINAL_QUEST_CLEAR);
    exe_write_diary(*creature.get_floor(), DiaryKind::DESCRIPTION, 0, _("見事に馬鹿馬鹿蛮怒の勝利者となった！", "finally became *WINNER* of Bakabakaband!"));
    patron_list[creature.get_patron()].admire(creature);
    msg_print(_("*** おめでとう ***", "*** CONGRATULATIONS ***"));
    msg_print(_("あなたはゲームをコンプリートしました。", "You have won the game!"));
    msg_print(_("準備が整ったら引退(自殺コマンド)しても結構です。", "You may retire (commit suicide) when you are ready."));
}

/*!
 * @brief モンスターが死亡した時の処理 /
 * Handle the "death" of a monster.
 * @param m_idx 死亡したモンスターのID
 * @param drop_item TRUEならばモンスターのドロップ処理を行う
 * @param type ラストアタックの属性 (単一属性)
 */
void monster_death(CreatureEntity &creature, MONSTER_IDX m_idx, bool drop_item, AttributeType type)
{
    AttributeFlags flags;
    flags.clear();
    flags.set(type);
    monster_death(creature, m_idx, drop_item, flags);
}

/*!
 * @brief モンスターが死亡した時の処理 /
 * Handle the "death" of a monster.
 * @param m_idx 死亡したモンスターのID
 * @param drop_item TRUEならばモンスターのドロップ処理を行う
 * @param attribute_flags ラストアタックの属性 (複数属性)
 * @details
 * <pre>
 * Disperse treasures centered at the monster location based on the
 * various flags contained in the monster flags fields.
 * Check for "Quest" completion when a quest monster is killed.
 * Note that only the player can induce "monster_death()" on Uniques.
 * Thus (for now) all Quest monsters should be Uniques.
 * Note that monsters can now carry objects, and when a monster dies,
 * it drops all of its objects, which may disappear in crowded rooms.
 * </pre>
 */
void monster_death(CreatureEntity &creature, MONSTER_IDX m_idx, bool drop_item, AttributeFlags attribute_flags)
{
    auto &floor = *creature.get_floor();

    MonsterDeath md(floor, m_idx, drop_item);
    auto &world = AngbandWorld::get_instance();
    if ((world.timewalk_m_idx > 0) && world.timewalk_m_idx == m_idx) {
        world.timewalk_m_idx = 0;
    }

    // プレイヤーしかユニークを倒せないのでここで時間を記録
    if (md.monrace->kind_flags.has(MonsterKindType::UNIQUE) && !md.m_ptr->has_constant_flag(MonsterConstantFlagType::CLONED)) {
        world.play_time.update();
        md.monrace->defeat_time = world.play_time.elapsed_sec();
        md.monrace->defeat_level = creature.get_level();
    }

    if (md.monrace->brightness_flags.has_any_of(ld_mask)) {
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::MONSTER_LITE);
    }

    write_pet_death(creature, &md);
    on_dead_explosion(creature, &md);
    if (md.m_ptr->is_chameleon()) {
        md.m_ptr->reset_chameleon_polymorph();
        md.monrace = md.m_ptr->get_monrace_shared();
    }

    // ジョークオプション：モンスターの墓石を立てる
    if (monster_tombstones) {
        screen_save();
        print_monster_tomb(creature, *md.m_ptr);
        msg_print(_("-続けるには何かキーを押してください-", "-Press any key to continue-"));
        screen_load();
        do_cmd_redraw(creature);
    }

    QuestCompletionChecker(creature, *md.m_ptr).complete();
    on_defeat_arena_monster(creature, &md);
    if (md.m_ptr->is_riding() && process_fall_off_horse(creature, -1, false)) {
        msg_print(_("地面に落とされた。", "You have fallen from the pet you were riding."));
    }

    wc_ptr->plus_collapsion(md.monrace->plus_collapse);

    // オディオアライアンスがNONLIVINGでないモンスター死亡時に戦力を増加
    if (md.monrace->kind_flags.has_not(MonsterKindType::NONLIVING)) {
        auto it = alliance_list.find(AllianceType::ODIO);
        if (it != alliance_list.end()) {
            // モンスターのレベルに応じて戦力を増加（レベル * 10）
            const auto power_gain = md.monrace->level * 10;
            it->second->base_power += power_gain;
        }
    }

    drop_corpse(creature, &md);
    // [ドロップ品移行] 一般ドロップ品はモンスター生成時に所持品として前生成済み。
    // 死亡時は drop_all_inventory() でまとめて床へ放出する。ただし drop_item が
    // false のケース (量子消失・モンスター同士の戦闘・一部効果死) では従来同様に
    // ドロップを抑制する (モンスターピット等での床溢れ防止)。
    if (drop_item) {
        md.m_ptr->drop_all_inventory(creature);
    }

    switch_special_death(creature, &md, attribute_flags);
    drop_artifacts(creature, &md);
    if ((md.monrace->misc_flags.has_not(MonsterMiscType::QUESTOR)) || AngbandSystem::get_instance().is_phase_out() || (md.m_ptr->get_r_idx() != MonraceId::SERPENT) || md.cloned) {
        return;
    }

    on_defeat_last_boss(creature);
}
