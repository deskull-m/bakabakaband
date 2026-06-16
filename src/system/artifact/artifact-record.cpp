#include "system/artifact/artifact-record.h"
#include "artifact/fixed-art-types.h"
#include "system/angband-exceptions.h"
#include "system/artifact-type-definition.h"
#include "util/enum-converter.h"
#include <fmt/format.h>

bool ArtifactRecord::get_generated() const
{
    return this->is_generated;
}

FLOOR_IDX ArtifactRecord::get_floor_id() const
{
    return this->floor_id;
}

void ArtifactRecord::set_generated(bool new_state)
{
    this->is_generated = new_state;
}

void ArtifactRecord::set_floor_id(FLOOR_IDX new_floor_id)
{
    this->floor_id = new_floor_id;
}

ArtifactRecords ArtifactRecords::instance{};

ArtifactRecords &ArtifactRecords::get_instance()
{
    return instance;
}

bool ArtifactRecords::get_generated(FixedArtifactId fa_id) const
{
    this->validate_fixed_artifact_id(fa_id);
    const auto it = this->records.find(fa_id);
    return (it != this->records.end()) && it->second.get_generated();
}

FLOOR_IDX ArtifactRecords::get_floor_id(FixedArtifactId fa_id) const
{
    this->validate_fixed_artifact_id(fa_id);
    const auto it = this->records.find(fa_id);
    return (it != this->records.end()) ? it->second.get_floor_id() : 0;
}

void ArtifactRecords::set_generated(FixedArtifactId fa_id, bool new_state)
{
    this->validate_fixed_artifact_id(fa_id);
    this->records[fa_id].set_generated(new_state);
}

void ArtifactRecords::set_floor_id(FixedArtifactId fa_id, FLOOR_IDX new_floor_id)
{
    this->validate_fixed_artifact_id(fa_id);
    this->records[fa_id].set_floor_id(new_floor_id);
}

void ArtifactRecords::reset_generated_flags()
{
    for (auto &[_, record] : this->records) {
        record.set_generated(false);
    }
}

/*!
 * @brief 固定アーティファクトIDの妥当性を検証する
 * @param fa_id 検証する固定アーティファクトID
 * @details bakabakaband の ArtifactRecords は遅延挿入方式で records.size() が妥当性の上限とならないため、
 *          定義の権威である ArtifactList の最大IDを上限として検証する (save.cpp の最大ID算出と同方式)。
 */
void ArtifactRecords::validate_fixed_artifact_id(FixedArtifactId fa_id) const
{
    const auto &artifacts = ArtifactList::get_instance();
    const auto max_fa_id = (artifacts.begin() != artifacts.end()) ? enum2i(artifacts.rbegin()->first) : 0;
    if ((fa_id < FixedArtifactId::NONE) || (enum2i(fa_id) > max_fa_id)) {
        THROW_EXCEPTION(std::out_of_range, fmt::format("Invalid Fixed Artifact ID: {}", enum2i(fa_id)));
    }
}
