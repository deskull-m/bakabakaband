#include "save/player-writer.h"
#include "avatar/avatar.h"
#include "combat/martial-arts-style.h"
#include "floor/dungeon-feeling.h"
#include "game-option/birth-options.h"
#include "market/arena-entry.h"
#include "object/tval-types.h"
#include "player-base/player-class.h"
#include "player/player-realm.h"
#include "player/player-skill.h"
#include "save/creature-common-writer.h"
#include "save/info-writer.h"
#include "save/player-class-specific-data-writer.h"
#include "save/save-util.h"
#include "system/angband-system.h"
#include "system/building-type-definition.h"
#include "system/creature-entity.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/dungeon/dungeon-record.h"
#include "system/floor/floor-info.h"
#include "system/floor/town-records.h"
#include "system/inner-game-data.h"
#include "util/enum-converter.h"
#include "world/world.h"
#include <variant>

/*!
 * @brief セーブデータに領域情報を書き込む / Write player realms
 * @param creature クリーチャーへの参照
 */
static void wr_relams(CreatureEntity &creature)
{
    PlayerRealm pr(creature);
    if (CreatureClass(creature).equals(PlayerClassType::ELEMENTALIST)) {
        wr_byte((byte)creature.element_realm);
    } else {
        wr_byte((byte)pr.realm1().to_enum());
    }
    wr_byte((byte)pr.realm2().to_enum());
}

/*!
 * @brief セーブデータにプレイヤー情報を書き込む / Write some "player" info
 * @param creature クリーチャーへの参照
 */
