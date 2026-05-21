#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

std::vector<std::string> tokenize(std::string_view buf, size_t num);
