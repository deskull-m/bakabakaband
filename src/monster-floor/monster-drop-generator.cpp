/*!
 * @file monster-drop-generator.cpp
 * @brief モンスター生成時にドロップ品を所持品として生成する処理の実装
 */

#include "monster-floor/monster-drop-generator.h"
#include "floor/floor-object.h"
#include "inventory/inventory-slot-types.h"
#include "monster-race/race-drop-flags.h"
#include "monster-race/race-kind-flags.h"
#include "monster-race/race-misc-flags.h"
#include "object-enchant/item-apply-magic.h"
#include "object/tval-types.h"
#include "sv-definition/sv-armor-types.h"
#include "sv-definition/sv-bow-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/angband-system.h"
#include "system/baseitem/baseitem-key.h"
#include "system/creature-entity.h"
#include "system/floor/floor-info.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/services/baseitem-monrace-service.h"
#include "term/z-rand.h"
#include "util/dice.h"
#include "util/enum-converter.h"
#include "util/probability-table.h"
#include <algorithm>
#include <limits>
#include <vector>

namespace {
/*!
 * @brief drop_flags からアイテム生成品質モード (AM_GOOD 等) を決める
 */
BIT_FLAGS decide_drop_quality_mode(const MonraceDefinition &monrace)
{
    BIT_FLAGS mode = 0L;
    if (monrace.drop_flags.has(MonsterDropType::DROP_GOOD)) {
        mode |= AM_GOOD;
    }
    if (monrace.drop_flags.has(MonsterDropType::DROP_GREAT)) {
        mode |= (AM_GOOD | AM_GREAT);
    }
    if (monrace.drop_flags.has(MonsterDropType::DROP_NASTY)) {
        mode |= AM_NASTY;
    }
    return mode;
}

/*!
 * @brief drop_flags からドロップ個数を決める
 * @details monster_death() 時の decide_drop_numbers() と同等。生成時点で
 *          確定しているクローン/ペット/アリーナ等の抑制条件も反映する。
 */
int decide_drop_numbers(const CreatureEntity &monster, const MonraceDefinition &monrace, bool inside_arena)
{
    int drop_numbers = 0;
    if (monrace.drop_flags.has(MonsterDropType::DROP_60) && evaluate_percent(60)) {
        drop_numbers++;
    }
    if (monrace.drop_flags.has(MonsterDropType::DROP_90) && evaluate_percent(90)) {
        drop_numbers++;
    }
    if (monrace.drop_flags.has(MonsterDropType::DROP_1D2)) {
        drop_numbers += Dice::roll(1, 2);
    }
    if (monrace.drop_flags.has(MonsterDropType::DROP_2D2)) {
        drop_numbers += Dice::roll(2, 2);
    }
    if (monrace.drop_flags.has(MonsterDropType::DROP_3D2)) {
        drop_numbers += Dice::roll(3, 2);
    }
    if (monrace.drop_flags.has(MonsterDropType::DROP_4D2)) {
        drop_numbers += Dice::roll(4, 2);
    }

    const auto cloned = monster.has_constant_flag(MonsterConstantFlagType::CLONED);
    if (cloned && monrace.kind_flags.has_not(MonsterKindType::UNIQUE)) {
        drop_numbers = 0;
    }
    if (monster.is_pet() || AngbandSystem::get_instance().is_phase_out() || inside_arena) {
        drop_numbers = 0;
    }
    if (monrace.misc_flags.has(MonsterMiscType::MULTIPLY) && (monrace.r_akills > 1024)) {
        drop_numbers = 0;
    }
    return drop_numbers;
}

/*!
 * @brief 武装モンスターのレベル帯ごとの近接武器候補
 * @details 先頭から順に `monrace.level >= min_level` で最初に一致した段を使い、
 *          その候補から 1 つを等確率で選ぶ。格が上がるほど上等な得物になる。
 *          **装備武器の打撃ダイスはモンスターの近接ダメージへ加算される**ため、
 *          この表がそのまま SOLDIER / WARRIOR 持ちモンスターの強化量になる。
 *          バランス調整はここで行うこと。
 */
struct InitialWeaponTier {
    int min_level; //!< この段が適用される最低種族レベル
    std::vector<BaseitemKey> candidates; //!< 等確率で選ぶ武器候補
};

const std::vector<InitialWeaponTier> &get_initial_weapon_tiers()
{
    static const std::vector<InitialWeaponTier> tiers = {
        { 60, { { ItemKindType::SWORD, SV_EXECUTIONERS_SWORD }, { ItemKindType::POLEARM, SV_HEAVY_LANCE }, { ItemKindType::HAFTED, SV_GREAT_HAMMER } } },
        { 40, { { ItemKindType::SWORD, SV_TWO_HANDED_SWORD }, { ItemKindType::POLEARM, SV_LOCHABER_AXE }, { ItemKindType::POLEARM, SV_GREAT_AXE } } },
        { 30, { { ItemKindType::SWORD, SV_KATANA }, { ItemKindType::POLEARM, SV_HALBERD }, { ItemKindType::POLEARM, SV_BATTLE_AXE } } },
        { 20, { { ItemKindType::SWORD, SV_LONG_SWORD }, { ItemKindType::POLEARM, SV_BROAD_SPEAR }, { ItemKindType::POLEARM, SV_BROAD_AXE } } },
        { 10, { { ItemKindType::SWORD, SV_TULWAR }, { ItemKindType::POLEARM, SV_AWL_PIKE }, { ItemKindType::HAFTED, SV_MACE } } },
        { 5, { { ItemKindType::SWORD, SV_SHORT_SWORD }, { ItemKindType::POLEARM, SV_SPEAR }, { ItemKindType::HAFTED, SV_WHIP } } },
        { 0, { { ItemKindType::SWORD, SV_DAGGER }, { ItemKindType::HAFTED, SV_CLUB }, { ItemKindType::POLEARM, SV_SICKLE } } },
    };

    return tiers;
}

/*!
 * @brief 種族レベルに応じた初期武器を 1 つ選ぶ
 * @param level モンスター種族のレベル
 * @return 選ばれた武器のベースアイテムキー
 */
BaseitemKey decide_initial_weapon(int level)
{
    for (const auto &tier : get_initial_weapon_tiers()) {
        if (level < tier.min_level) {
            continue;
        }

        return rand_choice(tier.candidates);
    }

    // 最下段の min_level が 0 のため通常ここには来ないが、防御的に短剣を返す。
    return { ItemKindType::SWORD, SV_DAGGER };
}

/*!
 * @brief 射手モンスターのレベル帯ごとの射撃武器 (弓) 候補
 * @details 近接武器と同じく、先頭から順に `monrace.level >= min_level` で最初に
 *          一致した段の候補から 1 つを等確率で選ぶ。
 *          **装備した弓の倍率はモンスターの射撃ダメージに反映される**
 *          (`monspell_damage()`) ため、この表がそのまま ARCHER / RANGER 持ち
 *          モンスターの射撃強化量になる。バランス調整はここで行うこと。
 */
const std::vector<InitialWeaponTier> &get_initial_bow_tiers()
{
    static const std::vector<InitialWeaponTier> tiers = {
        { 40, { { ItemKindType::BOW, SV_HEAVY_XBOW }, { ItemKindType::BOW, SV_LONG_BOW } } },
        { 25, { { ItemKindType::BOW, SV_LONG_BOW }, { ItemKindType::BOW, SV_LIGHT_XBOW } } },
        { 10, { { ItemKindType::BOW, SV_SHORT_BOW } } },
        { 0, { { ItemKindType::BOW, SV_SLING } } },
    };

    return tiers;
}

/*!
 * @brief 種族レベルに応じた初期弓を 1 つ選ぶ
 * @param level モンスター種族のレベル
 * @return 選ばれた弓のベースアイテムキー
 */
BaseitemKey decide_initial_bow(int level)
{
    for (const auto &tier : get_initial_bow_tiers()) {
        if (level < tier.min_level) {
            continue;
        }

        return rand_choice(tier.candidates);
    }

    // 最下段の min_level が 0 のため通常ここには来ないが、防御的にスリングを返す。
    return { ItemKindType::BOW, SV_SLING };
}

/*!
 * @brief 魔術師モンスターのレベル帯ごとの軽装 (胴体防具) 候補
 * @details 近接武器・弓と同じく、先頭から順に `monrace.level >= min_level` で
 *          最初に一致した段の候補から 1 つを等確率で選ぶ。
 *          魔術師が裸同然でいるのは不自然なため、ローブを基本としつつ格に応じて
 *          軽い革鎧までを与える。**モンスターの装備防具の AC は `get_ac()` で
 *          集計される**ため、この表がそのまま MAGE 持ちモンスターの AC 上昇量
 *          (最大 +5) になる。バランス調整はここで行うこと。
 */
const std::vector<InitialWeaponTier> &get_initial_robe_tiers()
{
    static const std::vector<InitialWeaponTier> tiers = {
        { 40, { { ItemKindType::SOFT_ARMOR, SV_SOFT_STUDDED_LEATHER }, { ItemKindType::SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR } } },
        { 15, { { ItemKindType::SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR }, { ItemKindType::SOFT_ARMOR, SV_ROBE } } },
        { 0, { { ItemKindType::SOFT_ARMOR, SV_ROBE } } },
    };

    return tiers;
}

/*!
 * @brief 種族レベルに応じた初期軽装を 1 つ選ぶ
 * @param level モンスター種族のレベル
 * @return 選ばれた胴体防具のベースアイテムキー
 */
BaseitemKey decide_initial_robe(int level)
{
    for (const auto &tier : get_initial_robe_tiers()) {
        if (level < tier.min_level) {
            continue;
        }

        return rand_choice(tier.candidates);
    }

    // 最下段の min_level が 0 のため通常ここには来ないが、防御的にローブを返す。
    return { ItemKindType::SOFT_ARMOR, SV_ROBE };
}
}

