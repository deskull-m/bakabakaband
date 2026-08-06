#include "system/artifact-type-definition.h"
#include "artifact/fixed-art-types.h"
#include "object-enchant/tr-types.h"
#include "object/tval-types.h"
#include "system/angband-exceptions.h"
#include "system/artifact/artifact-record.h"
#ifdef JP
#else
#include "util/string-processor.h"
#include <sstream>
#endif
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/item-entity.h"
#include "util/enum-converter.h"

ArtifactType::ArtifactType()
    : bi_key(BaseitemKey(ItemKindType::NONE))
{
}

/*!
 * @brief アーティファクトが生成可能か否かを確認する
 * @param bi_key 生成しようとするアーティファクトのベースアイテムキー
 * @param level プレイヤーが今いる階層
 */
bool ArtifactType::can_generate(FixedArtifactId fa_id, const BaseitemKey &generaing_bi_key) const
{
    if (ArtifactRecords::get_instance().get_generated(fa_id)) {
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
 * @brief アーティファクトを『』も考慮しつつ完全な名前を返す
 *
 * 日本語版：
 * FULL_NAMEフラグつきの場合、そのまま
 * 例1：運命のオーブ → ★運命のオーブ
 * 例2：トゲトゲバット『エスカリボルグ』 → ★トゲトゲバット『エスカリボルグ』
 *
 * FULL_NAMEフラグなしの場合、ベースアイテム名を付与する
 * - 『』ありなら前置する
 * - 『』なしなら後置する
 * 例1：『ナルサンク』 → ★ダガー『ナルサンク』
 * 例2：シヴァの化身の → ★シヴァの化身の軟革ブーツ
 *
 * English version:
 * If the artifact has the FULL_NAME flag, return it as is.
 * All fixed artifacts with "The", then it starts capital always; there must not be any name starting other than "The", such as "the".
 * Example1: 'Excaliborg, the *thorny* bat' => The 'Excaliborg, the *thorny* bat'
 * Example2: The Orb of Fate => The Orb of Fate
 *
 * If an artifact does not have the FULL_NAME flag, the baseitem name is always prefixed and the fixed artifact name is appended.
 * Example1: 'Narthanc' => The Dagger 'Narthanc'
 * Example2: of Corwin => The Set of Gauntlets of Corwin
 */
std::string ArtifactType::build_full_name() const
{
#ifdef JP
    std::string full_name("★");
    if (this->flags.has(TR_FULL_NAME)) {
        return full_name.append(this->name);
    }

    constexpr auto start = "『";
    const auto is_preposition = this->name.starts_with(start);
    const auto &baseitems = BaseitemList::get_instance();
    if (is_preposition) {
        return full_name.append(baseitems.lookup_baseitem(this->bi_key).name).append(this->name);
    }

    return full_name.append(this->name).append(baseitems.lookup_baseitem(this->bi_key).name);
#else
    constexpr auto definite_article = "The";
    if (this->flags.has(TR_FULL_NAME)) {
        std::stringstream ss;
        if (!this->name.starts_with(definite_article)) {
            ss << definite_article << ' ';
        }

        ss << this->name;
        return ss.str();
    }

    const auto &baseitems = BaseitemList::get_instance();
    const auto &baseitem = baseitems.lookup_baseitem(this->bi_key);
    const auto baseitem_name_without_numeral = str_replace(baseitem.name, "&", "");
    const auto baseitem_name_singular = str_replace(baseitem_name_without_numeral, "~", "");
    std::stringstream ss;
    ss << definite_article;
    if (!baseitem_name_singular.starts_with(' ')) {
        ss << ' ';
    }

    ss << baseitem_name_singular << ' ' << this->name;
    return ss.str();
#endif
}

/*!
 * @brief INSTA_ART型の固定アーティファクト生成を試みる
 * @param 生成基準階層 (現在フロアそのものではなくボーナスつき)
 * @param fa_id 固定アーティファクトID
 * @return 生成に成功したらそのアイテム、失敗したらnullopt
 */
tl::optional<BaseitemKey> ArtifactType::try_make_instant_artifact(FixedArtifactId fa_id, int making_level) const
{
    if (!this->can_make_instant_artifact(fa_id)) {
        return tl::nullopt;
    }

    if (!this->evaluate_shallow_instant_artifact(making_level)) {
        return tl::nullopt;
    }

    if (!this->evaluate_rarity()) {
        return tl::nullopt;
    }

    if (!this->evaluate_shallow_baseitem(making_level)) {
        return tl::nullopt;
    }

    return this->bi_key;
}

/*!
 * @brief INSTA_ARTフラグ付きアーティファクトの生成可否を判定する
 * @return 生成可否
 * @details 生成済、クエスト属性付き、非INSTA_ARTはfalse、普通のINSTA_ARTはtrue
 */
bool ArtifactType::can_make_instant_artifact(FixedArtifactId fa_id) const
{
    auto can_make = !ArtifactRecords::get_instance().get_generated(fa_id);
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

/*!
 * @brief 標準生成階層より浅い階層でのベースアイテム生成制限を判定する
 * @return 生成可否
 * @details 1/(不足階層*5) を満たさないと生成しない
 */
bool ArtifactType::evaluate_shallow_baseitem(int making_level) const
{
    const auto &baseitems = BaseitemList::get_instance();
    const auto &baseitem = baseitems.lookup_baseitem(this->bi_key);
    if (baseitem.level <= making_level) {
        return true;
    }

    return one_in_((baseitem.level - making_level) * 5);
}

ArtifactList ArtifactList::instance{};

ArtifactType ArtifactList::dummy{};

ArtifactList &ArtifactList::get_instance()
{
    return instance;
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

void ArtifactList::emplace(const FixedArtifactId fa_id, ArtifactType &&artifact)
{
    this->artifacts.emplace(fa_id, std::move(artifact));
}

std::string ArtifactList::get_full_name(const FixedArtifactId fa_id) const
{
    this->validate_fa_id(fa_id);
    if (fa_id == FixedArtifactId::NONE) {
        return "";
    }

    return this->artifacts.at(fa_id).build_full_name();
}

void ArtifactList::validate_fa_id(const FixedArtifactId fa_id) const
{
    if (fa_id < FixedArtifactId::NONE || fa_id > i2enum<FixedArtifactId>(this->artifacts.size())) {
        THROW_EXCEPTION(std::out_of_range, "Invalid FixedArtifactId: " + std::to_string(static_cast<int>(fa_id)));
    }
}

tl::optional<ItemEntity> ArtifactList::try_make_instant_artifact(int making_level) const
{
    for (const auto &[fa_id, artifact] : this->artifacts) {
        const auto bi_key = artifact.try_make_instant_artifact(fa_id, making_level);
        if (bi_key) {
            ItemEntity instant_artifact(*bi_key);
            instant_artifact.fa_id = fa_id;
            return instant_artifact;
        }
    }

    return tl::nullopt;
}
