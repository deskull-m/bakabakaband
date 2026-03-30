/*!
 * @file random-art-characteristics.cpp
 * @brief ランダムアーティファクトのバイアス付加処理実装
 */

#include "artifact/random-art-characteristics.h"
#include "flavor/object-flavor.h"
#include "game-option/cheat-types.h"
#include "io/files-util.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "player-base/player-class.h"
#include "system/creature-entity.h"
#include "system/item-entity.h"
#include "util/bit-flags-calculator.h"
#include "wizard/wizard-messages.h"
#include <sstream>
#include <string_view>

static void pval_subtraction(ItemEntity *o_ptr)
{
    if (o_ptr->pval > 0) {
        o_ptr->pval = 0 - (o_ptr->pval + randint1(4));
    }

    if (o_ptr->to_a > 0) {
        o_ptr->to_a = 0 - (o_ptr->to_a + randint1(4));
    }

    if (o_ptr->to_h > 0) {
        o_ptr->to_h = 0 - (o_ptr->to_h + randint1(4));
    }

    if (o_ptr->to_d > 0) {
        o_ptr->to_d = 0 - (o_ptr->to_d + randint1(4));
    }
}

static void add_negative_flags(ItemEntity *o_ptr)
{
    if (one_in_(4)) {
        o_ptr->curse_flags.set(CurseTraitType::PERMA_CURSE);
    }

    if (one_in_(3)) {
        o_ptr->art_flags.set(TR_TY_CURSE);
    }

    if (one_in_(2)) {
        o_ptr->art_flags.set(TR_AGGRAVATE);
    }

    if (one_in_(8)) {
        o_ptr->art_flags.set(TR_NASTY_AGGRAVATE);
    }

    if (one_in_(3)) {
        o_ptr->art_flags.set(TR_DRAIN_EXP);
    }

    if (one_in_(6)) {
        o_ptr->art_flags.set(TR_ADD_L_CURSE);
    }

    if (one_in_(9)) {
        o_ptr->art_flags.set(TR_ADD_H_CURSE);
    }

    if (one_in_(9)) {
        o_ptr->art_flags.set(TR_DRAIN_HP);
    }

    if (one_in_(9)) {
        o_ptr->art_flags.set(TR_DRAIN_MANA);
    }

    if (one_in_(2)) {
        o_ptr->art_flags.set(TR_TELEPORT);
    } else if (one_in_(3)) {
        o_ptr->art_flags.set(TR_NO_TELE);
    }
}

/*!
 * @brief ランダムアーティファクト生成中、対象のオブジェクトを呆いのアーティファクトにする経過処理。/ generation process of cursed artifact.
 * @details pval、AC、命中、ダメージが正の場合、符号反転の上1d4だけ悪化させ、重い呢い、呢いフラグを必ず付加。
 * 祝福を無効。確率に応じて、永遠の呢い、太古の怨念、経験値吸収、弱い呢いの継続的付加、強い呢いの継続的付加、HP吸収の呢い、
 * MP吸収の呢い、乱テレポート、反テレポート、反魔法をつける。
 * @attention プレイヤーの職業依存処理あり。
 * @param creature クリーチャーへの参照
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 */
void curse_artifact(CreatureEntity &creature, ItemEntity *o_ptr)
{
    pval_subtraction(o_ptr);
    o_ptr->curse_flags.set({ CurseTraitType::HEAVY_CURSE, CurseTraitType::CURSED });
    o_ptr->art_flags.reset(TR_BLESSED);
    add_negative_flags(o_ptr);
    if (!CreatureClass(creature).is_soldier() && one_in_(3)) {
        o_ptr->art_flags.set(TR_NO_MAGIC);
    }
}

/*!
 * @brief ランダムアーティファクトの名前リストをオブジェクト種別と生成パワーに応じて選択する
 * @param armour 防具かどうか
 * @param power 生成パワー
 * @return ファイル名
 */
static std::string get_random_art_filename(const bool armour, const int power)
{
    std::stringstream ss;
    ss << (armour ? "a_" : "w_");
    switch (power) {
    case 0:
        ss << "cursed";
        break;
    case 1:
        ss << "low";
        break;
    case 2:
        ss << "med";
        break;
    default:
        ss << "high";
    }

    ss << _("_j.txt", ".txt");
    return ss.str();
}

