#include "combat/slaying.h"
#include "artifact/fixed-art-types.h"
#include "combat/attack-criticality.h"
#include "effect/attribute-types.h"
#include "mind/mind-samurai.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-resistance-mask.h"
#include "monster/monster-info.h"
#include "object-enchant/tr-types.h"
#include "object/tval-types.h"
#include "player-base/player-class.h"
#include "player/attack-defense-types.h"
#include "realm/realm-hex-numbers.h"
#include "specific-object/torch.h"
#include "spell-realm/spells-crusade.h"
#include "spell-realm/spells-hex.h"
#include "system/creature-entity.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/z-rand.h"
#include "util/bit-flags-calculator.h"
#include "util/dice.h"
#include <algorithm>

/*!
 * @brief プレイヤー攻撃の種族スレイング倍率計算
 * @param creature クリーチャーへの参照
 * @param mult 算出前の基本倍率(/10倍)
 * @param flags スレイフラグ配列
 * @param m_ptr 目標モンスターの構造体参照ポインタ
 * @return スレイング加味後の倍率(/10倍)
 */
MULTIPLY mult_slaying(CreatureEntity &creature, MULTIPLY mult, const TrFlags &flags, const CreatureEntity &target)
{
    static const struct slay_table_t {
        tr_type slay_flag;
        MonsterKindType affect_race_flag;
        MULTIPLY slay_mult;
    } slay_table[] = {
        { TR_SLAY_ANIMAL, MonsterKindType::ANIMAL, 25 },
        { TR_KILL_ANIMAL, MonsterKindType::ANIMAL, 40 },
        { TR_SLAY_EVIL, MonsterKindType::EVIL, 20 },
        { TR_KILL_EVIL, MonsterKindType::EVIL, 35 },
        { TR_SLAY_GOOD, MonsterKindType::GOOD, 20 },
        { TR_KILL_GOOD, MonsterKindType::GOOD, 35 },
        { TR_SLAY_HUMAN, MonsterKindType::HUMAN, 25 },
        { TR_KILL_HUMAN, MonsterKindType::HUMAN, 40 },
        { TR_SLAY_MALE, MonsterKindType::MALE, 25 },
        { TR_KILL_MALE, MonsterKindType::MALE, 40 },
        { TR_SLAY_FEMALE, MonsterKindType::FEMALE, 25 },
        { TR_KILL_FEMALE, MonsterKindType::FEMALE, 40 },
        { TR_SLAY_UNDEAD, MonsterKindType::UNDEAD, 30 },
        { TR_KILL_UNDEAD, MonsterKindType::UNDEAD, 50 },
        { TR_SLAY_DEMON, MonsterKindType::DEMON, 30 },
        { TR_KILL_DEMON, MonsterKindType::DEMON, 50 },
        { TR_SLAY_ORC, MonsterKindType::ORC, 30 },
        { TR_KILL_ORC, MonsterKindType::ORC, 50 },
        { TR_SLAY_TROLL, MonsterKindType::TROLL, 30 },
        { TR_KILL_TROLL, MonsterKindType::TROLL, 50 },
        { TR_SLAY_GIANT, MonsterKindType::GIANT, 30 },
        { TR_KILL_GIANT, MonsterKindType::GIANT, 50 },
        { TR_SLAY_DRAGON, MonsterKindType::DRAGON, 30 },
        { TR_KILL_DRAGON, MonsterKindType::DRAGON, 50 },
    };

    auto &monrace = target.get_monrace();
    for (size_t i = 0; i < sizeof(slay_table) / sizeof(slay_table[0]); ++i) {
        const struct slay_table_t *p = &slay_table[i];

        if (flags.has_not(p->slay_flag) || monrace.kind_flags.has_not(p->affect_race_flag)) {
            continue;
        }

        if (is_original_ap_and_seen(creature, target)) {
            monrace.r_kind_flags.set(p->affect_race_flag);
        }

        mult = std::max(mult, p->slay_mult);
    }

    return mult;
}

