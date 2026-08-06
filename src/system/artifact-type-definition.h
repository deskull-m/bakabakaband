#pragma once

#include "object-enchant/tr-flags.h"
#include "object-enchant/trg-types.h"
#include "system/angband.h"
#include "system/baseitem/baseitem-key.h"
#include "util/abstract-map-wrapper.h"
#include "util/dice.h"
#include "util/flag-group.h"
#include <map>
#include <string>

/*!
 * @class ArtifactType
 * @brief 固定アーティファクト情報の構造体 / Artifact structure.
 * @details 実行時可変な生成状態 (is_generated / floor_id) は ArtifactRecords に分離している
 */
enum class FixedArtifactId : short;
enum class RandomArtActType : short;
class ArtifactType {
public:
    ArtifactType();
    ArtifactType(const ArtifactType &) = delete;
    ArtifactType &operator=(const ArtifactType &) = delete;
    ArtifactType(ArtifactType &&) = default;
    ArtifactType &operator=(ArtifactType &&) = delete;

    std::string name; /*!< アーティファクト名 / Name */
    std::string text; /*!< アーティファクト解説 / Text */
    BaseitemKey bi_key;
    PARAMETER_VALUE pval{}; /*!< pval修正値 / Artifact extra info */
    HIT_PROB to_h{}; /*!< 命中ボーナス値 /  Bonus to hit */
    int to_d{}; /*!< ダメージボーナス値 / Bonus to damage */
    ARMOUR_CLASS to_a{}; /*!< ACボーナス値 / Bonus to armor */
    ARMOUR_CLASS ac{}; /*!< 上書きベースAC値 / Base armor */
    Dice damage_dice; /*!< ダイス値 / Damage when hits */
    WEIGHT weight{}; /*!< 重量 / Weight */
    PRICE cost{}; /*!< 基本価格 / Artifact "cost" */
    TrFlags flags{}; /*! アイテムフラグ / Artifact Flags */
    EnumClassFlagGroup<ItemGenerationTraitType> gen_flags; /*! アイテム生成フラグ / flags for generate */
    DEPTH level{}; /*! 基本生成階 / Artifact level */
    RARITY rarity{}; /*! レアリティ / Artifact rarity */
    RandomArtActType act_idx{}; /*! 発動能力ID / Activative ability index */
    PERCENTAGE broken_rate; /*!< 発動破損率 */

    bool can_generate(FixedArtifactId fa_id, const BaseitemKey &bi_key) const;
    tl::optional<BaseitemKey> try_make_instant_artifact(FixedArtifactId fa_id, int making_level) const;
    std::string build_full_name() const;

private:
    bool can_make_instant_artifact(FixedArtifactId fa_id) const;
    bool evaluate_shallow_instant_artifact(int making_level) const;
    bool evaluate_rarity() const;
    bool evaluate_shallow_baseitem(int making_level) const;
};

class ItemEntity;
class ArtifactList : public util::AbstractMapWrapper<FixedArtifactId, ArtifactType> {
public:
    ArtifactList(const ArtifactList &) = delete;
    ArtifactList(ArtifactList &&) = delete;
    ArtifactList &operator=(const ArtifactList &) = delete;
    ArtifactList &operator=(ArtifactList &&) = delete;
    ~ArtifactList() = default;

    static ArtifactList &get_instance();
    const ArtifactType &get_artifact(const FixedArtifactId fa_id) const;
    ArtifactType &get_artifact(const FixedArtifactId fa_id);

    bool order(const FixedArtifactId id1, const FixedArtifactId id2) const;
    void emplace(const FixedArtifactId fa_id, ArtifactType &&artifact);
    tl::optional<ItemEntity> try_make_instant_artifact(int making_level) const;
    std::string get_full_name(const FixedArtifactId fa_id) const;

private:
    ArtifactList() = default;
    static ArtifactList instance;
    static ArtifactType dummy;

    std::map<FixedArtifactId, ArtifactType> artifacts{};

    void validate_fa_id(const FixedArtifactId fa_id) const;

    std::map<FixedArtifactId, ArtifactType> &get_inner_container() override
    {
        return this->artifacts;
    }
};
