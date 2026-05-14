#include "load/player-info-loader.h"
#include "avatar/avatar.h"
#include "combat/martial-arts-style.h"
#include "load/angband-version-comparer.h"
#include "load/birth-loader.h"
#include "load/dummy-loader.h"
#include "load/load-util.h"
#include "load/load-zangband.h"
#include "load/old/load-v1-7-0.h"
#include "load/player-attack-loader.h"
#include "load/player-class-specific-data-loader.h"
#include "load/world-loader.h"
#include "market/arena-entry.h"
#include "mind/mind-elementalist.h"
#include "monster-race/race-ability-flags.h"
#include "mutation/mutation-calculator.h"
#include "object/tval-types.h"
#include "player-base/player-class.h"
#include "player-info/mane-data-type.h"
#include "player-info/sniper-data-type.h"
#include "player/attack-defense-types.h"
#include "player/player-realm.h"
#include "player/player-skill.h"
#include "spell-realm/spells-song.h"
#include "system/angband-exceptions.h"
#include "system/angband-system.h"
#include "system/building-type-definition.h"
#include "system/creature-entity.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor/floor-info.h"
#include "system/inner-game-data.h"
#include "timed-effect/timed-effects.h"
#include "world/world.h"

/*!
 * @brief セーブデータから領域情報を読み込む / Read creature realms
 * @param creature クリーチャーへの参照
 */
static void rd_realms(CreatureEntity &creature)
{
    PlayerRealm pr(creature);
    pr.reset();

    if (CreatureClass(creature).equals(PlayerClassType::ELEMENTALIST)) {
        creature.element_realm = i2enum<ElementRealmType>(rd_byte());
        (void)rd_byte();
        return;
    }

    const auto realm1 = i2enum<RealmType>(rd_byte());
    auto realm2 = rd_byte();
    if (realm1 == RealmType::NONE) {
        return;
    }
    if (realm2 == 255) { // 何のため？
        realm2 = 0;
    }
    pr.set(realm1, i2enum<RealmType>(realm2));
}

/*!
 * @brief セーブデータからプレイヤー基本情報を読み込む / Read creature's basic info
 * @param creature クリーチャーへの参照
 */
void rd_base_info(CreatureEntity &creature)
{
    creature.name = rd_string();
    if (creature.name.length() > 40) {
        creature.name.resize(40);
    }
    creature.died_from = rd_string();
    creature.last_message = rd_string();

    load_quick_start();
    const int max_history_lines = 4;
    for (int i = 0; i < max_history_lines; i++) {
        const auto history = rd_string();
        const auto history_len = history.copy(creature.history[i], sizeof(creature.history[i]) - 1);
        creature.history[i][history_len] = '\0';
    }

    creature.prace = i2enum<PlayerRaceType>(rd_byte());
    creature.pclass = i2enum<PlayerClassType>(rd_byte());
    creature.ppersonality = i2enum<player_personality_type>(rd_byte());
    creature.psex = i2enum<player_sex>(rd_byte());

    rd_realms(creature);

    strip_bytes(1);

    creature.hit_dice = Dice(1, rd_byte());
    creature.expfact = rd_u16b();

    creature.death_count = rd_s32b();
    creature.set_age(rd_s16b());
    creature.set_ht(rd_s16b());
    creature.set_wt(rd_s16b());

    // 死亡履歴のロード（バージョン44以降）
    if (loading_savefile_version_is_older_than(44)) {
        creature.death_history.clear();
    } else {
        const auto death_history_size = rd_u32b();
        creature.death_history.clear();
        creature.death_history.reserve(death_history_size);
        for (uint32_t i = 0; i < death_history_size; ++i) {
            DeathRecord record;
            record.game_turn = rd_s32b();
            record.day = rd_s16b();
            record.hour = rd_s16b();
            record.min = rd_s16b();
            record.player_level = rd_s16b();
            record.cause = rd_string();
            record.killer_monrace_id = i2enum<MonraceId>(rd_s16b());
            creature.death_history.push_back(record);
        }
    }
}

