#pragma once

#include "object-use/read/read-executor-base.h"

class CreatureEntity;
class ItemEntity;
class ParchmentReadExecutor : public ReadExecutorBase {
public:
    ParchmentReadExecutor(CreatureEntity &creature, ItemEntity *o_ptr);
    ParchmentReadExecutor(const ParchmentReadExecutor &) = default;
    ParchmentReadExecutor(ParchmentReadExecutor &&) = default;
    ParchmentReadExecutor &operator=(const ParchmentReadExecutor &) = delete;
    ParchmentReadExecutor &operator=(ParchmentReadExecutor &&) = delete;
    bool read() override;
    bool is_identified() const override;

private:
    CreatureEntity &creature;
    ItemEntity *o_ptr;
};
