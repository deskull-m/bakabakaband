#include "melee/melee-spell.h"
#include "core/disturbance.h"
#include "melee/melee-spell-flags-checker.h"
#include "melee/melee-spell-util.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "mspell/assign-monster-spell.h"
#include "mspell/mspell-checker.h"
#include "mspell/mspell-result.h"
#include "mspell/mspell-util.h"
#include "player-base/player-class.h"
#include "player-info/mane-data-type.h"
#include "spell-realm/spells-hex.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/redrawing-flags-updater.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "world/world.h"
#ifdef JP
#else
#include "monster/monster-description-types.h"
#endif

#define RF4_SPELL_SIZE 32
#define RF5_SPELL_SIZE 32
#define RF6_SPELL_SIZE 32

static bool try_melee_spell(CreatureEntity &creature, melee_spell_type *ms_ptr)
{
    if (spell_is_inate(ms_ptr->thrown_spell) || (!ms_ptr->in_no_magic_dungeon && (!ms_ptr->m_ptr->get_remaining_stun() || one_in_(2)))) {
        return false;
    }

    disturb(creature, true, true);
    if (ms_ptr->see_m) {
        msg_format(_(" %s^は呪文を唱えようとしたが失敗した。", "%s^ tries to cast a spell, but fails."), ms_ptr->m_name.data());
    }

    return true;
}

static bool disturb_melee_spell(CreatureEntity &creature, melee_spell_type *ms_ptr)
{
    if (spell_is_inate(ms_ptr->thrown_spell) || !SpellHex(creature).check_hex_barrier(ms_ptr->m_idx, HEX_ANTI_MAGIC)) {
        return false;
    }

    if (ms_ptr->see_m) {
        msg_format(_("反魔法バリアが%s^の呪文をかき消した。", "Anti magic barrier cancels the spell which %s^ casts."), ms_ptr->m_name.data());
    }

    return true;
}

static void process_special_melee_spell(CreatureEntity &creature, melee_spell_type *ms_ptr)
{
    CreatureClass pc(creature);
    bool is_special_magic = ms_ptr->m_ptr->get_monster_profile().ml;
    is_special_magic &= ms_ptr->maneable;
    is_special_magic &= AngbandWorld::get_instance().timewalk_m_idx == 0;
    is_special_magic &= !creature.is_blind();
    is_special_magic &= pc.equals(PlayerClassType::IMITATOR);
    is_special_magic &= ms_ptr->thrown_spell != MonsterAbilityType::SPECIAL;
    if (!is_special_magic) {
        return;
    }

    auto mane_data = pc.get_specific_data<mane_data_type>();

    if (mane_data->mane_list.size() == MAX_MANE) {
        mane_data->mane_list.pop_front();
    }

    mane_data->mane_list.push_back({ ms_ptr->thrown_spell, ms_ptr->dam });
    mane_data->new_mane = true;
    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::IMITATION);
}

static void process_rememberance(melee_spell_type *ms_ptr)
{
    if (!ms_ptr->can_remember) {
        return;
    }

    ms_ptr->r_ptr->r_ability_flags.set(ms_ptr->thrown_spell);

    if (ms_ptr->r_ptr->r_cast_spell < MAX_UCHAR) {
        ms_ptr->r_ptr->r_cast_spell++;
    }
}

/*!
 * @brief モンスターが敵モンスターに特殊能力を使う処理のメインルーチン /
 * Monster tries to 'cast a spell' (or breath, etc) at another monster.
 * @param creature クリーチャーへの参照
 * @param m_idx 術者のモンスターID
 * @return 実際に特殊能力を使った場合TRUEを返す
 * @details
 * The creature is only disturbed if able to be affected by the spell.
 */
bool monst_spell_monst(CreatureEntity &creature, MONSTER_IDX m_idx)
{
    melee_spell_type tmp_ms(creature, m_idx);
    melee_spell_type *ms_ptr = &tmp_ms;
    if (!check_melee_spell_set(creature, ms_ptr)) {
        return false;
    }

    ms_ptr->m_name = monster_desc(creature, *ms_ptr->m_ptr, 0x00);
    ms_ptr->thrown_spell = rand_choice(ms_ptr->spells);
    if (ms_ptr->m_ptr->is_riding()) {
        disturb(creature, true, true);
    }

    if (try_melee_spell(creature, ms_ptr) || disturb_melee_spell(creature, ms_ptr)) {
        return true;
    }

    ms_ptr->can_remember = is_original_ap_and_seen(creature, *ms_ptr->m_ptr);
    const auto res = monspell_to_monster(creature, ms_ptr->thrown_spell, ms_ptr->y, ms_ptr->x, m_idx, ms_ptr->target_idx, false);
    if (!res.valid) {
        return false;
    }

    ms_ptr->dam = res.dam;
    process_special_melee_spell(creature, ms_ptr);
    process_rememberance(ms_ptr);
    if (creature.is_dead() && (ms_ptr->r_ptr->r_deaths < MAX_SHORT) && !creature.get_floor()->inside_arena) {
        ms_ptr->r_ptr->r_deaths++;
    }

    return true;
}
