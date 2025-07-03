/*!
 * @brief モンスター種族の確率分布及び選択処理実装
 * @author Hourier
 * @date 2024/12/03
 */

#include "system/monrace/monrace-allocation.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "world/world-collapsion.h"
#include <range/v3/algorithm.hpp>
#include <range/v3/view.hpp>

MonraceAllocationEntry::MonraceAllocationEntry(MonraceId index, int level, short prob1, short prob2)
    : index(index)
    , level(level)
    , prob1(prob1)
    , prob2(prob2)
{
}

/*!
 * @brief 一般的なモンスター生成ルーチンで生成しても良いモンスターか否かのフィルタ処理
 * @param level 生成基準階
 * @return 生成許可ならばtrue、禁止ならばfalse
 * @details クエストモンスター、ダンジョンの主、指定階未満でのFORCE_DEPTH フラグ持ちは生成禁止
 */
bool MonraceAllocationEntry::is_permitted(int threshold_level) const
{
    const auto &monrace = this->get_monrace();
    if (monrace.misc_flags.has(MonsterMiscType::QUESTOR)) {
        return false;
    }

    if (monrace.misc_flags.has(MonsterMiscType::GUARDIAN)) {
        return false;
    }

    if (monrace.misc_flags.has(MonsterMiscType::FORCE_DEPTH) && (monrace.level > threshold_level)) {
        return false;
    }

    return true;
}

/*!
 * @brief クエスト内で生成しても良いモンスターか否かのフィルタ処理
 * @param level 生成基準階
 * @return 生成許可ならばtrue、禁止ならばfalse
 * @details RES_ALL、または生成標準階未満でのダメージ低減フラグ持ちは生成禁止
 */
bool MonraceAllocationEntry::is_defeatable(int threshold_level) const
{
    const auto &monrace = this->get_monrace();
    const auto has_resist_all = monrace.resistance_flags.has(MonsterResistanceType::RESIST_ALL);
    const auto can_diminish = monrace.special_flags.has(MonsterSpecialType::DIMINISH_MAX_DAMAGE);
    const auto is_shallow = monrace.level > threshold_level;
    return !has_resist_all && !(can_diminish && is_shallow);
}

bool MonraceAllocationEntry::is_collapse_exceeded() const
{
    const auto &monrace = this->get_monrace();
    return monrace.collapse_over <= world_collapsion.collapse_degree;
}

bool MonraceAllocationEntry::is_same_alliance(AllianceType alliance_id) const
{
    const auto &monrace = this->get_monrace();
    return monrace.alliance_idx == alliance_id;
}

void MonraceAllocationEntry::update_prob2(int division)
{
    const auto numer = this->prob2 * division;
    const auto q = numer / 64;
    const auto r = numer % 64;
    this->prob2 = static_cast<short>(randint0(64) < r ? q + 1 : q);
}

const MonraceDefinition &MonraceAllocationEntry::get_monrace() const
{
    return MonraceList::get_instance().get_monrace(index);
}

MonraceAllocationTable MonraceAllocationTable::instance{};

MonraceAllocationTable &MonraceAllocationTable::get_instance()
{
    return instance;
}

void MonraceAllocationTable::initialize()
{
    const auto &monraces = MonraceList::get_instance();
    this->entries.reserve(monraces.size());

    for (const auto &[id, monrace] : monraces | ranges::views::filter([](const auto &x) { return x.second.is_valid(); })) {
        const auto prob = static_cast<short>(100 / monrace.rarity);
        this->entries.emplace_back(id, monrace.level, prob, prob);
    }

    ranges::stable_sort(this->entries, {}, &MonraceAllocationEntry::level);
}

const MonraceAllocationEntry &MonraceAllocationTable::get_entry(int index) const
{
    return this->entries.at(index);
}

MonraceAllocationEntry &MonraceAllocationTable::get_entry(int index)
{
    return this->entries.at(index);
}