namespace {
/*!
 * @brief プレイヤーが属性ブランドに免疫を持つか (モンスターの武器攻撃で標的がプレイヤーの場合に使用)
 * @details 毒ブランドはプレイヤー用の免疫クエリが無いため常に false (免疫なし扱い)。
 */
bool is_player_immune_to_brand(CreatureEntity &target, tr_type brand_flag)
{
    switch (brand_flag) {
    case TR_BRAND_ACID:
        return target.has_immune_acid() != 0;
    case TR_BRAND_ELEC:
        return target.has_immune_elec() != 0;
    case TR_BRAND_FIRE:
        return target.has_immune_fire() != 0;
    case TR_BRAND_COLD:
        return target.has_immune_cold() != 0;
    default:
        return false;
    }
}

/*!
 * @brief プレイヤーが属性ブランドに弱点を持つか (モンスターの武器攻撃で標的がプレイヤーの場合に使用)
 * @details 毒ブランドはプレイヤー用の弱点クエリが無いため常に false (弱点なし扱い)。
 */
bool is_player_vulnerable_to_brand(CreatureEntity &target, tr_type brand_flag)
{
    switch (brand_flag) {
    case TR_BRAND_ACID:
        return target.has_vuln_acid() != 0;
    case TR_BRAND_ELEC:
        return target.has_vuln_elec() != 0;
    case TR_BRAND_FIRE:
        return target.has_vuln_fire() != 0;
    case TR_BRAND_COLD:
        return target.has_vuln_cold() != 0;
    default:
        return false;
    }
}
}

/*!
 * @brief プレイヤー攻撃の属性スレイング倍率計算
 * @param creature クリーチャーへの参照
 * @param mult 算出前の基本倍率(/10倍)
 * @param flags スレイフラグ配列
 * @param m_ptr 目標モンスターの構造体参照ポインタ
 * @return スレイング加味後の倍率(/10倍)
 */
MULTIPLY mult_brand(CreatureEntity &creature, MULTIPLY mult, const TrFlags &flags, CreatureEntity &target)
{
    static const struct brand_table_t {
        tr_type brand_flag;
        EnumClassFlagGroup<MonsterResistanceType> resist_mask;
        MonsterResistanceType hurt_flag;
    } brand_table[] = {
        { TR_BRAND_ACID, RFR_EFF_IM_ACID_MASK, MonsterResistanceType::MAX },
        { TR_BRAND_ELEC, RFR_EFF_IM_ELEC_MASK, MonsterResistanceType::MAX },
        { TR_BRAND_FIRE, RFR_EFF_IM_FIRE_MASK, MonsterResistanceType::HURT_FIRE },
        { TR_BRAND_COLD, RFR_EFF_IM_COLD_MASK, MonsterResistanceType::HURT_COLD },
        { TR_BRAND_POIS, RFR_EFF_IM_POISON_MASK, MonsterResistanceType::MAX },
    };

    auto &monrace = target.get_monrace();
    for (size_t i = 0; i < sizeof(brand_table) / sizeof(brand_table[0]); ++i) {
        const struct brand_table_t *p = &brand_table[i];

        if (flags.has_not(p->brand_flag)) {
            continue;
        }

        // プレイヤーが攻撃対象の場合 (モンスターの武器攻撃) は、モンスター種族定義の代わりに
        // プレイヤー自身の免疫/弱点を参照してブランド倍率を決める。モンスター標的と対称に、
        // 免疫でブランド無効・弱点で 5 倍・それ以外 (部分耐性含む) は通常 2.5 倍とする。
        // (この属性ブランド倍率モデルではモンスターも「免疫のみ無効・部分耐性は素通し」のため対称)
        if (target.is_player()) {
            if (is_player_immune_to_brand(target, p->brand_flag)) {
                continue;
            }

            mult = std::max<short>(mult, is_player_vulnerable_to_brand(target, p->brand_flag) ? 50 : 25);
            continue;
        }

        /* Notice immunity */
        if (monrace.resistance_flags.has_any_of(p->resist_mask)) {
            if (is_original_ap_and_seen(creature, target)) {
                monrace.r_resistance_flags.set(monrace.resistance_flags & p->resist_mask);
            }

            continue;
        }

        /* Otherwise, take the damage */
        if (monrace.resistance_flags.has(p->hurt_flag)) {
            if (is_original_ap_and_seen(creature, target)) {
                monrace.r_resistance_flags.set(p->hurt_flag);
            }

            mult = std::max<short>(mult, 50);
            continue;
        }

        mult = std::max<short>(mult, 25);
    }

    return mult;
}