void rd_experience(CreatureEntity &creature)
{
    creature.max_exp = rd_s32b();
    creature.max_max_exp = rd_s32b();

    creature.exp = rd_s32b();
    creature.exp_frac = rd_u32b();

    creature.level = rd_s16b();
    for (int i = 0; i < 64; i++) {
        creature.spell_exp[i] = rd_s16b();
    }

    const int max_weapon_exp_size = 64;
    for (auto tval : TV_WEAPON_RANGE) {
        for (int j = 0; j < max_weapon_exp_size; j++) {
            creature.weapon_exp[tval][j] = rd_s16b();
        }
    }

    for (auto i : PLAYER_SKILL_KIND_TYPE_RANGE) {
        creature.skill_exp[i] = rd_s16b();
    }

    // resreved skills
    strip_bytes(sizeof(int16_t) * (MAX_SKILLS - PLAYER_SKILL_KIND_TYPE_RANGE.size()));

    // Load martial arts style
    if (loading_savefile_version_is_older_than(30)) {
        // 古いセーブファイルではデフォルト値を設定
        creature.martial_arts_style = MartialArtsStyleType::TRADITIONAL;
    } else {
        creature.martial_arts_style = static_cast<MartialArtsStyleType>(rd_s16b());
    }
}

void rd_skills(CreatureEntity &creature)
{
    CreatureClass(creature).init_specific_data();
    std::visit(PlayerClassSpecificDataLoader(), creature.class_specific_data);

    if (music_singing_any(creature)) {
        creature.action = ACTION_SING;
    }
}

static void set_race(CreatureEntity &creature)
{
    InnerGameData::get_instance().set_start_race(i2enum<PlayerRaceType>(rd_byte()));
    creature.old_race1 = rd_u32b();
    creature.old_race2 = rd_u32b();
    creature.old_realm = rd_s16b();
}

void rd_bounty_uniques()
{
    for (auto &[bounty_monrace_id, is_achieved] : AngbandWorld::get_instance().bounties) {
        auto monrace_id = rd_s16b();
        if (loading_savefile_version_is_older_than(16)) {
            constexpr auto old_achieved_flag = 10000; // かつて賞金首達成フラグとしてモンスター種族番号を10000増やしていた
            is_achieved = false;
            if (monrace_id >= old_achieved_flag) {
                monrace_id -= old_achieved_flag;
                is_achieved = true;
            }
        } else {
            is_achieved = rd_bool();
        }

        bounty_monrace_id = i2enum<MonraceId>(monrace_id);
    }
}

/*!
 * @brief 腕力などの基本ステータス情報を読み込む
 * @param creature クリーチャーへの参照
 * @details セーブファイルバージョン36以降は30-400形式、それ以前は3-18/220形式
 */
static void rd_base_status(CreatureEntity &creature)
{
    auto convert_old_stat_to_new = [](int16_t old_val) -> int16_t {
        // 旧形式 (3-18/220) を新形式 (30-400) に変換
        if (old_val <= 18) {
            // 3-18 -> 30-180
            return old_val * 10;
        } else {
            // 18/xx -> 180-400
            // 18/00 = 19 -> 190
            // 18/220 = 238 -> 400
            int bonus = old_val - 18;
            if (bonus > 220) {
                bonus = 220;
            }
            return 180 + (bonus * 220 / 220);
        }
    };

    bool is_old_format = loading_savefile_version_is_older_than(36);

    for (int i = 0; i < A_MAX; i++) {
        creature.stat_max[i] = rd_s16b();
        if (is_old_format) {
            creature.stat_max[i] = convert_old_stat_to_new(creature.stat_max[i]);
        }
    }

    for (int i = 0; i < A_MAX; i++) {
        creature.stat_max_max[i] = rd_s16b();
        if (is_old_format) {
            creature.stat_max_max[i] = convert_old_stat_to_new(creature.stat_max_max[i]);
        }
    }

    for (int i = 0; i < A_MAX; i++) {
        creature.stat_cur[i] = rd_s16b();
        if (is_old_format) {
            creature.stat_cur[i] = convert_old_stat_to_new(creature.stat_cur[i]);
        }
    }
}

static void set_imitation(CreatureEntity &creature)
{
    if (loading_savefile_version_is_older_than(11)) {
        auto mane_data = CreatureClass(creature).get_specific_data<mane_data_type>();
        if (!mane_data) {
            // ものまね師でない場合に読み捨てるためのダミーデータ領域
            mane_data = std::make_shared<mane_data_type>();
        }

        for (int i = 0; i < MAX_MANE; ++i) {
            auto spell = rd_s16b();
            auto damage = rd_s16b();
            mane_data->mane_list.push_back({ i2enum<MonsterAbilityType>(spell), damage });
        }
        auto count = rd_s16b();
        mane_data->mane_list.resize(count);
    }
}

