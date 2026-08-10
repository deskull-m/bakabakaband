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
 * @brief 魔法領域の「基本能力」(第1〜2書相当) を詠唱能力へ加える (提案C6)
 * @details ボルト・操作・自己強化など低〜中位の能力。realm_abilities の常時分、および
 *          魔導書学習 (提案C6-R) で第1〜2書を学んだ個体が得る分。
 */
void add_realm_base_abilities(EnumClassFlagGroup<MonsterAbilityType> &flags, RealmType realm)
{
    using Ma = MonsterAbilityType;
    switch (realm) {
    case RealmType::LIFE: // 生命: 自己回復と聖なる傷 (cause) の基本階梯。
        flags.set({ Ma::HEAL, Ma::CAUSE_1, Ma::CAUSE_2 });
        break;
    case RealmType::SORCERY: // 仙術: 非属性の操作・妨害・短距離転移。
        flags.set({ Ma::SLOW, Ma::CONF, Ma::BLIND, Ma::SCARE, Ma::BLINK });
        break;
    case RealmType::NATURE: // 自然: 元素のボルト群。
        flags.set({ Ma::BO_FIRE, Ma::BO_COLD, Ma::BO_ELEC });
        break;
    case RealmType::CHAOS: // 混沌: マジックミサイル・火炎ボルト・混乱。
        flags.set({ Ma::MISSILE, Ma::BO_FIRE, Ma::CONF });
        break;
    case RealmType::DEATH: // 死: 傷の呪い・地獄の矢・魔力吸収。
        flags.set({ Ma::CAUSE_1, Ma::CAUSE_2, Ma::BO_NETH, Ma::DRAIN_MANA });
        break;
    case RealmType::TRUMP: // トランプ: 転移の万能さ。
        flags.set({ Ma::BLINK, Ma::TPORT, Ma::TELE_TO, Ma::TELE_AWAY });
        break;
    case RealmType::ARCANE: // 秘術: 汎用で威力控えめ。ミサイルと短距離転移。
        flags.set({ Ma::MISSILE, Ma::BLINK });
        break;
    case RealmType::CRAFT: // 匠: 自己強化 (加速・回復)。
        flags.set({ Ma::HASTE, Ma::HEAL });
        break;
    case RealmType::DAEMON: // 悪魔: 火炎のボルト/ボール。
        flags.set({ Ma::BO_FIRE, Ma::BA_FIRE });
        break;
    case RealmType::CRUSADE: // 破邪: 光のボルト・恐慌・傷の呪い。
        flags.set({ Ma::BO_LITE, Ma::SCARE, Ma::CAUSE_2 });
        break;
    default:
        break;
    }
}

/*!
 * @brief 魔法領域の「高位能力」(第3〜4書相当) を詠唱能力へ加える (提案C6)
 * @details ボール・ブレス・召喚・高位 cause・無敵など高位の能力。realm_abilities では
 *          実効レベルが閾値以上のとき、魔導書学習 (提案C6-R) では第3〜4書を学んだ個体が得る。
 */
void add_realm_advanced_abilities(EnumClassFlagGroup<MonsterAbilityType> &flags, RealmType realm)
{
    using Ma = MonsterAbilityType;
    switch (realm) {
    case RealmType::LIFE:
        flags.set({ Ma::CAUSE_3, Ma::CAUSE_4 });
        break;
    case RealmType::SORCERY:
        flags.set({ Ma::HOLD, Ma::TELE_TO, Ma::TELE_AWAY });
        break;
    case RealmType::NATURE:
        flags.set({ Ma::BA_ELEC, Ma::BA_COLD, Ma::BA_POIS });
        break;
    case RealmType::CHAOS:
        flags.set({ Ma::BA_FIRE, Ma::BA_CHAO, Ma::BR_CHAO });
        break;
    case RealmType::DEATH:
        flags.set({ Ma::CAUSE_3, Ma::CAUSE_4, Ma::BA_NETH, Ma::S_UNDEAD });
        break;
    case RealmType::TRUMP:
        flags.set({ Ma::S_MONSTER, Ma::S_MONSTERS, Ma::S_KIN, Ma::S_HOUND });
        break;
    case RealmType::ARCANE:
        flags.set({ Ma::BO_MANA, Ma::BA_MANA });
        break;
    case RealmType::CRAFT:
        flags.set({ Ma::INVULNER });
        break;
    case RealmType::DAEMON:
        flags.set({ Ma::BR_FIRE, Ma::BA_NETH, Ma::S_DEMON });
        break;
    case RealmType::CRUSADE:
        flags.set({ Ma::BA_LITE, Ma::CAUSE_4 });
        break;
    default:
        break;
    }
}

/*!
 * @brief realm_abilities (魔法領域まるごと付与) 由来の詠唱能力を加える (提案C6)
 * @details 基本能力は常時、高位能力は実効レベルが閾値以上のときに付与する (提案C6第3弾)。
 */
void add_realm_granted_abilities(EnumClassFlagGroup<MonsterAbilityType> &flags, RealmType realm, int level)
{
    add_realm_base_abilities(flags, realm);
    if (level >= REALM_ADVANCED_ABILITY_MIN_LEVEL) {
        add_realm_advanced_abilities(flags, realm);
    }
}

/*!
 * @brief 魔導書学習 (提案C6-R) 由来の詠唱能力を加える
 * @details プレイヤー同様、モンスターは所定の魔導書から realm 呪文を学習する
 *          (spell_learned に登録済み)。学習した書のティアに応じて能力を付与する:
 *          第1〜2書 (spell_id 0..15) を学べば基本能力、第3〜4書 (16..31) を学べば
 *          高位能力。realm_abilities (まるごと付与) とは独立の、より忠実な経路。
 *          詠唱自体は既存 mspell (自動ターゲット・MP・耐性・AI) をそのまま用いる。
 */
void add_learned_spellbook_abilities(EnumClassFlagGroup<MonsterAbilityType> &flags, const CreatureEntity &caster)
{
    const auto realm = caster.get_realm1();
    if (realm == RealmType::NONE) {
        return;
    }

    auto learned_basic = false;
    for (auto spell_id = 0; spell_id < 16; ++spell_id) {
        if (caster.has_learned_spell(0, spell_id)) {
            learned_basic = true;
            break;
        }
    }
    auto learned_advanced = false;
    for (auto spell_id = 16; spell_id < 32; ++spell_id) {
        if (caster.has_learned_spell(0, spell_id)) {
            learned_advanced = true;
            break;
        }
    }

    if (learned_basic) {
        add_realm_base_abilities(flags, realm);
    }
    if (learned_advanced) {
        add_realm_advanced_abilities(flags, realm);
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

    // [提案 C6-R] 魔導書から学習した realm 呪文に応じて詠唱能力を加える (学習済みの書のティア準拠)。
    add_learned_spellbook_abilities(this->ability_flags, *this->m_ptr);
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
