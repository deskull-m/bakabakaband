/*!
 * @file monster-drop-generator.cpp
 * @brief モンスター生成時にドロップ品を所持品として生成する処理の実装
 */

#include "monster-floor/monster-drop-generator.h"
#include "artifact/random-art-generator.h"
#include "floor/floor-object.h"
#include "inventory/inventory-slot-types.h"
#include "monster-race/race-drop-flags.h"
#include "monster-race/race-kind-flags.h"
#include "monster-race/race-misc-flags.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/item-magic-applier.h"
#include "object-enchant/trg-types.h"
#include "object/object-info.h"
#include "object/tval-types.h"
#include "sv-definition/sv-armor-types.h"
#include "sv-definition/sv-bow-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/angband-system.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-key.h"
#include "system/baseitem/baseitem-list.h"
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
#include <map>
#include <set>
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

/*!
 * @brief 武装度予算で埋める装備スロットと、そこへ装備できるアイテム種別
 * @details 先頭の要素から順に予算を割り当てるため、**重要な部位ほど先に**並べる。
 *          指輪 (INVEN_MAIN_RING / SUB_RING)・首飾り (INVEN_NECK) は呪い付きや
 *          効果がランダムな品が多く、光源 (INVEN_LITE) は燃料の設定が必要なため
 *          初版では対象外。射撃 (INVEN_BOW) も、射撃能力を持たない個体に弓だけ
 *          持たせても無意味なので ARCHER / RANGER の役割装備に任せて対象外とする。
 */
struct ArmamentSlotEntry {
    int slot; //!< 装備スロット (INVEN_*)
    std::vector<ItemKindType> tvals; //!< そのスロットへ装備できるアイテム種別
};

const std::vector<ArmamentSlotEntry> &get_armament_slot_entries()
{
    static const std::vector<ArmamentSlotEntry> entries = {
        { INVEN_BODY, { ItemKindType::SOFT_ARMOR, ItemKindType::HARD_ARMOR } },
        { INVEN_MAIN_HAND, { ItemKindType::SWORD, ItemKindType::POLEARM, ItemKindType::HAFTED } },
        { INVEN_HEAD, { ItemKindType::HELM, ItemKindType::CROWN } },
        { INVEN_SUB_HAND, { ItemKindType::SHIELD } },
        { INVEN_FEET, { ItemKindType::BOOTS } },
        { INVEN_ARMS, { ItemKindType::GLOVES } },
        { INVEN_OUTER, { ItemKindType::CLOAK } },
    };

    return entries;
}

/*!
 * @brief 武装度予算で購入できるベースアイテムか判定する
 * @details 通常のフロア生成に乗らない品 (生成確率が全て 0)・固定アーティファクト
 *          化する品・クエスト専用品・呪い付きの品は除外する。価値 0 の品は
 *          予算の意味が無くなるため除外する。
 */
bool is_purchasable_armament(const BaseitemDefinition &baseitem)
{
    if (!baseitem.is_valid() || (baseitem.cost <= 0)) {
        return false;
    }

    const auto &tables = baseitem.alloc_tables;
    const auto has_allocation = std::any_of(tables.begin(), tables.end(), [](const auto &table) { return table.chance > 0; });
    if (!has_allocation) {
        return false;
    }

    return baseitem.gen_flags.has_none_of({
        ItemGenerationTraitType::INSTA_ART,
        ItemGenerationTraitType::QUESTITEM,
        ItemGenerationTraitType::CURSED,
        ItemGenerationTraitType::HEAVY_CURSE,
        ItemGenerationTraitType::PERMA_CURSE,
    });
}

//! 武装度予算で購入できるベースアイテムの候補 (価値・性能付き)
struct ArmamentCandidate {
    BaseitemKey bi_key;
    int cost; //!< 予算から差し引く価値
    int quality; //!< 選定に使う性能 (防具は基本AC、武器は打撃ダイスの最大値)
};

/*!
 * @brief ベースアイテムの「装備としての性能」を求める
 * @details 装備しても効果が無い品 (AC 0 の衣装や打撃ダイスを持たない品) を
 *          候補から除くために使う。防具は基本AC、武器は打撃ダイスの最大値。
 *          **選定基準には使わない** (理由は `pick_armament` の説明を参照)。
 */
int calc_armament_quality(const BaseitemDefinition &baseitem)
{
    switch (baseitem.bi_key.tval()) {
    case ItemKindType::SWORD:
    case ItemKindType::POLEARM:
    case ItemKindType::HAFTED:
        return baseitem.damage_dice.maxroll() + baseitem.to_d;
    default:
        return baseitem.ac + baseitem.to_a;
    }
}

/*!
 * @brief スロットごとの購入候補一覧を得る (初回呼出時にベースアイテム表から構築)
 * @details ベースアイテム表はゲーム初期化時に確定するため、初回の呼出で構築して
 *          以降は使い回す。モンスター生成のたびに全ベースアイテムを走査しない。
 */
