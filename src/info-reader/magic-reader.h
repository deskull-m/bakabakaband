#pragma once

#include "info-reader/parse-error-types.h"

#include <nlohmann/json_fwd.hpp>

enum class RealmType;
struct player_magic;

class MagicReader {
public:
    explicit MagicReader(const nlohmann::json &class_data);
    MagicReader(nlohmann::json &&) = delete;
    MagicReader(const MagicReader &) = delete;
    MagicReader(MagicReader &&) = delete;
    MagicReader &operator=(const MagicReader &) = delete;
    MagicReader &operator=(MagicReader &&) = delete;

    ParseErrorType read() const;

private:
    ParseErrorType set_class_id(int &class_id) const;
    ParseErrorType set_spell_type(player_magic &magics_info) const;
    ParseErrorType set_magic_status(player_magic &magics_info) const;
    ParseErrorType set_spell_data(const nlohmann::json &spell_data, player_magic &magics_info, RealmType realm) const;
    ParseErrorType set_realm_data(player_magic &magics_info) const;

    const nlohmann::json &class_data;
};
