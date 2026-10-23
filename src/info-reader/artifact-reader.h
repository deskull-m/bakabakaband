#pragma once

#include "info-reader/parse-error-types.h"
#include "system/angband.h"
#include <nlohmann/json_fwd.hpp>
#include <string_view>

class ArtifactType;

class ArtifactReader {
public:
    explicit ArtifactReader(const nlohmann::json &art_data);
    ArtifactReader(nlohmann::json &&) = delete;
    ArtifactReader(const ArtifactReader &) = delete;
    ArtifactReader(ArtifactReader &&) = delete;
    ArtifactReader &operator=(const ArtifactReader &) = delete;
    ArtifactReader &operator=(ArtifactReader &&) = delete;

    ParseErrorType read() const;

private:
    bool grab_one_artifact_flag(ArtifactType &artifact, std::string_view what) const;
    ParseErrorType set_art_baseitem(ArtifactType &artifact) const;
    ParseErrorType set_art_activate(ArtifactType &artifact) const;
    ParseErrorType set_art_flags(ArtifactType &artifact) const;

    const nlohmann::json &art_data;
};
