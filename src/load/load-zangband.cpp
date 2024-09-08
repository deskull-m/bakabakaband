#include "load/load-zangband.h"
#include "avatar/avatar.h"
#include "dungeon/quest.h"
#include "game-option/option-flags.h"
#include "info-reader/fixed-map-parser.h"
#include "load/load-util.h"
#include "market/bounty.h"
#include "pet/pet-util.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "player/attack-defense-types.h"
#include "player/patron.h"
#include "player/player-personality.h"
#include "player/player-realm.h"
#include "player/player-skill.h"
#include "realm/realm-types.h"
#include "spell/spells-status.h"
#include "system/building-type-definition.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/inner-game-data.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/system-variables.h"
#include "world/world.h"

void load_zangband_options(void)
{
    if (g_option_flags[5] & (0x00000001U << 4)) {
        g_option_flags[5] &= ~(0x00000001U << 4);
    } else {
        g_option_flags[5] |= (0x00000001U << 4);
    }

    if (g_option_flags[2] & (0x00000001U << 5)) {
        g_option_flags[2] &= ~(0x00000001U << 5);
    } else {
        g_option_flags[2] |= (0x00000001U << 5);
    }

    if (g_option_flags[4] & (0x00000001U << 5)) {
        g_option_flags[4] &= ~(0x00000001U << 5);
    } else {
        g_option_flags[4] |= (0x00000001U << 5);
    }

    if (g_option_flags[5] & (0x00000001U << 0)) {
        g_option_flags[5] &= ~(0x00000001U << 0);
    } else {
        g_option_flags[5] |= (0x00000001U << 0);
    }

    if (g_option_flags[5] & (0x00000001U << 12)) {
        g_option_flags[5] &= ~(0x00000001U << 12);
    } else {
        g_option_flags[5] |= (0x00000001U << 12);
    }

    if (g_option_flags[1] & (0x00000001U << 0)) {
        g_option_flags[1] &= ~(0x00000001U << 0);
    } else {
        g_option_flags[1] |= (0x00000001U << 0);
    }

    if (g_option_flags[1] & (0x00000001U << 18)) {
        g_option_flags[1] &= ~(0x00000001U << 18);
    } else {
        g_option_flags[1] |= (0x00000001U << 18);
    }

    if (g_option_flags[1] & (0x00000001U << 19)) {
        g_option_flags[1] &= ~(0x00000001U << 19);
    } else {
        g_option_flags[1] |= (0x00000001U << 19);
    }

    if (g_option_flags[5] & (0x00000001U << 3)) {
        g_option_flags[1] &= ~(0x00000001U << 3);
    } else {
        g_option_flags[5] |= (0x00000001U << 3);
    }
}

void set_zangband_realm(PlayerType *player_ptr)
{
    PlayerRealm pr(player_ptr);
    const auto realm1 = enum2i(pr.realm1().to_enum());
    if (realm1 == 9) {
        pr.set(REALM_MUSIC);
    }

    if (realm1 == 10) {
        pr.set(REALM_HISSATSU);
    }
}

void set_zangband_skill(PlayerType *player_ptr)
{
    if (!PlayerClass(player_ptr).equals(PlayerClassType::BEASTMASTER)) {
        player_ptr->skill_exp[PlayerSkillKindType::RIDING] /= 2;
    }

    player_ptr->skill_exp[PlayerSkillKindType::RIDING] = std::min(player_ptr->skill_exp[PlayerSkillKindType::RIDING], class_skills_info[enum2i(player_ptr->pclass)].s_max[PlayerSkillKindType::RIDING]);
}

void set_zangband_bounty_uniques(PlayerType *player_ptr)
{
    determine_bounty_uniques(player_ptr);
    const auto &monraces = MonraceList::get_instance();
    for (auto &[monrace_id, is_achieved] : AngbandWorld::get_instance().bounties) {
        /* Is this bounty unique already dead? */
        if (monraces.get_monrace(monrace_id).max_num == 0) {
            is_achieved = true;
        }
    }
}

void set_zangband_mimic(PlayerType *player_ptr)
{
    player_ptr->tim_res_time = 0;
    player_ptr->mimic_form = MimicKindType::NONE;
    player_ptr->tim_mimic = 0;
    player_ptr->tim_sh_fire = 0;
}

void set_zangband_holy_aura(PlayerType *player_ptr)
{
    player_ptr->tim_sh_holy = 0;
    player_ptr->tim_eyeeye = 0;
}

void set_zangband_reflection(PlayerType *player_ptr)
{
    player_ptr->tim_reflect = 0;
    player_ptr->multishadow = 0;
    player_ptr->dustrobe = 0;
}

void rd_zangband_dungeon()
{
    max_dlv[DUNGEON_ANGBAND] = rd_s16b();
}

void set_zangband_game_turns(PlayerType *player_ptr)
{
    player_ptr->current_floor_ptr->generated_turn /= 2;
    player_ptr->feeling_turn /= 2;
    auto &world = AngbandWorld::get_instance();
    world.game_turn /= 2;
    world.dungeon_turn /= 2;
}

void set_zangband_gambling_monsters(int i)
{
    mon_odds[i] = rd_s16b();
}

void set_zangband_special_attack(PlayerType *player_ptr)
{
    if (rd_byte() != 0) {
        player_ptr->special_attack = ATTACK_CONFUSE;
    }

    player_ptr->ele_attack = 0;
}

void set_zangband_special_defense(PlayerType *player_ptr)
{
    player_ptr->ele_immune = 0;
    player_ptr->special_defense = 0;
}

void set_zangband_action(PlayerType *player_ptr)
{
    if (rd_byte() != 0) {
        player_ptr->action = ACTION_LEARN;
    }
}

void set_zangband_visited_towns(PlayerType *player_ptr)
{
    strip_bytes(4);
    player_ptr->visit = 1L;
}

void set_zangband_quest(PlayerType *player_ptr, QuestType *const q_ptr, const QuestId loading_quest_index, const QuestId old_inside_quest)
{
    if (q_ptr->flags & QUEST_FLAG_PRESET) {
        q_ptr->dungeon = 0;
        return;
    }

    init_flags = INIT_ASSIGN;
    player_ptr->current_floor_ptr->quest_number = loading_quest_index;
    parse_fixed_map(player_ptr, QUEST_DEFINITION_LIST, 0, 0, 0, 0);
    player_ptr->current_floor_ptr->quest_number = old_inside_quest;
}

void set_zangband_learnt_spells(PlayerType *player_ptr)
{
    player_ptr->learned_spells = 0;
    for (int i = 0; i < 64; i++) {
        if ((i < 32) ? (player_ptr->spell_learned1 & (1UL << i)) : (player_ptr->spell_learned2 & (1UL << (i - 32)))) {
            player_ptr->learned_spells++;
        }
    }
}

void set_zangband_pet(PlayerType *player_ptr)
{
    player_ptr->pet_extra_flags = 0;
    if (rd_byte() != 0) {
        player_ptr->pet_extra_flags |= PF_OPEN_DOORS;
    }

    if (rd_byte() != 0) {
        player_ptr->pet_extra_flags |= PF_PICKUP_ITEMS;
    }

    if (rd_byte() != 0) {
        player_ptr->pet_extra_flags |= PF_TELEPORT;
    }

    if (rd_byte() != 0) {
        player_ptr->pet_extra_flags |= PF_ATTACK_SPELL;
    }

    if (rd_byte() != 0) {
        player_ptr->pet_extra_flags |= PF_SUMMON_SPELL;
    }

    if (rd_byte() != 0) {
        player_ptr->pet_extra_flags |= PF_BALL_SPELL;
    }
}