/*!
 * @brief ダメージにスレイ要素を加える総合処理ルーチン /
 * Extract the "total damage" from a given object hitting a given monster.
 * @param o_ptr 使用武器オブジェクトの構造体参照ポインタ
 * @param tdam 現在算出途中のダメージ値
 * @param m_ptr 目標モンスターの構造体参照ポインタ
 * @param mode 剣術のID
 * @param thrown 投擲処理ならばTRUEを指定する
 * @return 総合的なスレイを加味したダメージ値
 * @note
 * Note that "flasks of oil" do NOT do fire damage, although they\n
 * certainly could be made to do so.  XXX XXX\n
 *\n
 * Note that most brands and slays are x3, except Slay Animal (x2),\n
 * Slay Evil (x2), and Kill dragon (x5).\n
 */
int calc_attack_damage_with_slay(CreatureEntity &creature, ItemEntity *o_ptr, int tdam, CreatureEntity &target, combat_options mode, bool thrown)
{
    auto flags = o_ptr->get_flags();
    torch_flags(o_ptr, flags); /* torches has secret flags */

    if (!thrown) {
        if (creature.has_special_attack(ATTACK_ACID)) {
            flags.set(TR_BRAND_ACID);
        }
        if (creature.has_special_attack(ATTACK_COLD)) {
            flags.set(TR_BRAND_COLD);
        }
        if (creature.has_special_attack(ATTACK_ELEC)) {
            flags.set(TR_BRAND_ELEC);
        }
        if (creature.has_special_attack(ATTACK_FIRE)) {
            flags.set(TR_BRAND_FIRE);
        }
        if (creature.has_special_attack(ATTACK_POIS)) {
            flags.set(TR_BRAND_POIS);
        }
    }

    if (SpellHex(creature).is_spelling_specific(HEX_RUNESWORD)) {
        flags.set(TR_SLAY_GOOD);
    }

    if (has_slay_demon_from_exorcism(creature)) {
        flags.set(TR_SLAY_DEMON);
    }
    if (has_kill_demon_from_exorcism(creature)) {
        flags.set(TR_KILL_DEMON);
    }
    if (has_slay_undead_from_exorcism(creature)) {
        flags.set(TR_SLAY_UNDEAD);
    }
    if (has_kill_undead_from_exorcism(creature)) {
        flags.set(TR_KILL_UNDEAD);
    }

    MULTIPLY mult = 10;
    switch (o_ptr->bi_key.tval()) {
    case ItemKindType::NONE:
    case ItemKindType::SHOT:
    case ItemKindType::ARROW:
    case ItemKindType::BOLT:
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::SWORD:
    case ItemKindType::DIGGING:
    case ItemKindType::LITE: {
        mult = mult_slaying(creature, mult, flags, target);

        mult = mult_brand(creature, mult, flags, target);

        CreatureClass pc(creature);
        if (pc.equals(PlayerClassType::SAMURAI)) {
            mult = mult_hissatsu(creature, mult, flags, target, mode);
        }

        if (!pc.equals(PlayerClassType::SAMURAI) && (flags.has(TR_FORCE_WEAPON)) && (creature.get_current_mp() > (o_ptr->damage_dice.maxroll() / 5))) {
            creature.sub_current_mp((1 + (o_ptr->damage_dice.maxroll() / 5)));
            RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::MP);
            mult = mult * 3 / 2 + 20;
        }

        if ((o_ptr->is_specific_artifact(FixedArtifactId::NOTHUNG)) && (target.get_r_idx() == MonraceId::FAFNER)) {
            mult = 150;
        }

        /* 素手攻撃はスレイによる強化を半減させる */
        if (o_ptr->bi_key.tval() == ItemKindType::NONE) {
            mult = (mult - 10) / 2 + 10;
        }
        break;
    }

    default:
        break;
    }

    if (mult > 150) {
        mult = 150;
    }
    return tdam * mult / 10;
}

