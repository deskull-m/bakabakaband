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
#include "monster-floor/monster-corpse-leaver.h"
#include "monster-floor/monster-death-util.h"
#include "monster-floor/special-death-switcher.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/race-brightness-mask.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-party.h"
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
#include "system/deferred-update-requests.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/floor/monster-corpse.h"
#include "system/inner-game-data.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/services/baseitem-monrace-service.h"
#include "system/system-variables.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world-collapsion.h"
#include "world/world.h"
#include <algorithm>
#include <fmt/format.h>

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

/*!
 * @brief この個体が死体を遺すか判定する
 * @details 死体を遺す種族 (DROP_CORPSE / DROP_SKELETON) なら**必ず**遺す。
 *          以前は非ユニークで `one_in_(4)` の抽選だったが、死体を漁って戦利品を
 *          得る形へ移行するにあたり撤去した (Issue #9275 第 2 段)。
 *          闘技場・モンスター闘技場・クローン体・その日のペットは従来どおり除外する。
 */
static bool decide_leaving_corpse(const CreatureEntity &creature, const MonsterDeath *md_ptr)
{
    const auto &floor = *creature.get_floor();
    if (md_ptr->monrace->drop_flags.has_none_of({ MonsterDropType::DROP_CORPSE, MonsterDropType::DROP_SKELETON })) {
        return false;
    }

    const auto is_today_pet = (md_ptr->m_ptr->get_r_idx() == AngbandWorld::get_instance().today_mon) && md_ptr->m_ptr->is_pet();
    return !(floor.inside_arena || AngbandSystem::get_instance().is_phase_out() || md_ptr->cloned || is_today_pet);
}

/*!
 * @brief 死体から剥ぎ取れる鱗の枚数を算出する
 * @param monster 死亡したモンスターへの参照
 * @param monrace その種族定義への参照
 * @return 鱗の枚数 (最低 1 枚)
 * @details 「ドラゴンの大きさに応じて」の大きさは 2 つの軸で決まる。
 *          - **種族の大きさ**: 本データではドラゴンの体躯は種族レベルとよく相関する
 *            (ベビー Lv9〜 / ヤング Lv25〜 / 古代 Lv38〜 / ワイアーム Lv54〜) ため、
 *            レベルを体躯の近似として `SCALE_LEVEL_UNIT` ごとに 1 枚積む。
 *          - **個体の体格**: 生成時に付く巨大/大型/小型フラグ
 *            (`one_monster_placer.cpp`) で倍率を掛ける。ユニークには体格フラグが
 *            付かないため種族レベルぶんのみとなる。
 *          枚数の調整は本関数の 4 定数で行う。
 */
static int calc_scale_drop_number(const CreatureEntity &monster, const MonraceDefinition &monrace)
{
    constexpr auto SCALE_BASE_NUMBER = 1;
    constexpr auto SCALE_LEVEL_UNIT = 10;
    constexpr auto SCALE_HUGE_MULTIPLIER = 3;
    constexpr auto SCALE_MAX_NUMBER = 99;

    auto number = SCALE_BASE_NUMBER + monrace.level / SCALE_LEVEL_UNIT;
    if (monster.is_huge()) {
        number *= SCALE_HUGE_MULTIPLIER;
    } else if (monster.is_large()) {
        number = number * 3 / 2;
    } else if (monster.is_small()) {
        number /= 2;
    }

    return std::clamp(number, 1, SCALE_MAX_NUMBER);
}

/*!
 * @brief 死体/骨/ソウル/がらくた/鱗を死体の中へ納める
 * @param corpse_idx 納める死体の添字。0 なら死体が無いので従来どおり床へ落とす
 * @details **床へ落とさず死体の所持品にする**のが本関数の要点 (Issue #9275 第 2 段)。
 *          死体 1 体につき遺物が床へ 1〜3 個増えるのを避けるためで、
 *          プレイヤーは死体を漁って取り出す。死体が作れなかったとき
 *          (`MAX_FLOOR_CORPSES` 到達等) だけは品が失われないよう床へ落とす。
 */
static void store_monster_remains(CreatureEntity &creature, MonsterDeath *md_ptr, short corpse_idx)
{
    const auto &floor = *creature.get_floor();
    const auto monrace_id = md_ptr->m_ptr->get_r_idx();
    const auto deliver = [&creature, md_ptr, corpse_idx](const ItemEntity &item) {
        if (corpse_idx == 0) {
            auto floor_item = item.clone();
            (void)drop_near(creature, floor_item, md_ptr->get_position());
            return;
        }

        auto &corpse = creature.get_floor()->get_corpse(corpse_idx);
        if (corpse.body.store_item(item) < 0) {
            // 死体の所持枠が溢れた場合も品を失わないよう床へ逃がす
            auto floor_item = item.clone();
            (void)drop_near(creature, floor_item, md_ptr->get_position());
        }
    };

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
    item.pval = enum2i(monrace_id);
    deliver(item);

    try {
        if (one_in_(md_ptr->monrace->kind_flags.has(MonsterKindType::UNIQUE) ? 1 : 4)) {
            item.generate(BaseitemList::get_instance().lookup_baseitem_id({ ItemKindType::MONSTER_REMAINS, SV_SOUL }));
            item.pval = enum2i(monrace_id);
            deliver(item);
        }
    } catch (const std::exception &) {
        msg_print(_("エラー:ソウルドロップの処理に失敗", "Error: Failed to drop a soul."));
#ifndef WIN_DEBUG
        throw;
#endif
    }

    if (md_ptr->monrace->drop_flags.has(MonsterDropType::DROP_JUNK)) {
        ItemEntity item_junk({ ItemKindType::MONSTER_REMAINS, SV_JUNK });
        item_junk.pval = enum2i(monrace_id);
        deliver(item_junk);
    }

    // 鱗 (素材): ドラゴン等の鱗を持つ種族から、その大きさに応じた枚数を剥ぎ取れる。
    // 死体を遺さない種族では本関数自体が呼ばれないため、鱗も得られない。
    if (md_ptr->monrace->drop_flags.has(MonsterDropType::DROP_SCALE)) {
        ItemEntity item_scale({ ItemKindType::MONSTER_REMAINS, SV_SCALE });
        item_scale.pval = enum2i(monrace_id);
        item_scale.number = calc_scale_drop_number(*md_ptr->m_ptr, *md_ptr->monrace);
        deliver(item_scale);
    }
}