const std::map<int, std::vector<ArmamentCandidate>> &get_armament_candidates()
{
    static const auto candidates = [] {
        std::map<int, std::vector<ArmamentCandidate>> result;
        const auto &baseitems = BaseitemList::get_instance();
        for (const auto &entry : get_armament_slot_entries()) {
            auto &slot_candidates = result[entry.slot];
            for (const auto &baseitem : baseitems) {
                const auto tval = baseitem.bi_key.tval();
                if (std::find(entry.tvals.begin(), entry.tvals.end(), tval) == entry.tvals.end()) {
                    continue;
                }
                if (!is_purchasable_armament(baseitem)) {
                    continue;
                }

                const auto quality = calc_armament_quality(baseitem);
                if (quality <= 0) {
                    continue;
                }

                slot_candidates.push_back({ baseitem.bi_key, baseitem.cost, quality });
            }
        }

        return result;
    }();

    return candidates;
}

/*!
 * @brief 予算内で最も上等な装備を 1 つ選ぶ
 * @param slot 対象の装備スロット
 * @param budget このスロットに使える上限価値
 * @return 選ばれた候補。予算内の候補が無ければ tl::nullopt
 * @details 予算内で最も高価な品の価値を基準とし、その半額以上の候補から等確率で
 *          選ぶ。価値を選定基準にすることで「武装度が高いほど上質な装備」が
 *          単調に成り立ち (実データで防具AC計 17→42 / 武器ダイス 5→20)、同レベル帯の
 *          個体が全て同じ装備になることも避けられる。
 *          **性能 (`calc_armament_quality`) を選定基準にしてはならない。**
 *          本データには安価で高性能なフレーバー品 (鉄の甲羅 AC22 が 1G 等) が
 *          あるため、性能で選ぶと予算が効かず低レベル個体まで防具AC計 41〜86 に
 *          達してしまう。性能値は「装備しても無意味な AC0 の品」を候補から
 *          除くためだけに使う。
 */
tl::optional<ArmamentCandidate> pick_armament(int slot, int budget)
{
    const auto &candidates_map = get_armament_candidates();
    const auto it = candidates_map.find(slot);
    if (it == candidates_map.end()) {
        return tl::nullopt;
    }

    auto best_cost = 0;
    for (const auto &candidate : it->second) {
        if ((candidate.cost <= budget) && (candidate.cost > best_cost)) {
            best_cost = candidate.cost;
        }
    }

    if (best_cost <= 0) {
        return tl::nullopt;
    }

    std::vector<ArmamentCandidate> affordable;
    for (const auto &candidate : it->second) {
        if ((candidate.cost <= budget) && (candidate.cost * 2 >= best_cost)) {
            affordable.push_back(candidate);
        }
    }

    return rand_choice(affordable);
}

/*!
 * @brief 既に装備しているアイテムの価値の合計を求める
 * @details 役割装備 (SOLDIER / WARRIOR の近接武器、ARCHER / RANGER の弓、
 *          MAGE の軽装) で既に埋まっているスロットの分を武装度予算から差し引き、
 *          二重取りにならないようにする。
 */