/*!
 * @brief ランダムアーティファクト生成中、対象のオブジェクトに名前を与える.
 * @param o_ptr 処理中のアイテム参照ポインタ
 * @param armour 対象のオブジェクトが防具が否か
 * @param power 銘の基準となるオブジェクトの価値レベル(0=呪い、1=低位、2=中位、3以上=高位)
 * @return ランダムアーティファクト名
 * @details 確率によって、シンダリン銘、漢字銘 (anameからランダム)、固定名のいずれか一つが与えられる.
 * シンダリン銘：10%、漢字銘18%、固定銘72% (固定銘が尽きていたら漢字銘になることもある).
 */
std::string get_random_name(const ItemEntity &item, bool armour, int power)
{
    const auto prob = randint1(100);
    constexpr auto chance_sindarin = 10;
    if (prob <= chance_sindarin) {
        return get_table_sindarin();
    }

    constexpr auto chance_table = 20;
    if (prob <= chance_table) {
        return get_table_name();
    }

    const auto filename = get_random_art_filename(armour, power);
    const auto random_artifact_name = get_random_line(filename.data(), enum2i(item.artifact_bias));
#ifdef JP
    if (random_artifact_name.has_value()) {
        return random_artifact_name.value();
    }

    return get_table_name();
#else
    return random_artifact_name.value();
#endif
}

/*対邪平均ダメージの計算処理*/
static int calc_arm_avgdamage(CreatureEntity &creature, ItemEntity *o_ptr)
{
    const auto flags = o_ptr->get_flags();
    /// @todo 最初に小数点以下を切り捨てているため誤差が大きくなっている。なるべく正しいダメージが計算されるようにしたい。
    const auto base = o_ptr->damage_dice.floored_expected_value();
    auto dam = base;

    // 対邪ボーナスの計算
    int s_evil;
    if (flags.has(TR_KILL_EVIL)) {
        dam = s_evil = dam * 7 / 2;
    } else if (flags.has(TR_SLAY_EVIL)) {
        dam = s_evil = dam * 2;
    } else {
        s_evil = dam;
    }

    // 理力武器ボーナスの計算
    int forced;
    if (flags.has(TR_FORCE_WEAPON)) {
        dam = forced = dam * 3 / 2 + o_ptr->damage_dice.floored_expected_value_multiplied_by(2);
    } else {
        forced = dam;
    }

    // 切れ味ボーナスの計算
    int vorpal;
    if (flags.has(TR_VORPAL)) {
        dam = vorpal = dam * 11 / 9;
    } else {
        vorpal = dam;
    }

    dam += o_ptr->to_d;
    constexpr auto fmt = _("\u7d20:%d> \u5bfe\u90aa:%d> \u7406\u529b:%d> \u5207:%d> \u6700\u7d42:%d", "Normal:%d> Evil:%d> Force:%d> Vorpal:%d> Total:%d");
    msg_format_wizard(creature, CHEAT_OBJECT, fmt, base, s_evil, forced, vorpal, dam);
    return dam;
}

bool has_extreme_damage_rate(CreatureEntity &creature, ItemEntity *o_ptr)
{
    const auto damage = calc_arm_avgdamage(creature, o_ptr);
    const auto flags = o_ptr->get_flags();
    const auto has_blows = flags.has(TR_BLOWS);
    const auto pval = o_ptr->pval;

    if (flags.has(TR_VAMPIRIC)) {
        if (has_blows && pval == 1 && damage > 52) {
            return true;
        }
        if (has_blows && pval == 2 && damage > 43) {
            return true;
        }
        if (has_blows && pval == 3 && damage > 33) {
            return true;
        }
        return damage > 63;
    }

    if (has_blows && pval == 1 && damage > 65) {
        return true;
    }
    if (has_blows && pval == 2 && damage > 52) {
        return true;
    }
    if (has_blows && pval == 3 && damage > 40) {
        return true;
    }
    return damage > 75;
}
