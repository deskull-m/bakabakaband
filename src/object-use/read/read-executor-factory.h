#pragma once

#include "object-use/read/read-executor-base.h"
#include <memory>

class CreatureEntity;
class ItemEntity;
class ReadExecutorFactory {
public:
    static std::unique_ptr<ReadExecutorBase> create(CreatureEntity &creature, ItemEntity *o_ptr, bool known);

private:
    ReadExecutorFactory() = delete;
};
