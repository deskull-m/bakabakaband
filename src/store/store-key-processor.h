#pragma once

extern bool leave_store;

enum class StoreSaleType;
class CreatureEntity;
void store_process_command(CreatureEntity &creature, StoreSaleType store_num);
