#pragma once

/*!
 * @file artifact-record.h
 * @brief ゲーム中可変な固定アーティファクト情報 (ArtifactType から分離した実行時状態)
 * @details ArtifactType から is_generated / floor_id を分離して保持する。
 *          上流 hengband の ArtifactRecord/ArtifactRecords (PR #5436 系) に相当するが、
 *          bakabakaband は ArtifactType を維持しているため FixedArtifactId をキーに保持する。
 */

#include "system/angband.h"
#include "util/abstract-map-wrapper.h"
#include <map>

class ArtifactRecord {
public:
    ArtifactRecord() = default;

    bool get_generated() const;
    FLOOR_IDX get_floor_id() const;

    void set_generated(bool new_state);
    void set_floor_id(FLOOR_IDX new_floor_id);

private:
    bool is_generated = false; //!< 生成済か否か (生成済でも、「保存モードON」かつ「帰還等で鑑定前に消滅」したら未生成状態に戻る)
    FLOOR_IDX floor_id = 0; //!< アイテムを落としたフロアのID
};

enum class FixedArtifactId : short;
class ArtifactRecords : public util::AbstractMapWrapper<FixedArtifactId, ArtifactRecord> {
public:
    ArtifactRecords(ArtifactRecords &&) = delete;
    ArtifactRecords(const ArtifactRecords &) = delete;
    ArtifactRecords &operator=(const ArtifactRecords &) = delete;
    ArtifactRecords &operator=(ArtifactRecords &&) = delete;
    ~ArtifactRecords() = default;

    static ArtifactRecords &get_instance();

    bool get_generated(FixedArtifactId fa_id) const;
    FLOOR_IDX get_floor_id(FixedArtifactId fa_id) const;

    void set_generated(FixedArtifactId fa_id, bool new_state);
    void set_floor_id(FixedArtifactId fa_id, FLOOR_IDX new_floor_id);
    void reset_generated_flags();

private:
    ArtifactRecords() = default;
    static ArtifactRecords instance;

    std::map<FixedArtifactId, ArtifactRecord> records;

    std::map<FixedArtifactId, ArtifactRecord> &get_inner_container() override
    {
        return this->records;
    }
};