int calc_equipped_armament_cost(const CreatureEntity &monster)
{
    auto total = 0;
    const auto &baseitems = BaseitemList::get_instance();
    for (auto slot = static_cast<int>(INVEN_MAIN_HAND); slot < static_cast<int>(INVEN_TOTAL); slot++) {
        const auto &item = *monster.inventory[slot];
        if (!item.is_valid()) {
            continue;
        }

        total += baseitems.get_baseitem(item.bi_id).cost;
    }

    return total;
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

void equip_monster_by_armament_budget(CreatureEntity &monster)
{
    // 役割装備で既に使った分を差し引いた残額が、この個体の購買力になる。
    auto budget = monster.get_monrace().get_armament_level() - calc_equipped_armament_cost(monster);
    if (budget <= 0) {
        return;
    }

    // 体構造的に装備でき、かつまだ空いているスロットだけを対象にする。
    std::vector<const ArmamentSlotEntry *> targets;
    for (const auto &entry : get_armament_slot_entries()) {
        if (!monster.can_equip_to(entry.slot) || monster.inventory[entry.slot]->is_valid()) {
            continue;
        }

        targets.push_back(&entry);
    }

    // 残りスロット数で等分した額を各スロットの上限とし、余りは次のスロットへ繰り越す。
    // これにより 1 部位に予算を使い切らず、武装度が高いほど全身が上等になる。
    for (auto i = 0U; i < targets.size(); i++) {
        const auto slot_budget = budget / static_cast<int>(targets.size() - i);
        const auto candidate = pick_armament(targets[i]->slot, slot_budget);
        if (!candidate) {
            continue;
        }

        ItemEntity item(candidate->bi_key);
        item.number = 1;

        // 役割装備と同様、エゴ・アーティファクト化や強化値は付けない。
        (void)monster.acquire_item(item);
        budget -= candidate->cost;
    }
}

void apply_drop_kind_magic(CreatureEntity &creature, ItemEntity &item, int grade)
{
    const auto level = creature.get_floor()->dun_level;
    switch (grade) {
    case -2:
        ItemMagicApplier(creature, &item, level, AM_NO_FIXED_ART | AM_GOOD | AM_GREAT | AM_CURSED).execute();
        return;
    case -1:
        ItemMagicApplier(creature, &item, level, AM_NO_FIXED_ART | AM_GOOD | AM_CURSED).execute();
        return;
    case 0:
        ItemMagicApplier(creature, &item, level, AM_NO_FIXED_ART).execute();
        return;
    case 1:
        ItemMagicApplier(creature, &item, level, AM_NO_FIXED_ART | AM_GOOD).execute();
        return;
    case 2:
        ItemMagicApplier(creature, &item, level, AM_NO_FIXED_ART | AM_GOOD | AM_GREAT).execute();
        return;
    case 3:
        ItemMagicApplier(creature, &item, level, AM_GOOD | AM_GREAT | AM_SPECIAL).execute();
        if (!item.is_fixed_artifact()) {
            become_random_artifact(creature, &item, false);
        }
        return;
    default:
        return;
    }
}

short resolve_fixed_item_bi_id(CreatureEntity &creature, const MonraceDropKind &entry, bool is_itemkind)
{
    if (!is_itemkind) {
        return entry.id;
    }

    const auto tval = i2enum<ItemKindType>(entry.id);
    if (entry.use_allocation_table) {
        // 通常のアイテム生成 (make_object) と同じ深度加重抽選を使う。
        // 生成階を超える深度の品は選ばれないため、浅い階で高級品が出ない。
        // 候補が 1 つも無い深度では 0 (= 生成しない) が返る。
        auto &table = BaseitemAllocationTable::get_instance();
        table.set_restriction([tval](short bi_id) { return BaseitemList::get_instance().get_baseitem(bi_id).bi_key.tval() == tval; });
        const auto &floor = *creature.get_floor();
        const auto bi_id = floor.select_baseitem_id(floor.object_level, 0);
        table.reset_restriction();
        return bi_id;
    }

    // sval を省略 (tl::nullopt) すると当該種別の中から無作為にベースアイテムが選ばれる。
    // 0 を渡すと「sval 0 のベースアイテム」の完全一致検索になり、SWORD 等
    // sval 0 が存在しない種別で例外を投げるので注意。
    // 候補ゼロの種別は reader が弾いているため、ここで例外にはならない。
    return BaseitemList::get_instance().lookup_baseitem_id(BaseitemKey(tval));
}

tl::optional<ItemEntity> generate_fixed_item(CreatureEntity &creature, const MonraceDropKind &entry, bool is_itemkind)
{
    // 深度加重抽選 (use_allocation_table) で候補が無かった場合は 0 が返るので生成しない。
    const auto bi_id = resolve_fixed_item_bi_id(creature, entry, is_itemkind);
    if (bi_id == 0) {
        return tl::nullopt;
    }

    ItemEntity item;
    item.generate(bi_id);

    // apply_magic: false はハードコーディング由来の「素のアイテム」指定。
    // grade を無視し、エゴ・アーティファクト化・呪いのいずれも起こさない。
    if (entry.apply_magic) {
        apply_drop_kind_magic(creature, item, entry.grade);
    }

    return item;
}

bool roll_fixed_item_entry(const MonraceDropKind &entry, std::set<int> &fired_groups)
{
    // 同グループの先行エントリが既に当たっていれば、このエントリは抽選せず見送る。
    if ((entry.exclusive_group > 0) && fired_groups.contains(entry.exclusive_group)) {
        return false;
    }

    if (randint1(entry.denominator) > entry.numerator) {
        return false;
    }

    if (entry.exclusive_group > 0) {
        fired_groups.insert(entry.exclusive_group);
    }

    return true;
}

/*!
 * @brief 固定装備指定 1 リスト分をモンスターに持たせる
 * @param player プレイヤーへの参照 (アイテム生成基準)
 * @param monster 対象モンスター
 * @param entries 装備指定のリスト
 * @param is_itemkind entries がアイテム種別指定 (equip_tvals) なら true
 */
static void equip_monster_fixed_entries(CreatureEntity &player, CreatureEntity &monster, const std::vector<MonraceDropKind> &entries, bool is_itemkind)
{
    // 排他グループ (exclusive_group) は 1 リストの中で閉じる。
    std::set<int> fired_groups;
    for (const auto &entry : entries) {
        if (!roll_fixed_item_entry(entry, fired_groups)) {
            continue;
        }

        const auto item_nums = entry.dice.roll();
        for (auto i = 0; i < item_nums; i++) {
            auto item = generate_fixed_item(player, entry, is_itemkind);
            if (!item) {
                continue;
            }

            // 装備できるスロットが空いていれば装備し、無理なら所持品に入る。
            // いずれにせよ死亡時は drop_all_inventory() で床へ落ちる。
            (void)monster.acquire_item(*item);
        }
    }
}

void equip_monster_fixed_items(CreatureEntity &player, CreatureEntity &monster)
{
    const auto &monrace = monster.get_monrace();
    equip_monster_fixed_entries(player, monster, monrace.equip_kinds, false);
    equip_monster_fixed_entries(player, monster, monrace.equip_tvals, true);
}