/*!
 * @brief アーティファクトのドロップ判定処理
 * @param creature クリーチャーへの参照
 * @param md_ptr モンスター死亡構造体への参照ポインタ
 * @return 何かドロップするならドロップしたアーティファクトのID、何もドロップしないなら0
 */
/*!
 * @brief JSON の `artifacts` 指定に従って固定アーティファクトを落とす
 * @details 各エントリは**独立に**確率抽選する (当たったものは全て落ちる)。
 *          『混沌のサーペント』のようにグロンドと混沌の王冠を同時に落とす種族が
 *          あるため、最初の 1 個で打ち切らない。既に生成済みの
 *          アーティファクトは `drop_single_artifact()` が弾く。
 */
static void drop_artifact_from_unique(CreatureEntity &creature, MonsterDeath *md_ptr, AttributeFlags attribute_flags)
{
    const auto is_wizard = AngbandWorld::get_instance().wizard;
    for (const auto &drop : md_ptr->monrace->get_drop_artifacts()) {
        // 属性条件は確率抽選より先に見る (条件を満たさない場合に乱数を消費しない)。
        if (drop.required_attribute && attribute_flags.has_not(*drop.required_attribute)) {
            continue;
        }

        if (!is_wizard && !evaluate_percent(drop.chance)) {
            continue;
        }

        (void)drop_single_artifact(creature, md_ptr, drop.fa_id);
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

static void drop_artifacts(CreatureEntity &creature, MonsterDeath *md_ptr, AttributeFlags attribute_flags)
{
    if (!md_ptr->drop_chosen_item) {
        return;
    }

    drop_artifact_from_unique(creature, md_ptr, attribute_flags);
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

    msg_print(_("あなたは{}を制覇した！", "You have conquered {}!"), dungeon.name);
}

/*!
 * @brief 最終ボス(混沌のサーペント)を倒したときの処理
 * @param creature クリーチャーへの参照
 */
static void on_defeat_last_boss(CreatureEntity &creature)
{
    auto &world = AngbandWorld::get_instance();
    world.total_winner = true;
    InnerGameData::get_instance().add_winner_class(creature.pclass);
    DeferredUpdateRequests::get_instance().set_flag(MainWindowRedrawingFlag::TITLE);
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
        DeferredUpdateRequests::get_instance().set_flag(StatusRecalculatingFlag::MONSTER_LITE);
    }

    write_pet_death(creature, &md);

    // 集団AI: リーダーが倒れたら生存メンバーの士気が崩壊する。
    // md.m_ptr はこの時点でまだ有効なので、パーティ走査の除外対象に使える。
    collapse_party_morale(creature, m_idx);

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

    md.leaves_corpse = decide_leaving_corpse(creature, &md);

    // 死体をフロアへ遺す。死体はこの時点の所持品を**複製して**抱えるので、
    // 床への放出より先に作る必要がある (Issue #9275 第 3 段)。
    const auto corpse_idx = md.leaves_corpse ? leave_monster_corpse(creature, m_idx) : 0;
    const auto keeps_inventory_in_corpse = corpse_idx > 0;

    // [ドロップ品移行] 一般ドロップ品はモンスター生成時に所持品として前生成済み。
    // **死体を遺せた個体は床へ撒かず死体の中に残す** (撃破ごとに床が散らかるのを
    // 防ぐのが本件の主目的)。死体を作れなかった場合 (MAX_FLOOR_CORPSES 到達・
    // 死体を遺さない種族) は従来どおり床へ放出する。ただし drop_item が false の
    // ケース (量子消失・モンスター同士の戦闘・一部効果死) では従来同様に
    // ドロップを抑制する (モンスターピット等での床溢れ防止)。
    if (drop_item && !keeps_inventory_in_corpse) {
        md.m_ptr->drop_all_inventory(creature);
    }

    // drop_item が false の経路では従来どおり所持品は失われる。死体へ残すと
    // モンスター同士の戦闘やクローンで装備を無限に得られてしまうため、
    // 死体の中身を空にする (遺物アイテムはこの後で入れる)。
    if (keeps_inventory_in_corpse && !drop_item) {
        floor.get_corpse(corpse_idx).body.discard_all_inventory();
    }

    // 遺物 (死体/骨/ソウル/がらくた) を死体の中へ納める
    if (md.leaves_corpse) {
        store_monster_remains(creature, &md, corpse_idx);
    }

    switch_special_death(creature, &md);
    drop_artifacts(creature, &md, attribute_flags);
    if ((md.monrace->misc_flags.has_not(MonsterMiscType::QUESTOR)) || AngbandSystem::get_instance().is_phase_out() || (md.m_ptr->get_r_idx() != MonraceId::SERPENT) || md.cloned) {
        return;
    }

    on_defeat_last_boss(creature);
}
