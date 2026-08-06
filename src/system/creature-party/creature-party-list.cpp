#include "system/creature-party/creature-party-list.h"
#include "system/creature-party/creature-party-definition.h"

/*!
 * @brief シングルトンインスタンスの取得
 * @return CreaturePartyListのインスタンス
 */
CreaturePartyList &CreaturePartyList::get_instance()
{
    static CreaturePartyList instance;
    return instance;
}

/*!
 * @brief パーティ定義を取得 (const版)
 * @param party_id パーティID
 * @return パーティ定義への参照
 */
const CreaturePartyDefinition &CreaturePartyList::get_party(int party_id) const
{
    return this->parties.at(party_id);
}

/*!
 * @brief パーティ定義を取得 (非const版)
 * @param party_id パーティID
 * @return パーティ定義への参照
 */
CreaturePartyDefinition &CreaturePartyList::get_party(int party_id)
{
    return this->parties.at(party_id);
}

/*!
 * @brief 新しいパーティ定義を追加
 * @param party_id パーティID
 * @return 追加されたパーティ定義への参照
 */
CreaturePartyDefinition &CreaturePartyList::emplace(int party_id)
{
    return this->parties.emplace(party_id, CreaturePartyDefinition()).first->second;
}

/*!
 * @brief 指定された階層で生成可能なパーティのリストを取得
 * @param current_level 現在の階層レベル
 * @return 生成可能なパーティIDのリスト
 */
std::vector<int> CreaturePartyList::get_available_parties(int current_level) const
{
    std::vector<int> available_parties;

    for (const auto &[party_id, party] : *this) {
        if (party.can_generate(current_level)) {
            available_parties.push_back(party_id);
        }
    }

    return available_parties;
}

/*!
 * @brief パーティIDが有効かどうか判定
 * @param party_id パーティID
 * @return パーティIDが有効ならtrue
 */
bool CreaturePartyList::is_valid(int party_id)
{
    return party_id > 0;
}

/*!
 * @brief 空のパーティIDを返す
 * @return 0 (無効なパーティID)
 */
int CreaturePartyList::empty_id()
{
    return 0;
}

/*!
 * @brief すべてのパーティ定義をクリア
 */
void CreaturePartyList::reset()
{
    this->parties.clear();
}
