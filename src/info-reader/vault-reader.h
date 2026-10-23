#pragma once

#include "info-reader/parse-error-types.h"

#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#include <string_view>

struct vault_type;

struct VaultReadError {
    std::string id;
    std::string path;
    std::string reason;
};

class VaultReader {
public:
    explicit VaultReader(const nlohmann::json &data);
    VaultReader(nlohmann::json &&) = delete;
    VaultReader(const VaultReader &) = delete;
    VaultReader(VaultReader &&) = delete;
    VaultReader &operator=(const VaultReader &) = delete;
    VaultReader &operator=(VaultReader &&) = delete;

    parse_error_type read();
    const std::optional<VaultReadError> &error() const;

private:
    parse_error_type fail(parse_error_type code, std::string_view path, std::string_view reason);
    parse_error_type read_integer(std::string_view key, int &value, int minimum, int maximum);
    parse_error_type read_optional_integer(std::string_view key, int &value, int minimum, int maximum);
    parse_error_type read_flags(vault_type &vault);
    parse_error_type read_features(vault_type &vault);
    const nlohmann::json &data;
    std::optional<VaultReadError> diagnostic;
};
