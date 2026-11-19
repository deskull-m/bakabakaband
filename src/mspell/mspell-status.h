#pragma once

#include "system/angband.h"
#include <fmt/base.h>
#include <string_view>

struct MonsterSpellResult;

class CreatureEntity;

/*!
 * @brief プレイヤーへの状態異常の呪文のメッセージ
 * @details blind と not_blind の書式の引数 {0} は、唱えたモンスターの名前 (先頭を大文字にする)
 */
struct mspell_cast_msg_bad_status_to_player {
    using Format = fmt::format_string<std::string_view>;
    mspell_cast_msg_bad_status_to_player(Format blind, Format not_blind, std::string_view resist, std::string_view saved_throw);
    Format blind; /*盲目時*/
    Format not_blind; /*非盲目時*/
    std::string_view resist; /*耐性有*/
    std::string_view saved_throw; /*抵抗時*/
};

/*!
 * @brief モンスターへの状態異常の呪文のメッセージ
 * @details default_msg の書式の引数は、{0} が唱えたモンスターの名前 (先頭を大文字にする)、{1} が対象のモンスターの名前。
 * それ以外の書式の引数 {0} は、対象のモンスターの名前 (先頭を大文字にする)
 */
struct mspell_cast_msg_bad_status_to_monster {
    using Format = fmt::format_string<std::string_view>;
    using DefaultFormat = fmt::format_string<std::string_view, std::string_view>;
    mspell_cast_msg_bad_status_to_monster(DefaultFormat default_msg, Format resist, Format saved_throw, Format success);
    DefaultFormat default_msg; /*通常時*/
    Format resist; /*耐性有*/
    Format saved_throw; /*抵抗時*/
    Format success; /*成功時*/
};

void spell_badstatus_message_to_player(CreatureEntity &creature, MONSTER_IDX m_idx, const mspell_cast_msg_bad_status_to_player &msgs, bool resist,
    bool saved_throw);
void spell_badstatus_message_to_mons(CreatureEntity &creature, MONSTER_IDX m_idx, MONSTER_IDX t_idx, const mspell_cast_msg_bad_status_to_monster &msgs, bool resist,
    bool saved_throw);
MonsterSpellResult spell_RF5_DRAIN_MANA(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF5_MIND_BLAST(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF5_BRAIN_SMASH(CreatureEntity &creature, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF5_SCARE(MONSTER_IDX m_idx, CreatureEntity &creature, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF5_BLIND(MONSTER_IDX m_idx, CreatureEntity &creature, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF5_CONF(MONSTER_IDX m_idx, CreatureEntity &creature, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF5_HOLD(MONSTER_IDX m_idx, CreatureEntity &creature, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_HASTE(CreatureEntity &creature, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF5_SLOW(MONSTER_IDX m_idx, CreatureEntity &creature, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_HEAL(CreatureEntity &creature, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_INVULNER(CreatureEntity &creature, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_FORGET(CreatureEntity &creature, MONSTER_IDX m_idx);
