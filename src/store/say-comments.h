#pragma once

#include "system/angband.h"

enum class StoreSaleType;
class CreatureEntity;
void store_owner_says_comment(CreatureEntity &creature, StoreSaleType store_num);
void purchase_analyze(CreatureEntity &creature, PRICE price, PRICE value, PRICE guess);
