#pragma once

#include "room/vault-flag-types.h"
#include <string_view>
#include <unordered_map>

const std::unordered_map<std::string_view, VaultFeatureType> vault_flags = {
    { "NO_ROTATION", VaultFeatureType::NO_ROTATION },
};