void generate_monster_drop_items(CreatureEntity &player, CreatureEntity &monster)
{
    auto &floor = *player.get_floor();
    const auto &monrace = monster.get_monrace();

    const auto do_gold = monrace.drop_flags.has_none_of({
        MonsterDropType::ONLY_ITEM,
        MonsterDropType::DROP_GOOD,
        MonsterDropType::DROP_GREAT,
    });
    auto do_item = monrace.drop_flags.has_not(MonsterDropType::ONLY_GOLD);
    do_item |= monrace.drop_flags.has_any_of({ MonsterDropType::DROP_GOOD, MonsterDropType::DROP_GREAT });

    auto drop_numbers = decide_drop_numbers(monster, monrace, floor.inside_arena);
    if (!do_item && !monrace.symbol_char_is_any_of("$")) {
        drop_numbers = 0;
    }
    if (drop_numbers <= 0) {
        return;
    }

    const auto mo_mode = decide_drop_quality_mode(monrace);

    // 死亡時 (monster_death) と同様に、生成基準階をモンスター種族レベルで底上げする。
    const auto backup_object_level = floor.object_level;
    floor.object_level = (floor.dun_level + monrace.level) / 2;

    for (auto i = 0; i < drop_numbers; i++) {
        if (do_gold && (!do_item || one_in_(2))) {
            const auto bi_key = BaseitemMonraceService::lookup_fixed_gold_drop(monrace.drop_flags);
            auto item = floor.make_gold(bi_key);
            // 構成材質に応じて金銭額を増減させる (貴金属系は増、紙・糞は減)。
            const auto gold_percent = monster.get_material_gold_drop_percent();
            if (gold_percent != 100) {
                const auto scaled = static_cast<int>(item.pval) * gold_percent / 100;
                item.pval = static_cast<PARAMETER_VALUE>(std::clamp(scaled, 1, static_cast<int>(std::numeric_limits<PARAMETER_VALUE>::max())));
            }
            monster.acquire_item(item);
        } else {
            if (auto item = make_object(player, mo_mode)) {
                monster.acquire_item(*item);
            }
        }
    }

    floor.object_level = backup_object_level;
}

