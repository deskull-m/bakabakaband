#pragma once

#include "system/angband.h"
#include <cstdint>

enum class StoreSaleType;
class CreatureEntity;
void store_owner_says_comment(int price, StoreSaleType store_num);
void purchase_analyze(CreatureEntity &creature, PRICE price, int64_t value, int64_t guess);
