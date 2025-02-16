#include "system/alloc-entries.h"
#include "floor/floor-base-definitions.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/monster-race-info.h"
#include "world/world-collapsion.h"
#include <array>

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

const MonraceDefinition &MonraceAllocationEntry::get_monrace() const
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

BaseitemAllocationEntry::BaseitemAllocationEntry(short index, int level, short prob1, short prob2)
    : index(index)
    , level(level)
    , prob1(prob1)
    , prob2(prob2)
{
}

bool BaseitemAllocationEntry::is_same_bi_key(const BaseitemKey &bi_key) const
{
    return bi_key == this->get_bi_key();
}

bool BaseitemAllocationEntry::is_chest() const
{
    return this->get_bi_key().tval() == ItemKindType::CHEST;
}

int BaseitemAllocationEntry::get_baseitem_level() const
{
    return this->get_baseitem().level;
}

bool BaseitemAllocationEntry::order_level(const BaseitemAllocationEntry &other) const
{
    return this->level < other.level;
}

BaseitemAllocationTable BaseitemAllocationTable::instance{};

const BaseitemDefinition &BaseitemAllocationEntry::get_baseitem() const
{
    return BaseitemList::get_instance().get_baseitem(this->index);
}

const BaseitemKey &BaseitemAllocationEntry::get_bi_key() const
{
    return this->get_baseitem().bi_key;
}

BaseitemAllocationTable &BaseitemAllocationTable::get_instance()
{
    return instance;
}

void BaseitemAllocationTable::initialize()
{
    std::array<short, MAX_DEPTH> num{};
    auto allocation_size = 0;
    const auto &baseitems = BaseitemList::get_instance();
    for (const auto &baseitem : baseitems) {
        for (const auto &[level, chance] : baseitem.alloc_tables) {
            if (chance != 0) {
                allocation_size++;
                num[level]++;
            }
        }
    }

    for (auto i = 1; i < MAX_DEPTH; i++) {
        num[i] += num[i - 1];
    }

    if (num[0] == 0) {
        THROW_EXCEPTION(std::runtime_error, _("町のアイテムがない！", "No town items!"));
    }

    this->entries = std::vector<BaseitemAllocationEntry>(allocation_size);
    std::array<short, MAX_DEPTH> aux{};
    for (const auto &baseitem : baseitems) {
        for (const auto &[level, chance] : baseitem.alloc_tables) {
            if (chance == 0) {
                continue;
            }

            const auto x = level;
            const short p = 100 / chance;
            const auto y = (x > 0) ? num[x - 1] : 0;
            const auto z = y + aux[x];
            this->entries[z] = BaseitemAllocationEntry(baseitem.idx, x, p, p);
            aux[x]++;
        }
    }
}

std::vector<BaseitemAllocationEntry>::iterator BaseitemAllocationTable::begin()
{
    return this->entries.begin();
}

std::vector<BaseitemAllocationEntry>::const_iterator BaseitemAllocationTable::begin() const
{
    return this->entries.cbegin();
}

std::vector<BaseitemAllocationEntry>::iterator BaseitemAllocationTable::end()
{
    return this->entries.end();
}

std::vector<BaseitemAllocationEntry>::const_iterator BaseitemAllocationTable::end() const
{
    return this->entries.cend();
}

size_t BaseitemAllocationTable::size() const
{
    return this->entries.size();
}

const BaseitemAllocationEntry &BaseitemAllocationTable::get_entry(int index) const
{
    return this->entries.at(index);
}

BaseitemAllocationEntry &BaseitemAllocationTable::get_entry(int index)
{
    return this->entries.at(index);
}

bool BaseitemAllocationTable::order_level(int index1, int index2) const
{
    const auto &entry1 = this->entries.at(index1);
    const auto &entry2 = this->entries.at(index2);
    return entry1.order_level(entry2);
}