static void rd_phase_out(CreatureEntity &creature)
{
    auto &system = AngbandSystem::get_instance();
    creature.get_floor()->inside_arena = rd_s16b() != 0;
    creature.get_floor()->quest_number = i2enum<QuestId>(rd_s16b());
    system.set_phase_out(rd_s16b() != 0);
}

static void rd_arena(CreatureEntity &creature)
{
    set_gambling_monsters();

    creature.town_num = rd_s16b();
    auto &entries = ArenaEntryList::get_instance();
    entries.load_current_entry(rd_s16b());
    if (loading_savefile_version < 28) {
        const auto currrent_entry = entries.get_current_entry();
        if (currrent_entry < 0) {
            entries.load_current_entry(-currrent_entry);
            entries.set_defeated_entry();
        }
    } else {
        entries.load_defeated_entry(rd_s16b());
    }

    rd_phase_out(creature);
    AngbandWorld::get_instance().set_arena(rd_bool());
    strip_bytes(1);

    creature.oldpx = rd_s16b();
    creature.oldpy = rd_s16b();
}

/*!
 * @brief プレイヤーの最大HP/現在HPを読み込む
 * @param creature クリーチャーへの参照
 */
static void rd_hp(CreatureEntity &creature)
{
    creature.maxhp = rd_s32b();
    creature.hp = rd_s32b();
    creature.hp_frac = rd_u32b();

    // セーブファイルバージョン35以降で与ダメージ蓄積を読み込み
    if (!loading_savefile_version_is_older_than(35)) {
        creature.dealt_damage = rd_s32b();
    } else {
        creature.dealt_damage = 0;
    }
}

/*!
 * @brief プレイヤーの最大MP/現在MPを読み込む
 * @param creature クリーチャーへの参照
 */
static void rd_mana(CreatureEntity &creature)
{
    creature.msp = rd_s32b();
    creature.csp = rd_s32b();
    creature.csp_frac = rd_u32b();
}

/*!
 * @brief プレイヤーのバッドステータス (と空腹)を読み込む
 * @param creature クリーチャーへの参照
 */
static void rd_bad_status(CreatureEntity &creature)
{
    const auto effects = creature.effects();
    strip_bytes(2); /* Old "rest" */
    effects->blindness().set(rd_s16b());
    effects->paralysis().set(rd_s16b());
    effects->confusion().set(rd_s16b());
    creature.food = rd_s16b();
    strip_bytes(4); /* Old "food_digested" / "protection" */
}

static void rd_energy(CreatureEntity &creature)
{
    creature.energy_need = rd_s16b();
    creature.enchant_energy_need = rd_s16b();
}

/*!
 * @brief プレイヤーのグッド/バッドステータスを読み込む
 * @param creature クリーチャーへの参照
 * @todo 明らかに関数名がビッグワードだが他に思いつかなかった
 */
static void rd_status(CreatureEntity &creature)
{
    const auto effects = creature.effects();
    effects->acceleration().set(rd_s16b());
    effects->deceleration().set(rd_s16b());
    effects->fear().set(rd_s16b());
    effects->cut().set(rd_s16b());
    effects->stun().set(rd_s16b());
    effects->poison().set(rd_s16b());
    effects->hallucination().set(rd_s16b());
    effects->protection().set(rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::INVULNERABILITY, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::ULTIMATE_RESISTANCE, rd_s16b());
}

static void set_timed_effects(CreatureEntity &creature)
{
    creature.set_timed_effect(CreatureTimedEffect::TIM_ESP, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::WRAITH_FORM, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::RESIST_MAGIC, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::TIM_REGEN, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::TIM_PASS_WALL, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::TIM_STEALTH, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::TIM_LEVITATION, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::TIM_SH_TOUKI, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::LIGHTSPEED, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::TSUBURERU, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::MAGICDEF, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::TIM_RES_NETHER, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::TIM_RES_TIME, rd_s16b());
    creature.set_mimic_form(i2enum<MimicKindType>(rd_byte()));
    creature.set_timed_effect(CreatureTimedEffect::TIM_MIMIC, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::TIM_SH_FIRE, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::TIM_SH_HOLY, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::TIM_EYEEYE, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::TIM_REFLECT, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::MULTISHADOW, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::DUSTROBE, rd_s16b());

    if (!loading_savefile_version_is_older_than(37)) {
        creature.set_timed_effect(CreatureTimedEffect::TIM_RES_LITE, rd_s16b());
        creature.set_timed_effect(CreatureTimedEffect::TIM_RES_DARK, rd_s16b());
        creature.set_timed_effect(CreatureTimedEffect::TIM_RES_FEAR, rd_s16b());
        creature.set_timed_effect(CreatureTimedEffect::TIM_EMISSION, rd_s16b());
        creature.set_timed_effect(CreatureTimedEffect::TIM_EXORCISM, rd_s16b());
        creature.set_timed_effect(CreatureTimedEffect::TIM_IMM_DARK, rd_s16b());
    }
}

