#pragma once

#include "system/creature-party/creature-party-definition.h"
#include "util/abstract-map-wrapper.h"
#include <map>
#include <memory>

class CreaturePartyDefinition;

/*!
 * @brief モンスターパーティの定義リスト管理クラス
 * @details
 * すべてのモンスターパーティ定義を管理するシングルトンクラス
 */
class CreaturePartyList : public util::AbstractMapWrapper<int, CreaturePartyDefinition> {
public:
    CreaturePartyList(CreaturePartyList &&) = delete;
    CreaturePartyList(const CreaturePartyList &) = delete;
    CreaturePartyList &operator=(const CreaturePartyList &) = delete;
    CreaturePartyList &operator=(CreaturePartyList &&) = delete;
    ~CreaturePartyList() override = default;

    static CreaturePartyList &get_instance();

    /*!
     * @brief パーティ定義を取得 (const版)
     * @param party_id パーティID
     * @return パーティ定義への参照
     */
    const CreaturePartyDefinition &get_party(int party_id) const;

    /*!
     * @brief パーティ定義を取得 (非const版)
     * @param party_id パーティID
     * @return パーティ定義への参照
     */
    CreaturePartyDefinition &get_party(int party_id);

    /*!
     * @brief 新しいパーティ定義を追加
     * @param party_id パーティID
     * @return 追加されたパーティ定義への参照
     */
    CreaturePartyDefinition &emplace(int party_id);

    /*!
     * @brief 指定された階層で生成可能なパーティのリストを取得
     * @param current_level 現在の階層レベル
     * @return 生成可能なパーティIDのリスト
     */
    std::vector<int> get_available_parties(int current_level) const;

    /*!
     * @brief パーティIDが有効かどうか判定
     * @param party_id パーティID
     * @return パーティIDが有効ならtrue
     */
    static bool is_valid(int party_id);

    /*!
     * @brief 空のパーティIDを返す
     * @return 0 (無効なパーティID)
     */
    static int empty_id();

    /*!
     * @brief すべてのパーティ定義をクリア
     */
    void reset();

private:
    CreaturePartyList() = default;

    std::map<int, CreaturePartyDefinition> parties;

    std::map<int, CreaturePartyDefinition> &get_inner_container() override
    {
        return this->parties;
    }
};