/*!
 * @brief 武器を装備したクリーチャーの1打撃分の武器ダメージを、プレイヤーの打撃処理と同一の式で算出する
 * @param attacker 攻撃側クリーチャー (プレイヤー・モンスターいずれも可)
 * @param weapon 使用する近接武器
 * @param target 攻撃対象クリーチャー (プレイヤー・モンスターいずれも可)
 * @param hand 使用する手 (0=利き手 / 1=逆手)。会心判定の基本命中力 (meichuu) 参照に使う
 * @return 武器ダイス→スレイ/ブランド倍率→会心→ヴォーパル→武器の to_d までを反映したダメージ値
 * @details
 * プレイヤーの process_weapon_attack() が武器から算出する部分
 * (ダイスロール→calc_attack_damage_with_slay()→critical_norm()→process_vorpal_attack()
 * → to_d 加算) と同一のパイプラインを、攻撃側の種別 (プレイヤー/モンスター) に依らず
 * 共通で適用する。これにより、武器を装備したモンスターは対プレイヤー・対モンスターの
 * いずれでもプレイヤーと同じ武器打撃ダメージを与える。
 * プレイヤー固有の追加ダイス (damage_dice_bonus) ・get_to_d(hand) 等の
 * 装備/職業由来ボーナスは呼出側で別途反映される (モンスターは get_melee_stat_damage_bonus() 等)。
 * ヴォーパルの倍率算出はプレイヤーと同一だが、ルーンソード呪唱 (術者状態) や
 * 斬鉄剣無効化 (対象が非切断) 等の術者/対象文脈依存分岐は武器固有プロパティのみに限定する。
 */
int calc_weapon_melee_damage(CreatureEntity &attacker, ItemEntity &weapon, CreatureEntity &target, int hand)
{
    const auto flags = weapon.get_flags();
    auto damage = calc_attack_damage_with_slay(attacker, &weapon, weapon.damage_dice.roll(), target, HISSATSU_NONE, false);
    damage = critical_norm(attacker, weapon.weight, weapon.to_h, damage, attacker.get_to_h(hand), HISSATSU_NONE, flags.has(TR_IMPACT));

    // ヴォーパル (メッタ斬り): プレイヤーの process_vorpal_attack() と同一の倍率算出。
    // メッセージは攻撃者・対象双方の文脈が必要なため、共通ダメージ計算では省略する。
    if (flags.has(TR_VORPAL)) {
        const auto is_vorpal_artifact = weapon.is_specific_artifact(FixedArtifactId::VORPAL_BLADE) || weapon.is_specific_artifact(FixedArtifactId::CHAINSWORD);
        const auto vorpal_chance = is_vorpal_artifact ? 2 : 4;
        if (randint1(vorpal_chance * 3 / 2) == 1) {
            auto vorpal_magnification = 2;
            while (one_in_(vorpal_chance)) {
                vorpal_magnification++;
            }
            damage *= vorpal_magnification;
        }
    }

    damage += weapon.to_d;
    return damage;
}

/*!
 * @brief 吸血武器を装備したクリーチャーの近接打撃による吸血 (HP 吸収) を適用する
 * @param attacker 攻撃側クリーチャー (プレイヤー・モンスターいずれも可)
 * @param weapon 使用した近接武器
 * @param target 攻撃対象クリーチャー (プレイヤー・モンスターいずれも可)
 * @param weapon_damage calc_weapon_melee_damage() が返した当該打撃の武器ダメージ
 * @return 吸収して回復した (ロールした) HP 量。0 なら吸血が発生しなかった
 * @details
 * プレイヤーの吸血処理 (blood-sucking-processor.cpp) と同じく、TR_VAMPIRIC を持つ武器で
 * 生命のある対象 (has_living_flag) を攻撃した場合に、与えた武器ダメージ (対象の残 HP が上限) を
 * 元に 2d(drain/6) の HP を攻撃側へ回復する。吸血はダメージ倍率を持たず回復のみ (プレイヤー版と同様)。
 * プレイヤー版が行う対象最大 HP 減少 (weaken) は、既存のモンスター吸血 (heal_monster_by_melee) が
 * 回復のみである慣習に合わせて行わない。回復量の攻撃全体上限 (MAX_VAMPIRIC_DRAIN) も既存モンスター
 * 吸血同様に設けない。メッセージ表示・体力バー再描画は呼出側が担う。
 */
int apply_weapon_vampiric_drain(CreatureEntity &attacker, const ItemEntity &weapon, const CreatureEntity &target, int weapon_damage)
{
    if (!weapon.get_flags().has(TR_VAMPIRIC) || !target.has_living_flag()) {
        return 0;
    }

    const auto drain = std::min(weapon_damage, target.get_current_hp());
    constexpr auto real_drain = 5;
    if (drain <= real_drain) {
        return 0;
    }

    const auto drain_heal = Dice::roll(2, drain / 6);
    attacker.heal_hp(drain_heal);
    return drain_heal;
}

