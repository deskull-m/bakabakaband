#include "system/artifact-type-definition.h"
#include "artifact/fixed-art-types.h"
#include "object/tval-types.h"

ArtifactType::ArtifactType()
    : bi_key(BaseitemKey(ItemKindType::NONE))
{
}

/*!
 * @brief アーティファクトが生成可能か否かを確認する
 * @param bi_key 生成しようとするアーティファクトのベースアイテムキー
 * @param level プレイヤーが今いる階層
 */
bool ArtifactType::can_generate(const BaseitemKey &generaing_bi_key) const
{
    if (this->is_generated) {
        return false;
    }

    if (this->gen_flags.has(ItemGenerationTraitType::QUESTITEM)) {
        return false;
    }

    if (this->gen_flags.has(ItemGenerationTraitType::INSTA_ART)) {
        return false;
    }

    return this->bi_key == generaing_bi_key;
}

/*!
 * @brief INSTA_ARTフラグ付きアーティファクトの生成可否を判定する
 * @return 生成可否
 * @details 生成済、クエスト属性付き、非INSTA_ARTはfalse、普通のINSTA_ARTはtrue
 */
bool ArtifactType::can_make_instant_artifact() const
{
    auto can_make = !this->is_generated;
    can_make &= this->gen_flags.has_not(ItemGenerationTraitType::QUESTITEM);
    can_make &= this->gen_flags.has(ItemGenerationTraitType::INSTA_ART);
    return can_make;
}

/*!
 * @brief 標準生成階層より浅い階層での生成制限を判定する
 * @return 生成可否
 * @details 1/(不足階層*2) を満たさないと生成しない
 */
bool ArtifactType::evaluate_shallow_instant_artifact(int making_level) const
{
    if (this->level <= making_level) {
        return true;
    }

    return one_in_((this->level - making_level) * 2);
}

/*!
 * @brief レアリティによる生成制限を判定する
 * @return 生成可否
 */
bool ArtifactType::evaluate_rarity() const
{
    return one_in_(this->rarity);
}

std::map<FixedArtifactId, ArtifactType> artifacts;

ArtifactList ArtifactList::instance{};

ArtifactType ArtifactList::dummy{};

ArtifactList &ArtifactList::get_instance()
{
    return instance;
}

std::map<FixedArtifactId, ArtifactType>::iterator ArtifactList::begin()
{
    return this->artifacts.begin();
}

std::map<FixedArtifactId, ArtifactType>::iterator ArtifactList::end()
{
    return this->artifacts.end();
}

std::map<FixedArtifactId, ArtifactType>::const_iterator ArtifactList::begin() const
{
    return this->artifacts.cbegin();
}

std::map<FixedArtifactId, ArtifactType>::const_iterator ArtifactList::end() const
{
    return this->artifacts.cend();
}

std::map<FixedArtifactId, ArtifactType>::reverse_iterator ArtifactList::rbegin()
{
    return this->artifacts.rbegin();
}

std::map<FixedArtifactId, ArtifactType>::reverse_iterator ArtifactList::rend()
{
    return this->artifacts.rend();
}

std::map<FixedArtifactId, ArtifactType>::const_reverse_iterator ArtifactList::rbegin() const
{
    return this->artifacts.crbegin();
}

std::map<FixedArtifactId, ArtifactType>::const_reverse_iterator ArtifactList::rend() const
{
    return this->artifacts.crend();
}

const ArtifactType &ArtifactList::get_artifact(const FixedArtifactId fa_id) const
{
    if (fa_id == FixedArtifactId::NONE) {
        return dummy;
    }

    return this->artifacts.at(fa_id);
}

ArtifactType &ArtifactList::get_artifact(const FixedArtifactId fa_id)
{
    if (fa_id == FixedArtifactId::NONE) {
        return dummy;
    }

    return this->artifacts.at(fa_id);
}

bool ArtifactList::order(const FixedArtifactId id1, const FixedArtifactId id2) const
{
    const auto &artifact1 = this->get_artifact(id1);
    const auto &artifact2 = this->get_artifact(id2);
    if (artifact1.bi_key < artifact2.bi_key) {
        return true;
    }

    if (artifact1.bi_key > artifact2.bi_key) {
        return false;
    }

    if (artifact1.level < artifact2.level) {
        return true;
    }

    if (artifact1.level > artifact2.level) {
        return false;
    }

    return id1 < id2;
}

void ArtifactList::emplace(const FixedArtifactId fa_id, const ArtifactType &artifact)
{
    this->artifacts.emplace(fa_id, artifact);
}

void ArtifactList::reset_generated_flags()
{
    for (auto &[_, artifact] : this->artifacts) {
        artifact.is_generated = false;
    }
}
