#pragma once

#include "locale/localized-string.h"
#include "system/angband.h"
#include "util/dice.h"
#include <string>
#include <vector>

enum class MonraceId : int16_t;

/*!
 * @brief モンスターパーティのメンバー定義
 */
class CreaturePartyMember {
public:
    CreaturePartyMember(MonraceId monrace_id, const Dice &count_dice, int probability);

    MonraceId monrace_id; //!< モンスター種族ID
    Dice count_dice; //!< 出現数ダイス (XdY形式)
    int probability; //!< 出現確率 (%)
};

/*!
 * @brief モンスターパーティの定義
 * @details
 * モンスターがパーティ編成で出現する際の定義を管理する
 * 複数のモンスター種族を組み合わせて編成を作成できる
 */
class CreaturePartyDefinition {
public:
    CreaturePartyDefinition();
    CreaturePartyDefinition(const CreaturePartyDefinition &) = delete;
    CreaturePartyDefinition &operator=(const CreaturePartyDefinition &) = delete;
    CreaturePartyDefinition(CreaturePartyDefinition &&) = default;
    CreaturePartyDefinition &operator=(CreaturePartyDefinition &&) = delete;

    int idx{}; //!< パーティID
    LocalizedString name{}; //!< パーティ名
    std::string tag = ""; //!< パーティのタグ
    int min_level{}; //!< 出現最小階層レベル
    int max_level{}; //!< 出現最大階層レベル
    int rarity{}; //!< レアリティ (値が小さいほど出現しやすい)
    bool allow_unique{}; //!< ユニークモンスターを含むか
    std::vector<CreaturePartyMember> members; //!< パーティメンバーのリスト

    bool is_valid() const;
    bool can_generate(int current_level) const;
    int get_total_monster_count() const;
    void add_member(MonraceId monrace_id, const Dice &count_dice, int probability);
};