static void set_mutations(CreatureEntity &creature)
{
    if (loading_savefile_version_is_older_than(2)) {
        for (int i = 0; i < 3; i++) {
            auto tmp32u = rd_u32b();
            migrate_bitflag_to_flaggroup(creature.muta, tmp32u, i * 32);
        }
    } else {
        rd_FlagGroup(creature.muta, rd_byte);

        if (!loading_savefile_version_is_older_than(7)) {
            rd_FlagGroup(creature.trait, rd_byte);
        }
    }
}

static void set_virtues(CreatureEntity &creature)
{
    // Load virtue values (8 entries)
    int16_t virtue_values[8];
    for (int i = 0; i < 8; i++) {
        virtue_values[i] = rd_s16b();
    }

    // Load virtue types (8 entries) and build map
    creature.virtues.clear();
    for (int i = 0; i < 8; i++) {
        auto vir_type = i2enum<Virtue>(rd_s16b());
        if (vir_type != Virtue::NONE && vir_type < Virtue::MAX) {
            creature.virtues[vir_type] = virtue_values[i];
        }
    }
}

/*!
 * @brief 各種時限効果を読み込む
 * @param creature クリーチャーへの参照
 */
static void rd_timed_effects(CreatureEntity &creature)
{
    set_timed_effects(creature);
    creature.patron = rd_s16b();
    set_mutations(creature);
    set_virtues(creature);
}

static void rd_player_status(CreatureEntity &creature)
{
    rd_base_status(creature);
    strip_bytes(24);
    creature.au = rd_s32b();
    rd_experience(creature);
    rd_skills(creature);
    set_race(creature);
    set_imitation(creature);
    rd_bounty_uniques();
    rd_arena(creature);
    rd_dummy1();
    rd_hp(creature);
    rd_mana(creature);
    creature.max_plv = rd_s16b();
    rd_dungeons(creature);
    strip_bytes(8);
    creature.set_prestige(rd_s16b());
    if (loading_savefile_version_is_older_than(11)) {
        auto sniper_data = CreatureClass(creature).get_specific_data<SniperData>();
        if (sniper_data) {
            sniper_data->concent = rd_s16b();
        } else {
            // 職業がスナイパーではないので読み捨てる
            strip_bytes(2);
        }
    }
    rd_bad_status(creature);
    rd_energy(creature);
    rd_status(creature);
    creature.set_timed_effect(CreatureTimedEffect::HERO, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::BERSERK, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::SHIELD, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::BLESSED, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::TIM_INVIS, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::WORD_RECALL, rd_s16b());
    creature.recall_dungeon = i2enum<DungeonId>(rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::ALTER_REALITY, rd_s16b());
    creature.see_infra = rd_s16b();
    creature.set_timed_effect(CreatureTimedEffect::TIM_INFRA, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::OPPOSE_FIRE, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::OPPOSE_COLD, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::OPPOSE_ACID, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::OPPOSE_ELEC, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::OPPOSE_POIS, rd_s16b());
    creature.set_timed_effect(CreatureTimedEffect::TSUYOSHI, rd_s16b());
    rd_timed_effects(creature);
    creature.mutant_regenerate_mod = calc_mutant_regenerate_mod(creature);

    if (!loading_savefile_version_is_older_than(6)) {
        int32_t num = rd_s32b();
        for (int32_t i = 0; i < num * 2; i += 2) {
            int32_t id, count;
            id = rd_s32b();
            count = rd_s32b();
            creature.incident[(INCIDENT)id] = count;
        }
    }

    // Load incident_tree (string-keyed, tree-structured incidents)
    if (!loading_savefile_version_is_older_than(31)) {
        int32_t num = rd_s32b();
        for (int32_t i = 0; i < num; i++) {
            std::string key = rd_string();
            int32_t count = rd_s32b();
            creature.incident_tree[key] = count;
        }
    }
}

void rd_player_info(CreatureEntity &creature)
{
    rd_player_status(creature);
    rd_special_attack(creature);
    rd_special_action(creature);
    rd_special_defense(creature);
    creature.knowledge = rd_byte();
    rd_autopick(creature);
    rd_action(creature);
}
