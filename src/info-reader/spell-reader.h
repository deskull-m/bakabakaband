#pragma once

#include "info-reader/parse-error-types.h"

#include <nlohmann/json_fwd.hpp>

enum class RealmType;
class SpellInfoList;

class SpellReader {
public:
    SpellReader(const nlohmann::json &realm_data, SpellInfoList &spell_info_list);
    SpellReader(nlohmann::json &&, SpellInfoList &) = delete;
    SpellReader(const SpellReader &) = delete;
    SpellReader(SpellReader &&) = delete;
    SpellReader &operator=(const SpellReader &) = delete;
    SpellReader &operator=(SpellReader &&) = delete;

    ParseErrorType read() const;

private:
    ParseErrorType set_realm(RealmType &realm) const;
    ParseErrorType set_spell_data(const nlohmann::json &spell_data, RealmType realm) const;
    ParseErrorType set_book_data(RealmType realm) const;

    const nlohmann::json &realm_data;
    SpellInfoList &spell_info_list;
};
