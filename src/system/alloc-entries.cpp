#include "system/alloc-entries.h"
#include "system/baseitem-info.h"
#include "system/monster-race-info.h"

MonraceAllocationEntry::MonraceAllocationEntry(MonsterRaceId index, int level, short prob1, short prob2)
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

const MonsterRaceInfo &MonraceAllocationEntry::get_monrace() const
{
    return MonraceList::get_instance().get_monrace(index);
}

MonraceAllocationTable MonraceAllocationTable::instance{};

MonraceAllocationTable &MonraceAllocationTable::get_instance()
{
    return instance;
}

size_t MonraceAllocationTable::size() const
{
    return this->entries.size();
}

void MonraceAllocationTable::initialize()
{
    auto &monraces = MonraceList::get_instance();
    const auto &elements = monraces.get_sorted_monraces();
    this->entries.reserve(elements.size());
    for (const auto &[monrace_id, r_ptr] : elements) {
        const auto prob = static_cast<short>(100 / r_ptr->rarity);
        this->entries.emplace_back(monrace_id, r_ptr->level, prob, prob);
    }
}

std::vector<MonraceAllocationEntry>::iterator MonraceAllocationTable::begin()
{
    return this->entries.begin();
}

std::vector<MonraceAllocationEntry>::const_iterator MonraceAllocationTable::begin() const
{
    return this->entries.begin();
}

std::vector<MonraceAllocationEntry>::iterator MonraceAllocationTable::end()
{
    return this->entries.end();
}

std::vector<MonraceAllocationEntry>::const_iterator MonraceAllocationTable::end() const
{
    return this->entries.end();
}

const MonraceAllocationEntry &MonraceAllocationTable::get_entry(int index) const
{
    return this->entries.at(index);
}

MonraceAllocationEntry &MonraceAllocationTable::get_entry(int index)
{
    return this->entries.at(index);
}

/* The entries in the "kind allocator table" */
std::vector<alloc_entry> alloc_kind_table;

BaseitemInfo &alloc_entry::get_baseitem() const
{
    return BaseitemList::get_instance().get_baseitem(this->index);
}
