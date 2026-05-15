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
#include "system/inner-game-data.h"
#include "timed-effect/timed-effects.h"
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

    wr_string(creature.name.data());
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
    wr_s16b(creature.age);
    wr_s16b(creature.ht);
    wr_s16b(creature.wt);

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

    for (int i = 0; i < A_MAX; ++i) {
        wr_s16b(creature.stat_max[i]);
    }

    for (int i = 0; i < A_MAX; ++i) {
        wr_s16b(creature.stat_max_max[i]);
    }

    for (int i = 0; i < A_MAX; ++i) {
        wr_s16b(creature.stat_cur[i]);
    }

    for (int i = 0; i < 12; ++i) {
        wr_s16b(0);
    }

    wr_u32b(creature.au);
    wr_u32b(creature.max_exp);
    wr_u32b(creature.max_max_exp);
    wr_u32b(creature.exp);
    wr_u32b(creature.exp_frac);
    wr_s16b(creature.level);

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
    wr_s32b(creature.old_race1);
    wr_s32b(creature.old_race2);
    wr_s16b(creature.old_realm);

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

    wr_s16b(creature.town_num);
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
    wr_s32b(creature.maxhp);
    wr_s32b(creature.hp);
    wr_u32b(creature.hp_frac);
    wr_s32b(creature.dealt_damage); // セーブファイルバージョン35以降で与ダメージ蓄積を保存
    wr_s32b(creature.msp);
    wr_s32b(creature.csp);
    wr_u32b(creature.csp_frac);
    wr_s16b(creature.max_plv);

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
    wr_s16b(creature.prestige);

    auto effects = creature.effects();
    wr_s16b(0); /* old "rest" */
    wr_s16b(effects->blindness().current());
    wr_s16b(effects->paralysis().current());
    wr_s16b(effects->confusion().current());
    wr_s16b(creature.food);
    wr_s16b(0); /* old "food_digested" */
    wr_s16b(0); /* old "protection" */
    wr_s16b(creature.energy_need);
    wr_s16b(creature.enchant_energy_need);
    wr_s16b(effects->acceleration().current());
    wr_s16b(effects->deceleration().current());
    wr_s16b(effects->fear().current());
    wr_s16b(effects->cut().current());
    wr_s16b(effects->stun().current());
    wr_s16b(effects->poison().current());
    wr_s16b(effects->hallucination().current());
    wr_s16b(effects->protection().current());
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::INVULNERABILITY));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::ULTIMATE_RESISTANCE));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::HERO));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::BERSERK));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::SHIELD));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::BLESSED));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::TIM_INVIS));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::WORD_RECALL));
    wr_s16b(static_cast<int16_t>(creature.recall_dungeon));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::ALTER_REALITY));
    wr_s16b(creature.see_infra);
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::TIM_INFRA));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::OPPOSE_FIRE));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::OPPOSE_COLD));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::OPPOSE_ACID));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::OPPOSE_ELEC));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::OPPOSE_POIS));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::TSUYOSHI));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::TIM_ESP));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::WRAITH_FORM));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::RESIST_MAGIC));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::TIM_REGEN));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::TIM_PASS_WALL));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::TIM_STEALTH));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::TIM_LEVITATION));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::TIM_SH_TOUKI));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::LIGHTSPEED));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::TSUBURERU));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::MAGICDEF));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::TIM_RES_NETHER));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::TIM_RES_LITE));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::TIM_RES_DARK));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::TIM_RES_FEAR));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::TIM_RES_TIME));
    wr_byte((byte)creature.get_mimic_form());
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::TIM_MIMIC));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::TIM_SH_FIRE));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::TIM_SH_HOLY));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::TIM_EYEEYE));

    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::TIM_REFLECT));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::MULTISHADOW));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::DUSTROBE));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::TIM_EMISSION));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::TIM_EXORCISM));
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::TIM_IMM_DARK));

    wr_s16b(creature.patron);
    wr_FlagGroup(creature.muta, wr_byte);
    wr_FlagGroup(creature.trait, wr_byte);

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

    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::ELE_ATTACK));
    wr_u32b(creature.special_attack);
    wr_s16b(creature.get_timed_effect(CreatureTimedEffect::ELE_IMMUNE));
    wr_u32b(creature.special_defense);
    wr_byte(creature.knowledge);
    wr_bool(creature.autopick_autoregister);
    wr_byte(0);
    wr_byte((byte)creature.action);
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
    wr_u16b(world.noscore);
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
    wr_s32b(creature.visit);
    wr_u32b(creature.count);
}