void equip_armed_monster_initial_weapon(CreatureEntity &monster)
{
    const auto &monrace = monster.get_monrace();
    if (monrace.kind_flags.has_none_of({ MonsterKindType::SOLDIER, MonsterKindType::WARRIOR })) {
        return;
    }

    // 体構造的に武器を持てない個体 (四足・不定形・非実体等) には持たせない。
    if (!monster.can_equip_to(INVEN_MAIN_HAND)) {
        return;
    }

    // 既に利き手が埋まっているなら何もしない (生成直後は通常空)。
    if (monster.inventory[INVEN_MAIN_HAND]->is_valid()) {
        return;
    }

    ItemEntity weapon(decide_initial_weapon(monrace.level));
    weapon.number = 1;

    // エゴ・アーティファクト化や強化値は付けない。素の打撃ダイスのみを加える
    // ことで、強化量をレベル帯テーブルの範囲に収める。
    (void)monster.acquire_item(weapon);
}

void equip_ranged_monster_initial_bow(CreatureEntity &monster)
{
    const auto &monrace = monster.get_monrace();
    if (monrace.kind_flags.has_none_of({ MonsterKindType::ARCHER, MonsterKindType::RANGER })) {
        return;
    }

    // 体構造的に弓を構えられない個体 (四足・不定形・非実体等) には持たせない。
    if (!monster.can_equip_to(INVEN_BOW)) {
        return;
    }

    // 既に射撃スロットが埋まっているなら何もしない (生成直後は通常空)。
    if (monster.inventory[INVEN_BOW]->is_valid()) {
        return;
    }

    ItemEntity bow(decide_initial_bow(monrace.level));
    bow.number = 1;

    // 近接武器と同様、エゴ・アーティファクト化や強化値は付けない。
    (void)monster.acquire_item(bow);
}

void equip_spellcaster_monster_initial_robe(CreatureEntity &monster)
{
    const auto &monrace = monster.get_monrace();
    if (monrace.kind_flags.has_not(MonsterKindType::MAGE)) {
        return;
    }

    // 体構造的に胴体防具を着られない個体 (不定形・非実体等) には持たせない。
    if (!monster.can_equip_to(INVEN_BODY)) {
        return;
    }

    // 既に胴体スロットが埋まっているなら何もしない (生成直後は通常空)。
    if (monster.inventory[INVEN_BODY]->is_valid()) {
        return;
    }

    ItemEntity robe(decide_initial_robe(monrace.level));
    robe.number = 1;

    // 近接武器・弓と同様、エゴ・アーティファクト化や強化値は付けない。
    (void)monster.acquire_item(robe);
}
