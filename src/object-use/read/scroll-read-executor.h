#pragma once

#include "object-use/read/read-executor-base.h"

class CreatureEntity;
class ItemEntity;
class ScrollReadExecutor : public ReadExecutorBase {
public:
    ScrollReadExecutor(CreatureEntity &creature, ItemEntity *o_ptr, bool known);
    ScrollReadExecutor &operator=(const ScrollReadExecutor &) = delete;
    ScrollReadExecutor &operator=(ScrollReadExecutor &&) = delete;
    bool is_identified() const override;
    bool read() override;

private:
    CreatureEntity &creature;
    ItemEntity *o_ptr;
    bool known;
    bool ident = false;
};
