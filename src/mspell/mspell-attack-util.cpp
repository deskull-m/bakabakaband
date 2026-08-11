#include "mspell/mspell-attack-util.h"
#include "monster-race/race-ability-flags.h"
#include "realm/realm-types.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"

namespace {
/*!
 * @brief realm 由来の高位能力を付与する最小モンスターレベル閾値 (提案C6第3弾)
 * @details これ未満のモンスターは各 realm の基本能力のみを得る。realm 由来能力は
 *          opt-in 個体のみに付くため、既定バランスには影響しない。調整用定数。
 */
constexpr int REALM_ADVANCED_ABILITY_MIN_LEVEL = 20;

/*!
 * @brief 魔法領域に対応する MonsterAbilityType 群を詠唱能力へ加える (提案C6)
 * @param flags 詠唱能力フラグ (OR-in 先)
 * @param realm 付与する魔法領域
 * @param level モンスターの実効レベル (高位能力のレベル段階化に使用、提案C6第3弾)
 * @details realm_abilities が指定されたモンスターの詠唱時に、その realm 由来の
 *          モンスター能力を ability_flags に OR-in する。既存 mspell 経路
 *          (自動ターゲット・MP消費(C4)・耐性・AI) をそのまま利用する。
 *          各 realm は「基本能力 (常時)」と「高位能力 (level >= 閾値)」の 2 段階に分け、
 *          低レベル個体には過剰な高位能力 (ボール/ブレス/召喚/高位 cause 等) を与えない。
 *          両段の和集合は従来の全集合と一致するため、閾値以上の個体は従来と同一。
 *          写像はバランス調整・拡張の起点。MUSIC / HISSATSU / HEX / NONE は現状未マッピング。
 */
void add_realm_granted_abilities(EnumClassFlagGroup<MonsterAbilityType> &flags, RealmType realm, int level)
{
    using Ma = MonsterAbilityType;
    const auto advanced = level >= REALM_ADVANCED_ABILITY_MIN_LEVEL;
    switch (realm) {
    case RealmType::LIFE:
        // 生命: 自己回復と対象を蝕む聖なる傷 (cause) の階梯。
        flags.set({ Ma::HEAL, Ma::CAUSE_1, Ma::CAUSE_2 });
        if (advanced) {
            flags.set({ Ma::CAUSE_3, Ma::CAUSE_4 });
        }
        break;
    case RealmType::SORCERY:
        // 仙術: 非属性の操作・妨害・移動。減速/混乱/盲目/恐慌と短距離転移。
        flags.set({ Ma::SLOW, Ma::CONF, Ma::BLIND, Ma::SCARE, Ma::BLINK });
        if (advanced) {
            flags.set({ Ma::HOLD, Ma::TELE_TO, Ma::TELE_AWAY });
        }
        break;
    case RealmType::NATURE:
        // 自然: 元素のボルト群を基本に、上位で元素・毒のボール。
        flags.set({ Ma::BO_FIRE, Ma::BO_COLD, Ma::BO_ELEC });
        if (advanced) {
            flags.set({ Ma::BA_ELEC, Ma::BA_COLD, Ma::BA_POIS });
        }
        break;
    case RealmType::CHAOS:
        // 混沌: マジックミサイル・火炎を基本に、上位で火/ログルス球とカオスブレス。
        flags.set({ Ma::MISSILE, Ma::BO_FIRE, Ma::CONF });
        if (advanced) {
            flags.set({ Ma::BA_FIRE, Ma::BA_CHAO, Ma::BR_CHAO });
        }
        break;
    case RealmType::DEATH:
        // 死: 傷の呪い・地獄の矢・魔力吸収を基本に、上位で高位 cause/地獄球/死者復活。
        flags.set({ Ma::CAUSE_1, Ma::CAUSE_2, Ma::BO_NETH, Ma::DRAIN_MANA });
        if (advanced) {
            flags.set({ Ma::CAUSE_3, Ma::CAUSE_4, Ma::BA_NETH, Ma::S_UNDEAD });
        }
        break;
    case RealmType::TRUMP:
        // トランプ: 転移の万能さを基本に、上位で多様な召喚。
        flags.set({ Ma::BLINK, Ma::TPORT, Ma::TELE_TO, Ma::TELE_AWAY });
        if (advanced) {
            flags.set({ Ma::S_MONSTER, Ma::S_MONSTERS, Ma::S_KIN, Ma::S_HOUND });
        }
        break;
    case RealmType::ARCANE:
        // 秘術: 汎用で威力控えめ。ミサイルと短距離転移を基本に、上位で魔力の矢/嵐。
        flags.set({ Ma::MISSILE, Ma::BLINK });
        if (advanced) {
            flags.set({ Ma::BO_MANA, Ma::BA_MANA });
        }
        break;
    case RealmType::CRAFT:
        // 匠: 自己強化 (加速・回復) を基本に、上位で無敵化。
        flags.set({ Ma::HASTE, Ma::HEAL });
        if (advanced) {
            flags.set(Ma::INVULNER);
        }
        break;
    case RealmType::DAEMON:
        // 悪魔: 火炎のボルトを基本に、上位で火炎球・火炎ブレス・地獄球・悪魔召喚。
        flags.set(Ma::BO_FIRE);
        if (advanced) {
            flags.set({ Ma::BA_FIRE, Ma::BR_FIRE, Ma::BA_NETH, Ma::S_DEMON });
        }
        break;
    case RealmType::CRUSADE:
        // 破邪: 光のボルト・恐慌・傷の呪いを基本に、上位でスターバースト・秘孔。
        flags.set({ Ma::BO_LITE, Ma::SCARE, Ma::CAUSE_2 });
        if (advanced) {
            flags.set({ Ma::BA_LITE, Ma::CAUSE_4 });
        }
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
    // [提案 C6第2弾] realm_abilities2 も指定されていれば併用し、二重詠唱者となる。
    // [提案 C6第3弾] 高位能力はモンスターの実効レベルで段階化する。
    const auto realm_ability_level = this->m_ptr->get_level();
    if (this->monrace->realm_abilities != RealmType::NONE) {
        add_realm_granted_abilities(this->ability_flags, this->monrace->realm_abilities, realm_ability_level);
    }
    if (this->monrace->realm_abilities2 != RealmType::NONE) {
        add_realm_granted_abilities(this->ability_flags, this->monrace->realm_abilities2, realm_ability_level);
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
