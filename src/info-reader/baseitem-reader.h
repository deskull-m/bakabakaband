#pragma once

#include "info-reader/parse-error-types.h"

#include <nlohmann/json_fwd.hpp>
#include <string_view>

class BaseitemDefinition;

class BaseitemReader {
public:
    BaseitemReader(const nlohmann::json &baseitem_data, int &previous_id);
    BaseitemReader(nlohmann::json &&, int &) = delete;
    BaseitemReader(const BaseitemReader &) = delete;
    BaseitemReader(BaseitemReader &&) = delete;
    BaseitemReader &operator=(const BaseitemReader &) = delete;
    BaseitemReader &operator=(BaseitemReader &&) = delete;

    ParseErrorType read() const;

private:
    bool grab_one_baseitem_flag(BaseitemDefinition &baseitem, std::string_view what) const;
    ParseErrorType set_baseitem_symbol(BaseitemDefinition &baseitem) const;
    ParseErrorType set_baseitem_kind(BaseitemDefinition &baseitem) const;
    ParseErrorType set_baseitem_parameter_value(BaseitemDefinition &baseitem) const;
    ParseErrorType set_baseitem_allocations(BaseitemDefinition &baseitem) const;
    ParseErrorType set_baseitem_activate(BaseitemDefinition &baseitem) const;
    ParseErrorType set_baseitem_alliance(BaseitemDefinition &baseitem) const;
    ParseErrorType set_baseitem_flags(BaseitemDefinition &baseitem) const;

    const nlohmann::json &baseitem_data;
    int &previous_id; //!< 直前に読み込んだベースアイテムID (昇順・重複の検証用。読み込み全体で共有)
};
