#include "mspell/mspell-attack-util.h"
#include "monster-race/race-ability-flags.h"
#include "realm/realm-types.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"

namespace {
/*!
 * @brief 魔法領域に対応する MonsterAbilityType 群を詠唱能力へ加える (提案C6)
 * @details realm_abilities が指定されたモンスターの詠唱時に、その realm 由来の
 *          モンスター能力を ability_flags に OR-in する。既存 mspell 経路
 *          (自動ターゲット・MP消費(C4)・耐性・AI) をそのまま利用する。
 *          写像は保守的な初期セット。バランス調整・拡張はこの表で行う。
 *          MUSIC / HISSATSU / HEX / NONE は現状未マッピング。
 */
void add_realm_granted_abilities(EnumClassFlagGroup<MonsterAbilityType> &flags, RealmType realm)
{
    using Ma = MonsterAbilityType;
    switch (realm) {
    case RealmType::LIFE:
        flags.set({ Ma::HEAL, Ma::CAUSE_2, Ma::CAUSE_3 });
        break;
    case RealmType::SORCERY:
        flags.set({ Ma::SLOW, Ma::CONF, Ma::SCARE, Ma::BLINK, Ma::TELE_AWAY });
        break;
    case RealmType::NATURE:
        flags.set({ Ma::BA_ELEC, Ma::BA_COLD, Ma::BO_FIRE });
        break;
    case RealmType::CHAOS:
        flags.set({ Ma::BA_CHAO, Ma::BA_FIRE, Ma::BR_CHAO, Ma::CONF });
        break;
    case RealmType::DEATH:
        flags.set({ Ma::CAUSE_3, Ma::CAUSE_4, Ma::BO_NETH, Ma::BA_NETH, Ma::DRAIN_MANA });
        break;
    case RealmType::TRUMP:
        flags.set({ Ma::BLINK, Ma::TPORT, Ma::TELE_TO, Ma::S_MONSTER, Ma::S_KIN });
        break;
    case RealmType::ARCANE:
        flags.set({ Ma::BO_MANA, Ma::BA_MANA, Ma::MIND_BLAST });
        break;
    case RealmType::CRAFT:
        flags.set({ Ma::HASTE, Ma::HEAL });
        break;
    case RealmType::DAEMON:
        flags.set({ Ma::BA_FIRE, Ma::BR_FIRE, Ma::BA_NETH, Ma::S_DEMON });
        break;
    case RealmType::CRUSADE:
        flags.set({ Ma::BA_LITE, Ma::CAUSE_3, Ma::SCARE });
        break;
    default:
        break;
    }
}
}

msa_type::msa_type(CreatureEntity &creature, MONSTER_IDX m_idx)
    : m_idx(m_idx)
    , m_ptr(&creature.get_floor()->get_monster(m_idx))
    , x(creature.x)
    , y(creature.y)
    , do_spell(DO_SPELL_NONE)
    , thrown_spell(MonsterAbilityType::MAX)
{
    this->monrace = this->m_ptr->get_monrace_shared();
    this->no_inate = !evaluate_percent(this->monrace->freq_spell * 2);
    this->ability_flags = this->monrace->ability_flags;

    // [提案 C6] realm_abilities が指定された個体は、その魔法領域由来の
    // モンスター能力を詠唱能力へ加える (恒久的に monrace は変えず、詠唱文脈のみ)。
    if (this->monrace->realm_abilities != RealmType::NONE) {
        add_realm_granted_abilities(this->ability_flags, this->monrace->realm_abilities);
    }
}

Pos2D msa_type::get_position() const
{
    return Pos2D(this->y, this->x);
}

void msa_type::set_position(const Pos2D &pos)
{
    this->y = pos.y;
    this->x = pos.x;
}

Pos2D msa_type::get_position_lite() const
{
    return Pos2D(this->y_br_lite, this->x_br_lite);
}

void msa_type::set_position_lite(const Pos2D &pos)
{
    this->y_br_lite = pos.y;
    this->x_br_lite = pos.x;
}
