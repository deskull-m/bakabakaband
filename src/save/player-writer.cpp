#include "save/player-writer.h"
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
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/dungeon/dungeon-record.h"
#include "system/floor-type-definition.h"
#include "system/inner-game-data.h"
#include "system/player-type-definition.h"
#include "timed-effect/timed-effects.h"
#include "world/world.h"
#include <variant>

/*!
 * @brief セーブデータに領域情報を書き込む / Write player realms
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void wr_relams(PlayerType *player_ptr)
{
    PlayerRealm pr(player_ptr);
    if (PlayerClass(player_ptr).equals(PlayerClassType::ELEMENTALIST)) {
        wr_byte((byte)player_ptr->element);
    } else {
        wr_byte((byte)pr.realm1().to_enum());
    }
    wr_byte((byte)pr.realm2().to_enum());
}

/*!
 * @brief セーブデータにプレイヤー情報を書き込む / Write some "player" info
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void wr_player(PlayerType *player_ptr)
{
    wr_string(player_ptr->name);
    wr_string(player_ptr->died_from);
    wr_string(player_ptr->last_message);

    save_quick_start();
    for (int i = 0; i < 4; i++) {
        wr_string(player_ptr->history[i]);
    }

    wr_byte((byte)player_ptr->prace);
    wr_byte((byte)player_ptr->pclass);
    wr_byte((byte)player_ptr->ppersonality);
    wr_byte((byte)player_ptr->psex);
    wr_relams(player_ptr);
    wr_byte(0);

    wr_byte((byte)player_ptr->hit_dice.sides);
    wr_u16b(player_ptr->expfact);

    wr_s32b(player_ptr->death_count);
    wr_s16b(player_ptr->age);
    wr_s16b(player_ptr->ht);
    wr_s16b(player_ptr->wt);

    for (int i = 0; i < A_MAX; ++i) {
        wr_s16b(player_ptr->stat_max[i]);
    }

    for (int i = 0; i < A_MAX; ++i) {
        wr_s16b(player_ptr->stat_max_max[i]);
    }

    for (int i = 0; i < A_MAX; ++i) {
        wr_s16b(player_ptr->stat_cur[i]);
    }

    for (int i = 0; i < 12; ++i) {
        wr_s16b(0);
    }

    wr_u32b(player_ptr->au);
    wr_u32b(player_ptr->max_exp);
    wr_u32b(player_ptr->max_max_exp);
    wr_u32b(player_ptr->exp);
    wr_u32b(player_ptr->exp_frac);
    wr_s16b(player_ptr->lev);

    for (int i = 0; i < 64; i++) {
        wr_s16b(player_ptr->spell_exp[i]);
    }

    for (auto tval : TV_WEAPON_RANGE) {
        for (int j = 0; j < 64; j++) {
            wr_s16b(player_ptr->weapon_exp[tval][j]);
        }
    }

    for (auto i : PLAYER_SKILL_KIND_TYPE_RANGE) {
        wr_s16b(player_ptr->skill_exp[i]);
    }
    for (auto i = 0U; i < MAX_SKILLS - PLAYER_SKILL_KIND_TYPE_RANGE.size(); ++i) {
        // resreved skills
        wr_s16b(0);
    }

    std::visit(PlayerClassSpecificDataWriter(), player_ptr->class_specific_data);

    wr_byte(static_cast<uint8_t>(InnerGameData::get_instance().get_start_race()));
    wr_s32b(player_ptr->old_race1);
    wr_s32b(player_ptr->old_race2);
    wr_s16b(player_ptr->old_realm);

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

    wr_s16b(player_ptr->town_num);
    const auto &entries = ArenaEntryList::get_instance();
    wr_s16b(static_cast<int16_t>(entries.get_current_entry()));
    const auto defeated_entry = entries.get_defeated_entry();
    wr_s16b(static_cast<int16_t>(defeated_entry.value_or(-1)));
    wr_s16b(player_ptr->current_floor_ptr->inside_arena);
    wr_s16b(enum2i(player_ptr->current_floor_ptr->quest_number));
    wr_s16b(AngbandSystem::get_instance().is_phase_out());
    wr_byte(world.get_arena());
    wr_byte(0); /* Unused */

    wr_s16b((int16_t)player_ptr->oldpx);
    wr_s16b((int16_t)player_ptr->oldpy);

    wr_s16b(0);
    wr_s32b(player_ptr->mhp);
    wr_s32b(player_ptr->chp);
    wr_u32b(player_ptr->chp_frac);
    wr_s32b(player_ptr->msp);
    wr_s32b(player_ptr->csp);
    wr_u32b(player_ptr->csp_frac);
    wr_s16b(player_ptr->max_plv);

    byte tmp8u = (byte)dungeons_info.size();
    wr_byte(tmp8u);
    for (int i = 0; i < tmp8u; i++) {
        wr_s16b((int16_t)max_dlv[i]);
    }

    wr_s16b(0);
    wr_s16b(0);
    wr_s16b(0);
    wr_s16b(0);
    wr_s16b(player_ptr->prestige);

    auto effects = player_ptr->effects();
    wr_s16b(0); /* old "rest" */
    wr_s16b(effects->blindness().current());
    wr_s16b(effects->paralysis().current());
    wr_s16b(effects->confusion().current());
    wr_s16b(player_ptr->food);
    wr_s16b(0); /* old "food_digested" */
    wr_s16b(0); /* old "protection" */
    wr_s16b(player_ptr->energy_need);
    wr_s16b(player_ptr->enchant_energy_need);
    wr_s16b(effects->acceleration().current());
    wr_s16b(effects->deceleration().current());
    wr_s16b(effects->fear().current());
    wr_s16b(effects->cut().current());
    wr_s16b(effects->stun().current());
    wr_s16b(effects->poison().current());
    wr_s16b(effects->hallucination().current());
    wr_s16b(effects->protection().current());
    wr_s16b(player_ptr->invuln);
    wr_s16b(player_ptr->ult_res);
    wr_s16b(player_ptr->hero);
    wr_s16b(player_ptr->shero);
    wr_s16b(player_ptr->shield);
    wr_s16b(player_ptr->blessed);
    wr_s16b(player_ptr->tim_invis);
    wr_s16b(player_ptr->word_recall);
    wr_s16b(player_ptr->recall_dungeon);
    wr_s16b(player_ptr->alter_reality);
    wr_s16b(player_ptr->see_infra);
    wr_s16b(player_ptr->tim_infra);
    wr_s16b(player_ptr->oppose_fire);
    wr_s16b(player_ptr->oppose_cold);
    wr_s16b(player_ptr->oppose_acid);
    wr_s16b(player_ptr->oppose_elec);
    wr_s16b(player_ptr->oppose_pois);
    wr_s16b(player_ptr->tsuyoshi);
    wr_s16b(player_ptr->tim_esp);
    wr_s16b(player_ptr->wraith_form);
    wr_s16b(player_ptr->resist_magic);
    wr_s16b(player_ptr->tim_regen);
    wr_s16b(player_ptr->tim_pass_wall);
    wr_s16b(player_ptr->tim_stealth);
    wr_s16b(player_ptr->tim_levitation);
    wr_s16b(player_ptr->tim_sh_touki);
    wr_s16b(player_ptr->lightspeed);
    wr_s16b(player_ptr->tsubureru);
    wr_s16b(player_ptr->magicdef);
    wr_s16b(player_ptr->tim_res_nether);
    wr_s16b(player_ptr->tim_res_time);
    wr_byte((byte)player_ptr->mimic_form);
    wr_s16b(player_ptr->tim_mimic);
    wr_s16b(player_ptr->tim_sh_fire);
    wr_s16b(player_ptr->tim_sh_holy);
    wr_s16b(player_ptr->tim_eyeeye);

    wr_s16b(player_ptr->tim_reflect);
    wr_s16b(player_ptr->multishadow);
    wr_s16b(player_ptr->dustrobe);

    wr_s16b(player_ptr->chaos_patron);
    wr_FlagGroup(player_ptr->muta, wr_byte);
    wr_FlagGroup(player_ptr->trait, wr_byte);

    for (int i = 0; i < 8; i++) {
        wr_s16b(player_ptr->virtues[i]);
    }

    for (int i = 0; i < 8; i++) {
        wr_s16b(enum2i(player_ptr->vir_types[i]));
    }

    wr_s32b(int32_t(player_ptr->incident.size()));
    std::map<INCIDENT, int32_t>::iterator it;
    for (it = player_ptr->incident.begin(); it != player_ptr->incident.end(); it++) {
        wr_s32b((int32_t)it->first);
        wr_s32b(it->second);
    }

    wr_s16b(player_ptr->ele_attack);
    wr_u32b(player_ptr->special_attack);
    wr_s16b(player_ptr->ele_immune);
    wr_u32b(player_ptr->special_defense);
    wr_byte(player_ptr->knowledge);
    wr_bool(player_ptr->autopick_autoregister);
    wr_byte(0);
    wr_byte((byte)player_ptr->action);
    wr_byte(0);
    wr_bool(preserve_mode);
    wr_bool(player_ptr->wait_report_score);

    for (int i = 0; i < 12; i++) {
        wr_u32b(0L);
    }

    /* Ignore some flags */
    wr_u32b(0L);
    wr_u32b(0L);
    wr_u32b(0L);

    const auto &system = AngbandSystem::get_instance();
    wr_u32b(system.get_seed_flavor());
    wr_u32b(system.get_seed_town());
    wr_u16b(player_ptr->panic_save);
    wr_u16b(world.total_winner);
    wr_u16b(world.noscore);
    wr_bool(player_ptr->is_dead);
    wr_byte(player_ptr->feeling);
    wr_s32b(player_ptr->current_floor_ptr->generated_turn);
    wr_s32b(player_ptr->feeling_turn);
    wr_s32b(world.game_turn);
    wr_s32b(world.dungeon_turn);
    wr_s32b(world.arena_start_turn);
    wr_s16b(enum2i(world.today_mon));
    wr_s16b(world.knows_daily_bounty ? 1 : 0); // 現在bool型だが、かつてモンスター種族IDを保存していた仕様に合わせる
    wr_s16b(player_ptr->riding);
    wr_s16b(player_ptr->floor_id);

    /* Save temporary preserved pets (obsolated) */
    wr_s16b(0);
    wr_u32b(world.play_time);
    wr_s32b(player_ptr->visit);
    wr_u32b(player_ptr->count);
}
