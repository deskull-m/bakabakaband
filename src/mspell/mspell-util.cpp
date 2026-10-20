#include "mspell/mspell-util.h"
#include "core/disturbance.h"
#include "floor/geometry.h"
#include "grid/grid.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <fmt/format.h>

// 書式は空文字列にしておき、使う前に書き換える
mspell_cast_msg::mspell_cast_msg()
    : mspell_cast_msg("", "", "", "")
{
}

mspell_cast_msg::mspell_cast_msg(MSpellMessageFormat to_player_true, MSpellMessageFormat to_mons_true, MSpellMessageFormat to_player_false, MSpellMessageFormat to_mons_false)
    : to_player_true(to_player_true)
    , to_mons_true(to_mons_true)
    , to_player_false(to_player_false)
    , to_mons_false(to_mons_false)
{
}

mspell_cast_msg_blind::mspell_cast_msg_blind()
    : mspell_cast_msg_blind("", "", "")
{
}

mspell_cast_msg_blind::mspell_cast_msg_blind(MSpellMessageFormat blind, MSpellMessageFormat to_player, MSpellMessageFormat to_mons)
    : blind(blind)
    , to_player(to_player)
    , to_mons(to_mons)
{
}

mspell_cast_msg_simple::mspell_cast_msg_simple()
    : mspell_cast_msg_simple("", "")
{
}

mspell_cast_msg_simple::mspell_cast_msg_simple(MSpellMessageFormat to_player, MSpellMessageFormat to_mons)
    : to_player(to_player)
    , to_mons(to_mons)
{
}

/*!
 * @brief モンスターが呪文を唱えるときのメッセージを出力する
 * @param creature クリーチャーへの参照
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param fmt メッセージの書式
 */
static void print_monspell_message(CreatureEntity &creature, MONSTER_IDX m_idx, MONSTER_IDX t_idx, MSpellMessageFormat fmt)
{
    const auto &monster = creature.get_floor()->get_monster(m_idx);
    const auto m_name = monster_name(creature, m_idx);
    const auto m_name_upcased = str_upcase_first(m_name);
    const auto t_name = monster_name(creature, t_idx);
    const auto t_name_upcased = str_upcase_first(t_name);
    const auto m_poss = monster_desc(creature, monster, MD_PRON_VISIBLE | MD_POSSESSIVE);
    msg_print(fmt::vformat(fmt.get(), fmt::make_format_args(m_name_upcased, t_name, m_name, m_poss, t_name_upcased)));
}

/*!
 * @brief プレイヤーがモンスターを見ることができるかの判定 /
 * @param creature クリーチャーへの参照
 * @param m_idx モンスターID
 * @return プレイヤーがモンスターを見ることができるならTRUE、そうでなければFALSEを返す。
 */
bool see_monster(CreatureEntity &creature, MONSTER_IDX m_idx)
{
    const auto &monster = creature.get_floor()->get_monster(m_idx);
    return is_seen(creature, monster);
}

/*!
 * @brief モンスター2体がプレイヤーの近くに居るかの判定 /
 * @param floor フロアへの参照
 * @param m_idx モンスターID一体目
 * @param t_idx モンスターID二体目
 * @return モンスター2体のどちらかがプレイヤーの近くに居ればTRUE、どちらも遠ければFALSEを返す。
 */
bool monster_near_player(const CreatureEntity &creature, MONSTER_IDX m_idx, MONSTER_IDX t_idx)
{
    const auto &floor = *creature.get_floor();
    const auto p_pos = creature.get_position();
    const auto &monster = floor.get_monster(m_idx);
    const auto &monster_target = floor.get_monster(t_idx);
    return (Grid::calc_distance(p_pos, monster.get_position()) <= MAX_PLAYER_SIGHT) || (Grid::calc_distance(p_pos, monster_target.get_position()) <= MAX_PLAYER_SIGHT);
}

/*!
 * @brief モンスターが呪文行使する際のメッセージを処理する汎用関数 /
 * @param creature クリーチャーへの参照
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param msgs メッセージの構造体
 * @param msg_flag_aux メッセージを分岐するためのフラグ
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return メッセージを表示した場合trueを返す。
 */
bool monspell_message_base(CreatureEntity &creature, MONSTER_IDX m_idx, MONSTER_IDX t_idx, const mspell_cast_msg &msgs, bool msg_flag_aux, int target_type)
{
    auto &floor = *creature.get_floor();
    const auto known = monster_near_player(creature, m_idx, t_idx);
    const auto see_either = see_monster(creature, m_idx) || see_monster(creature, t_idx);
    const auto mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    const auto mon_to_player = (target_type == MONSTER_TO_PLAYER);
    if (mon_to_mon && known && !see_either) {
        floor.monster_noise = true;
    }

    if (!mon_to_player && !(mon_to_mon && known && see_either)) {
        return false;
    }

    disturb(creature, true, true);
    if (mon_to_player) {
        print_monspell_message(creature, m_idx, t_idx, msg_flag_aux ? msgs.to_player_true : msgs.to_player_false);
    } else {
        print_monspell_message(creature, m_idx, t_idx, msg_flag_aux ? msgs.to_mons_true : msgs.to_mons_false);
    }

    return true;
}

/*!
 * @brief モンスターが呪文行使する際のメッセージを処理する汎用関数。盲目時と通常時のメッセージを切り替える。 /
 * @param creature クリーチャーへの参照
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param msgs メッセージの構造体
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return メッセージを表示した場合trueを返す。
 */
bool monspell_message(CreatureEntity &creature, MONSTER_IDX m_idx, MONSTER_IDX t_idx, const mspell_cast_msg_blind &msgs, int target_type)
{
    mspell_cast_msg mcm(msgs.blind, msgs.blind, msgs.to_player, msgs.to_mons);
    const auto is_blind = creature.is_blind();
    return monspell_message_base(creature, m_idx, t_idx, mcm, is_blind, target_type);
}

/*!
 * @brief モンスターが呪文行使する際のメッセージを処理する汎用関数。対モンスターと対プレイヤーのメッセージを切り替える。 /
 * @param creature クリーチャーへの参照
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param msgs メッセージの構造体
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 */
void simple_monspell_message(CreatureEntity &creature, MONSTER_IDX m_idx, MONSTER_IDX t_idx, const mspell_cast_msg_simple &msgs, int target_type)
{
    mspell_cast_msg mcm(msgs.to_player, msgs.to_mons, msgs.to_player, msgs.to_mons);
    const auto is_blind = creature.is_blind();
    monspell_message_base(creature, m_idx, t_idx, mcm, is_blind, target_type);
}
