#pragma once

#include "system/angband.h"

enum class StoreSaleType;
class CreatureEntity;
void store_owner_says_comment(int price, StoreSaleType store_num);
void purchase_analyze(CreatureEntity &creature, PRICE price, PRICE value, PRICE guess);
