#include "system/artifact/artifact-record.h"

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
    const auto it = this->records.find(fa_id);
    return (it != this->records.end()) && it->second.get_generated();
}

FLOOR_IDX ArtifactRecords::get_floor_id(FixedArtifactId fa_id) const
{
    const auto it = this->records.find(fa_id);
    return (it != this->records.end()) ? it->second.get_floor_id() : 0;
}

void ArtifactRecords::set_generated(FixedArtifactId fa_id, bool new_state)
{
    this->records[fa_id].set_generated(new_state);
}

void ArtifactRecords::set_floor_id(FixedArtifactId fa_id, FLOOR_IDX new_floor_id)
{
    this->records[fa_id].set_floor_id(new_floor_id);
}

void ArtifactRecords::reset_generated_flags()
{
    for (auto &[_, record] : this->records) {
        record.set_generated(false);
    }
}