void wr_player(CreatureEntity &creature)
{
    auto &system = AngbandSystem::get_instance();

    // セーブデータバージョン 51 以降: CreatureEntity 共通基底フィールドを
    // wr_creature_common() に集約する。以降のプレイヤー固有フィールドは
    // 共通フィールドを除いて従来の相対順序を保つ。
    wr_creature_common(creature);

    wr_string(creature.died_from);
    wr_string(creature.last_message);

    save_quick_start();
    for (int i = 0; i < 4; i++) {
        wr_string(creature.history[i]);
    }

    wr_byte((byte)creature.prace);
    wr_byte((byte)creature.pclass);
    wr_byte((byte)creature.ppersonality);
    wr_byte((byte)creature.psex);
    wr_relams(creature);
    wr_byte(0);

    wr_byte((byte)creature.hit_dice.sides);
    wr_u16b(creature.expfact);

    wr_s32b(creature.death_count);
    // age / ht / wt は wr_creature_common() で保存済み

    // 死亡履歴のセーブ
    wr_u32b(static_cast<uint32_t>(creature.death_history.size()));
    for (const auto &record : creature.death_history) {
        wr_s32b(record.game_turn);
        wr_s16b(record.day);
        wr_s16b(record.hour);
        wr_s16b(record.min);
        wr_s16b(record.player_level);
        wr_string(record.cause);
        wr_s16b(enum2i(record.killer_monrace_id));
    }

    // stat_max / stat_max_max / stat_cur は wr_creature_common() で保存済み (v52 拡張)

    for (int i = 0; i < 12; ++i) {
        wr_s16b(0);
    }

    // au / exp / max_exp / max_max_exp / exp_frac / level は wr_creature_common() で保存済み

    for (int i = 0; i < 64; i++) {
        wr_s16b(creature.get_spell_exp(i));
    }

    for (auto tval : TV_WEAPON_RANGE) {
        for (int j = 0; j < 64; j++) {
            wr_s16b(creature.get_weapon_exp(tval, j));
        }
    }

    for (auto i : PLAYER_SKILL_KIND_TYPE_RANGE) {
        wr_s16b(creature.get_skill_exp(i));
    }
    for (auto i = 0U; i < MAX_SKILLS - PLAYER_SKILL_KIND_TYPE_RANGE.size(); ++i) {
        // resreved skills
        wr_s16b(0);
    }

    // Save martial arts style
    wr_s16b(static_cast<int16_t>(creature.martial_arts_style));

    std::visit(PlayerClassSpecificDataWriter(), creature.class_specific_data);

    wr_byte(static_cast<uint8_t>(InnerGameData::get_instance().get_start_race()));
    wr_s32b(creature.get_old_race_flags1());
    wr_s32b(creature.get_old_race_flags2());
    wr_s16b(creature.get_old_realm());

    const auto &world = AngbandWorld::get_instance();
    for (const auto &[monrace_id, is_achieved] : world.bounties) {
        wr_s16b(enum2i(monrace_id));
        wr_bool(is_achieved);
    }

    const auto &melee_arena = MeleeArena::get_instance();
    for (const auto &gladiator : melee_arena.get_gladiators()) {
        wr_s16b(enum2i(gladiator.monrace_id));
        wr_u32b(gladiator.odds);
    }

    wr_s16b(creature.get_town_num());
    const auto &entries = ArenaEntryList::get_instance();
    wr_s16b(static_cast<int16_t>(entries.get_current_entry()));
    const auto defeated_entry = entries.get_defeated_entry();
    wr_s16b(static_cast<int16_t>(defeated_entry.value_or(-1)));
    wr_s16b(creature.get_floor()->inside_arena);
    wr_s16b(enum2i(creature.get_floor()->quest_number));
    wr_s16b(AngbandSystem::get_instance().is_phase_out());
    wr_byte(world.get_arena());
    wr_byte(0); /* Unused */

    wr_s16b((int16_t)creature.oldpx);
    wr_s16b((int16_t)creature.oldpy);

    wr_s16b(0);
    // maxhp / hp / dealt_damage / hp_frac / max_mp / current_mp / current_mp_frac は
    // wr_creature_common() で保存済み (hp_frac/max_mp/current_mp/current_mp_frac は v52 拡張)
    wr_s16b(creature.get_max_plv());

    const auto &dungeon_records = DungeonRecords::get_instance();
    auto tmp8u = static_cast<uint8_t>(dungeon_records.size());
    wr_byte(tmp8u);
    for (const auto &[_, dungeon_record] : dungeon_records) {
        wr_s16b(static_cast<int16_t>(dungeon_record->get_max_level()));
    }

    wr_s16b(0);
    wr_s16b(0);
    wr_s16b(0);
    wr_s16b(0);
    wr_s16b(creature.get_prestige());

    wr_s16b(0); /* old "rest" */
    // 全時限効果 (BLINDNESS / PARALYSIS / CUT / POISON / HALLUCINATION /
    // PROTECTION / ULTIMATE_RESISTANCE / HERO / ... / TIM_IMM_DARK 等すべて) は
    // wr_creature_common() の全時限効果ダンプ (v53) に集約済み。ここでは
    // 時限効果以外の (時限効果と交互配置されていた) フィールドのみ書き出す。
    wr_s16b(creature.get_food());
    wr_s16b(0); /* old "food_digested" */
    wr_s16b(0); /* old "protection" */
    wr_s16b(creature.get_enchant_energy_need());
    wr_s16b(static_cast<int16_t>(creature.get_recall_dungeon()));
    wr_s16b(creature.get_infravision());
    wr_byte((byte)creature.get_mimic_form());

    wr_s16b(creature.get_patron());
    wr_FlagGroup(creature.get_mutations(), wr_byte);
    wr_FlagGroup(creature.get_traits(), wr_byte);

    // Save virtues in legacy format (8 entries)
    // First, collect virtues into arrays
    Virtue vir_types[8];
    int16_t vir_values[8];
    int idx = 0;
    for (const auto &[vir_type, value] : creature.virtues) {
        if (idx < 8) {
            vir_types[idx] = vir_type;
            vir_values[idx] = value;
            idx++;
        }
    }
    // Fill remaining slots with NONE
    for (; idx < 8; idx++) {
        vir_types[idx] = Virtue::NONE;
        vir_values[idx] = 0;
    }

    // Write virtue values
    for (int i = 0; i < 8; i++) {
        wr_s16b(vir_values[i]);
    }

    // Write virtue types
    for (int i = 0; i < 8; i++) {
        wr_s16b(enum2i(vir_types[i]));
    }

    wr_s32b(int32_t(creature.incident.size()));
    for (const auto &it : creature.incident) {
        wr_s32b((int32_t)it.first);
        wr_s32b(it.second);
    }

    // Save incident_tree (string-keyed, tree-structured incidents)
    wr_s32b(int32_t(creature.incident_tree.size()));
    for (const auto &it : creature.incident_tree) {
        wr_string(it.first);
        wr_s32b(it.second);
    }

    // ELE_ATTACK / ELE_IMMUNE は wr_creature_common() の全時限効果ダンプに集約済み
    wr_u32b(creature.get_special_attack_flags());
    wr_u32b(creature.get_special_defense_flags());
    wr_byte(creature.knowledge);
    wr_bool(creature.is_autopick_autoregister());
    wr_byte(0);
    wr_byte((byte)creature.get_action());
    wr_byte(0);
    wr_bool(preserve_mode);
    wr_bool(system.is_awaiting_report_status());

    for (int i = 0; i < 12; i++) {
        wr_u32b(0L);
    }

    /* Ignore some flags */
    wr_u32b(0L);
    wr_u32b(0L);
    wr_u32b(0L);

    wr_u32b(system.get_seed_flavor());
    wr_u32b(system.get_seed_town());
    wr_u16b(system.is_panic_save_executed() ? 1 : 0);
    wr_u16b(world.total_winner);
    wr_u16b(InnerGameData::get_instance().get_no_score());
    wr_bool(creature.is_dead());
    const auto &df = DungeonFeeling::get_instance();
    wr_byte(static_cast<uint8_t>(df.get_feeling()));
    wr_s32b(creature.get_floor()->generated_turn);
    wr_s32b(df.get_turns());
    wr_s32b(world.game_turn);
    wr_s32b(world.dungeon_turn);
    wr_s32b(world.arena_start_turn);
    wr_s16b(enum2i(world.today_mon));
    wr_s16b(world.knows_daily_bounty ? 1 : 0); // 現在bool型だが、かつてモンスター種族IDを保存していた仕様に合わせる
    wr_s16b(creature.get_riding());
    wr_s16b(creature.floor_id);

    /* Save temporary preserved pets (obsolated) */
    wr_s16b(0);
    wr_u32b(world.play_time.elapsed_sec());

    // 訪問済みの町情報。TownRecords を旧来の creature.visit ビットマスク (u32) に変換して書き出す
    // (bit N = TownId N)。セーブフォーマット互換のため形式は不変。
    uint32_t visit_flags = 0;
    const auto visited_towns = TownRecords::get_instance().get_ids();
    for (auto i = 0; i < enum2i(TownId::MAX); i++) {
        if (visited_towns.has(i2enum<TownId>(i))) {
            visit_flags |= 1U << i;
        }
    }

    wr_s32b(visit_flags);
    wr_u32b(creature.count);

    // [モンスタープレイヤー] プレイヤーがモンスター化している場合の種族 ID を保存する。
    // 通常プレイヤーは PLAYER (= 0)。セーブファイルバージョン 48 以降。
    wr_s16b(enum2i(creature.get_r_idx()));
    wr_s16b(enum2i(creature.get_ap_r_idx()));
}
