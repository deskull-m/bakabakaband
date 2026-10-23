#pragma once

#include "info-reader/parse-error-types.h"

#include <initializer_list>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#include <string_view>

struct skill_table;

struct SkillReadError {
    std::string class_id;
    std::string path;
    std::string reason;
};

class SkillReader {
public:
    explicit SkillReader(const nlohmann::json &class_data);
    SkillReader(nlohmann::json &&) = delete;
    SkillReader(const SkillReader &) = delete;
    SkillReader(SkillReader &&) = delete;
    SkillReader &operator=(const SkillReader &) = delete;
    SkillReader &operator=(SkillReader &&) = delete;

    ParseErrorType read();
    const std::optional<SkillReadError> &error() const;

private:
    ParseErrorType read_weapons(skill_table &skills);
    ParseErrorType read_skills(skill_table &skills);
    ParseErrorType check_keys(const nlohmann::json &data, std::string_view path, std::initializer_list<std::string_view> keys);
    ParseErrorType read_integer(const nlohmann::json &data, int &value, int maximum, std::string_view path);
    ParseErrorType fail(ParseErrorType code, std::string_view path, std::string_view reason);

    const nlohmann::json &class_data;
    std::string class_id;
    std::optional<SkillReadError> diagnostic;
};
