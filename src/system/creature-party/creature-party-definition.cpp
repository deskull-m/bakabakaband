#include "system/creature-party/creature-party-definition.h"
#include "system/enums/monrace/monrace-id.h"
#include "util/dice.h"
#include "util/enum-converter.h"
#include <algorithm>

/*!
 * @brief パーティメンバーのコンストラクタ
 * @param monrace_id モンスター種族ID
 * @param count_dice 出現数ダイス (XdY形式)
 * @param probability 出現確率 (%)
 */
CreaturePartyMember::CreaturePartyMember(MonraceId monrace_id, const Dice &count_dice, int probability)
    : monrace_id(monrace_id)
    , count_dice(count_dice)
    , probability(probability)
{
}

/*!
 * @brief パーティ定義のコンストラクタ
 */
CreaturePartyDefinition::CreaturePartyDefinition()
    : idx(0)
    , min_level(0)
    , max_level(999)
    , rarity(1)
    , allow_unique(false)
{
}

/*!
 * @brief パーティ定義が有効かどうか判定
 * @return パーティが有効ならtrue
 */
bool CreaturePartyDefinition::is_valid() const
{
    if (this->idx <= 0) {
        return false;
    }

    if (this->members.empty()) {
        return false;
    }

    if (this->min_level < 0 || this->max_level < this->min_level) {
        return false;
    }

    return true;
}

/*!
 * @brief 指定された階層で生成可能かどうか判定
 * @param current_level 現在の階層レベル
 * @return 生成可能ならtrue
 */
bool CreaturePartyDefinition::can_generate(int current_level) const
{
    if (!this->is_valid()) {
        return false;
    }

    if (current_level < this->min_level || current_level > this->max_level) {
        return false;
    }

    return true;
}

/*!
 * @brief パーティの総モンスター数を計算（最大値）
 * @return 最大モンスター数
 */
int CreaturePartyDefinition::get_total_monster_count() const
{
    int total = 0;
    for (const auto &member : this->members) {
        total += member.count_dice.maxroll();
    }
    return total;
}

/*!
 * @brief パーティメンバーを追加
 * @param monrace_id モンスター種族ID
 * @param count_dice 出現数ダイス (XdY形式)
 * @param probability 出現確率 (%)
 */
void CreaturePartyDefinition::add_member(MonraceId monrace_id, const Dice &count_dice, int probability)
{
    this->members.emplace_back(monrace_id, count_dice, probability);
}