/*!
 * @brief 装備武器による近接打撃が地震を起こすか判定する
 * @param weapon 使用した近接武器
 * @param weapon_damage calc_weapon_melee_damage() が返した当該打撃の武器ダメージ
 * @return 地震を起こすなら true
 * @details
 * プレイヤーの does_equip_cause_earthquake() と同じく、地震付与武器 (TR_EARTHQUAKE) で
 * 与えたダメージが 50 を超えるか 1/7 の確率で地震を起こす。実際の地震発生はモンスターの
 * 全打撃終了後 (打撃ループ中は floor / monster 参照が無効化されうるため) に呼出側が行う。
 */
bool does_weapon_cause_earthquake(const ItemEntity &weapon, int weapon_damage)
{
    if (!weapon.get_flags().has(TR_EARTHQUAKE)) {
        return false;
    }

    return (weapon_damage > 50) || one_in_(7);
}

AttributeFlags melee_attribute(CreatureEntity &creature, ItemEntity *o_ptr, combat_options mode)
{
    AttributeFlags attribute_flags{};
    attribute_flags.set(AttributeType::PLAYER_MELEE);

    if (CreatureClass(creature).equals(PlayerClassType::SAMURAI)) {
        static const struct samurai_convert_table_t {
            combat_options hissatsu_type;
            AttributeType attribute;
        } samurai_convert_table[] = {
            { HISSATSU_FIRE, AttributeType::FIRE },
            { HISSATSU_COLD, AttributeType::COLD },
            { HISSATSU_ELEC, AttributeType::ELEC },
            { HISSATSU_POISON, AttributeType::POIS },
            { HISSATSU_HAGAN, AttributeType::KILL_WALL },
        };

        for (size_t i = 0; i < sizeof(samurai_convert_table) / sizeof(samurai_convert_table[0]); ++i) {
            const struct samurai_convert_table_t *p = &samurai_convert_table[i];

            if (mode == p->hissatsu_type) {
                attribute_flags.set(p->attribute);
            }
        }
    }

    auto flags = o_ptr->get_flags();

    if (creature.has_special_attack(ATTACK_ACID)) {
        flags.set(TR_BRAND_ACID);
    }
    if (creature.has_special_attack(ATTACK_COLD)) {
        flags.set(TR_BRAND_COLD);
    }
    if (creature.has_special_attack(ATTACK_ELEC)) {
        flags.set(TR_BRAND_ELEC);
    }
    if (creature.has_special_attack(ATTACK_FIRE)) {
        flags.set(TR_BRAND_FIRE);
    }
    if (creature.has_special_attack(ATTACK_POIS)) {
        flags.set(TR_BRAND_POIS);
    }

    if (SpellHex(creature).is_spelling_specific(HEX_RUNESWORD)) {
        flags.set(TR_SLAY_GOOD);
    }

    static const struct brand_convert_table_t {
        tr_type brand_type;
        AttributeType attribute;
    } brand_convert_table[] = {
        { TR_BRAND_ACID, AttributeType::ACID },
        { TR_BRAND_FIRE, AttributeType::FIRE },
        { TR_BRAND_ELEC, AttributeType::ELEC },
        { TR_BRAND_COLD, AttributeType::COLD },
        { TR_BRAND_POIS, AttributeType::POIS },
        { TR_SLAY_GOOD, AttributeType::HELL_FIRE },
        { TR_KILL_GOOD, AttributeType::HELL_FIRE },
        { TR_SLAY_EVIL, AttributeType::HOLY_FIRE },
        { TR_KILL_EVIL, AttributeType::HOLY_FIRE },
    };

    for (size_t i = 0; i < sizeof(brand_convert_table) / sizeof(brand_convert_table[0]); ++i) {
        const struct brand_convert_table_t *p = &brand_convert_table[i];

        if (flags.has(p->brand_type)) {
            attribute_flags.set(p->attribute);
        }
    }

    if ((flags.has(TR_FORCE_WEAPON)) && (creature.get_current_mp() > (o_ptr->damage_dice.maxroll() / 5))) {
        attribute_flags.set(AttributeType::MANA);
    }

    return attribute_flags;
}
