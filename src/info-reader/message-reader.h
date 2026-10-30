#pragma once

#include "info-reader/parse-error-types.h"

#include <nlohmann/json_fwd.hpp>

class MessageReader {
public:
    explicit MessageReader(const nlohmann::json &message_data);
    MessageReader(nlohmann::json &&) = delete;
    MessageReader(const MessageReader &) = delete;
    MessageReader(MessageReader &&) = delete;
    MessageReader &operator=(const MessageReader &) = delete;
    MessageReader &operator=(MessageReader &&) = delete;

    ParseErrorType read() const;

private:
    ParseErrorType set_mon_message() const;

    const nlohmann::json &message_data;
};
